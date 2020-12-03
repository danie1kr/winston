/*

  Minnow Server Reference Example

 ## Use this example with or without TLS.
 The code can be used with or without a TLS stack. The code is
 tailored for the SharkSSL API and the code automatically uses the
 SharkSSL API if compiled together with a SharkSSL delivery. The code
 uses standard non secure connections if not using TLS. See macro
 MS_SEC below for details. Note that all Minnow Server and SMQ APIs
 used in this code utilize the secure API. This also works for non secure
 mode since the header files includes macros that redirect the secure
 API to the non secure API.

 Minnow Server (MS) Reference manual:
 https://realtimelogic.com/ba/doc/en/C/shark/group__MSLib.html


 ## Macro USE_SMQ: compile with or without IoT functionality
 As explained in the example's tutorial, the example by default
 operates as a WebSocket server only. Enable IoT mode by compiling
 with macro USE_SMQ defined.

 SMQ Reference manual
  Standard: https://realtimelogic.com/ba/doc/en/C/reference/html/structSMQ.html
  Secure: https://realtimelogic.com/ba/doc/en/C/shark/group__SMQLib.html


 ## JSON:
 This example requires the following JSON library:
 https://github.com/RealTimeLogic/JSON
 The parser, the JSON node factory, and the JSON encoder can be used
 without a dynamic allocator as we do by default in this code. See the
 macro USE_STATIC_ALLOC below for details. The JSON parser takes a
 JSON value node factory as an argument. The JSON library supports two
 node factories, one design for super small microcontrollers and
 another designed for simplicity. The one for super small micros is
 called JDecoder and the one used for simplicity is called
 JParserValFact. We use JParserValFact in the code below since
 JDecoder is more difficult to use. For a detailed introduction to the
 various JParser options, see the following tutorial:
 https://realtimelogic.com/ba/doc/en/C/reference/html/md_en_C_md_JSON.html
*/


#include "winston-minnow.h"

/* Send a complete JSON message over WebSockets.

   This is the BufPrint flush function that gets called when the
   BufPrint buffer is flushed, either when the buffer is full or when
   actively flushed, however, this code is designed to only accept a
   full JSON message, thus an indirect flush is not accepted. The flag
   "committed" helps us check if we are performing an active flush in
   SendData_commit. The buffer used by BufPrint is either the Minnow
   Server buffer or the SMQ buffer. The buffer is too small if the
   flush callback is called and if the flag "committed" is false.

   The JEncoder object (used for encoding JSON) requires a BufPrint
   instance.
   https://realtimelogic.com/ba/doc/en/C/reference/html/structJEncoder.html
   https://realtimelogic.com/ba/doc/en/C/reference/html/structBufPrint.html
 */
static int
SendData_wsSendJSON(BufPrint* bp, int sizeRequired)
{
    MS* ms;
    SendData* o = (SendData*)bp; /* (Ref-bp) */
    (void)sizeRequired; /* not used */
    /* From SendData_constructor >  BufPrint_setBuf */
    ms = (MS*)BufPrint_getUserData(bp);
    if (!o->committed)
    {
        xprintf(("ERR: WebSocket send buffer too small\n"));
        baAssert(0);/* This is a 'design' error */
        return -1;
    }

    /* Minnow Server (MS) in large mode. Pad with spaces if size less than
       128 (Ref-Size).
       https://realtimelogic.com/ba/doc/en/C/shark/group__MSLib.html
     */
    while (bp->cursor < 128)
        bp->buf[bp->cursor++] = ' '; /* cursor is current bufsize */
    if (MS_sendText(ms, bp->cursor) < 0)
    {
        xprintf(("WebSocket connection closed on send\n"));
        return -1;
    }
    return 0;
}

/* Construct the SendData container object used for sending formatted JSON.
   We use a BufPrint instance as the output buffer for JEncoder (Ref-bp).
   https://realtimelogic.com/ba/doc/en/C/reference/html/structBufPrint.html
   https://realtimelogic.com/ba/doc/en/C/reference/html/structJEncoder.html
*/
static void
SendData_constructor(SendData* o, ConnData* cd)
{
    int sendBufSize;
    U8* buf;
    if (ConnData_WebSocketMode(cd)) /* Always true if USE_SMQ not set */
    {
        BufPrint_constructor(&o->super, cd->u.ms, SendData_wsSendJSON);
        /* Minnow Server: second arg=TRUE: Set size > 128 (Ref-Size) */
        buf = MS_prepSend(cd->u.ms, TRUE, &sendBufSize);
        /* JEncoder is formatting data via BufPrint directly into MS buffer */
        BufPrint_setBuf(&o->super, (char*)buf, sendBufSize);
    }
    JErr_constructor(&o->err);
    JEncoder_constructor(&o->encoder, &o->err, &o->super);
    o->committed = FALSE;
}

/* Called when we are done creating a JSON message.
 */
static int
SendData_commit(SendData* o)
{
    if (o->committed) return -1;
    o->committed = TRUE;
    /* Trigger SendData_wsSendJSON() or SendData_smqSendJSON() */
    return JEncoder_commit(&o->encoder);
}



/****************************  Application ********************************/

/* All JSON messages start with the following: [messagename, ... */
static void
beginMessage(SendData* sd, const char* messagename)
{
    JEncoder_beginArray(&sd->encoder);
    JEncoder_setString(&sd->encoder, messagename);
}

/* All messages end with: ...] */
static int
endMessage(SendData* sd)
{
    JEncoder_endArray(&sd->encoder);
    return SendData_commit(sd);
}

/* For the LED example. Convert type to string *
static const char*
ledType2String(LedColor t)
{
    switch (t)
    {
    case LedColor_red: return "red";
    case LedColor_yellow: return "yellow";
    case LedColor_green: return "green";
    case LedColor_blue: return "blue";
    }
    baAssert(0);
    return "";
}



/*
   ["ledinfo", {"leds" : [...]}]
   where ... is one or several objects of type:
      {"name":"ledname","id":number, "color":"cname"}
      See func. ledType2String for color names
 *
static int
sendLedInfo(ConnData* cd)
{
    int i, ledLen;
    SendData sd;
    const LedInfo* ledInfo = getLedInfo(&ledLen);
    SendData_constructor(&sd, cd);
    beginMessage(&sd, "ledinfo");
    JEncoder_beginObject(&sd.encoder);
    JEncoder_setName(&sd.encoder, "leds");
    JEncoder_beginArray(&sd.encoder);
    for (i = 0; i < ledLen; i++)
    {
        JEncoder_set(&sd.encoder, "{dssb}",
            JE_MEMBER(ledInfo + i, id),
            "color", ledType2String(ledInfo[i].color),
            JE_MEMBER(ledInfo + i, name),
            "on", (BaBool)getLedState(ledInfo[i].id));
    }
    JEncoder_endArray(&sd.encoder);
    JEncoder_endObject(&sd.encoder);
    return endMessage(&sd);
}
*/

void minnowSendPrepare(ConnData* cd, SendData *sd, const char* op)
{
    
    SendData_constructor(sd, cd);
    //beginMessage(sd, msg);

    JEncoder_beginObject(&sd->encoder);
    JEncoder_setName(&sd->encoder, "op");
    JEncoder_setString(&sd->encoder, op);
    JEncoder_setName(&sd->encoder, "data");
}

void minnowSendFill(SendData* sd, const char* msg, const char* fmt, ...)
{
    va_list varargs;
    va_start(varargs, fmt);
    JEncoder_set(&sd->encoder, fmt, varargs);
}

int minnowSendSubmit(SendData* sd)
{
    JEncoder_endObject(&sd->encoder);
    return SendData_commit(sd);
    //return endMessage(sd);
}

/*
   ["setled", {"id": number, "on": boolean}]
 *
static int
sendSetLED(ConnData* cd, int ledId, int on)
{
    SendData sd;
    SendData_constructor(&sd, cd);
    beginMessage(&sd, "setled");
    JEncoder_set(&sd.encoder, "{db}",
        "id", ledId,
        "on", (BaBool)on);
    return endMessage(&sd);
}

/*
   Manage 'setled' command received from browser.
 *
static int
manageSetLED(ConnData* cd, JErr* e, JVal* v)
{
    S32 ledID;
    BaBool on;
    JVal_get(v, e, "{db}", "id", &ledID, "on", &on);
    if (JErr_isError(e) || setLed(ledID, (int)on)) return -1;
    return sendSetLED(cd, ledID, (int)on);
}


/*
   ["settemp", number]
 *
static int
sendSetTemp(ConnData* cd, int temp)
{
    SendData sd;
    SendData_constructor(&sd, cd);
    beginMessage(&sd, "settemp");
    JEncoder_setInt(&sd.encoder, temp);
    return endMessage(&sd);
}


/*
  ["DeviceName", ["the-name"]
 *
static int
sendDeviceName(ConnData* cd)
{
    SendData sd;
    SendData_constructor(&sd, cd);
    beginMessage(&sd, "devname");
    JEncoder_beginArray(&sd.encoder);
    JEncoder_setString(&sd.encoder, getDevName());
    JEncoder_endArray(&sd.encoder);
    return endMessage(&sd);
}

/*
  ["uploadack", number]
 *
static int
sendUploadAck(ConnData* cd, S32 messages)
{
    SendData sd;
    SendData_constructor(&sd, cd);
    beginMessage(&sd, "uploadack");
    JEncoder_setInt(&sd.encoder, messages);
    return endMessage(&sd);
}


/* The idle function, which simulates events in the system, is called
 * when not receiving data.
 *
static int
eventSimulator(ConnData* cd)
{
    int x; /* used for storing ledId and temperature *
    int on;
    static int temperature = 0;
    if (setLedFromDevice(&x, &on) && /* If a local button was pressed *
        sendSetLED(cd, x, on))
    {
        return -1;
    }
    x = getTemp();
    return x != temperature ? temperature = x, sendSetTemp(cd, x) : 0;
}
*/
/****************************  Application: AJAX *****************************

/* All AJAX messages begin with: ["AJAX",[RPC-ID, .... *
static void
beginAjaxResp(SendData* sd, S32 ajaxHandle)
{
    beginMessage(sd, "AJAX");
    JEncoder_beginArray(&sd->encoder);
    JEncoder_setInt(&sd->encoder, ajaxHandle);
}


/* All AJAX messages end with: ...]] *
static int
endAjaxResp(SendData* sd)
{
    JEncoder_endArray(&sd->encoder);
    return endMessage(sd);
}


/* Send ["AJAX",[RPC-ID, {"err": emsg}]]
 *
static int
sendAjaxErr(ConnData* cd, S32 ajaxHandle, const char* emsg)
{
    SendData sd;
    SendData_constructor(&sd, cd);
    beginAjaxResp(&sd, ajaxHandle);
    JEncoder_set(&sd.encoder, "{s}", "err", emsg);
    return endAjaxResp(&sd);
}



/* Manages the following AJAX requests:
   math/add
   math/subtract
   math/mul
   math/div
 *
static int
math_xx(ConnData* cd, const char* op, JErr* e, S32 ajaxHandle, JVal* v)
{
    double arg1, arg2, resp;
    SendData sd;
    JVal_get(v, e, "[ff]", &arg1, &arg2);
    if (JErr_isError(e))
        return sendAjaxErr(cd, ajaxHandle, "math/: invalid args");
    switch (op[0]) /* Use first letter of 'add', 'subtract', 'mul', and 'div' *
    {
    case 'a': resp = arg1 + arg2; break;
    case 's': resp = arg1 - arg2; break;
    case 'm': resp = arg1 * arg2; break;
    case 'd':
        if (arg2 == 0)
            return sendAjaxErr(cd, ajaxHandle, "Divide by zero!");
        resp = arg1 / arg2;
        break;
    default:
        return sendAjaxErr(cd, ajaxHandle, "Unknown math operation");
    }
    SendData_constructor(&sd, cd);
    beginAjaxResp(&sd, ajaxHandle);
    JEncoder_set(&sd.encoder, "{f}", "rsp", resp);
    return endAjaxResp(&sd);
}


static int
auth_setcredentials(ConnData* cd, JErr* e, S32 ajaxHandle, JVal* v)
{
    SendData sd;
    SharkSslSha1Ctx ctx;
    U8 digest[20];
    const char* curUname;
    const char* curPwd;
    const char* newUname;
    const char* newPwd;
    JVal_get(v, e, "[ssss]", &curUname, &curPwd, &newUname, &newPwd);
    if (JErr_isError(e))
        return sendAjaxErr(cd, ajaxHandle, "auth/setcredentials: invalid args");
    SharkSslSha1Ctx_constructor(&ctx);
    SharkSslSha1Ctx_append(&ctx, (U8*)curPwd, strlen(curPwd));
    SharkSslSha1Ctx_finish(&ctx, digest);
    SendData_constructor(&sd, cd);
    if (checkCredentials(curUname, 0, digest))
    {
        return sendAjaxErr(cd, ajaxHandle,
            "Please provide correct current credentials!");
    }
    if (setcredentials(newUname, newPwd))
    {
        return sendAjaxErr(cd, ajaxHandle,
            "Saving credentials failed!");
    }

    beginAjaxResp(&sd, ajaxHandle);
    JEncoder_set(&sd.encoder, "{b}", "rsp", TRUE);
    return endAjaxResp(&sd);
}


static int
ajax(ConnData* cd, JErr* e, JVal* v)
{
    const char* service; /* The AJAX service name *
    S32 ajaxHandle;
    JVal* vPayload;

    JVal_get(v, e, "[sdJ]", &service, &ajaxHandle, &vPayload);
    if (JErr_isError(e))
    {  /* This error would indicate a problem with the code in connection.js *
        xprintf(("AJAX semantic err: %s\n", e->msg));
        return -1;
    }
    if (!strncmp("math/", service, 5))
        return math_xx(cd, service + 5, e, ajaxHandle, vPayload);
    if (!strcmp("auth/setcredentials", service))
        return auth_setcredentials(cd, e, ajaxHandle, vPayload);
    return sendAjaxErr(cd, ajaxHandle, "Service not found!");
}

*/
/******************************  RecData ************************************/

/*
  The binary message types sent from the browser to the server.
  See the JavaScript equivalent in WebSocketCon.js
*/
typedef enum {
    BinMsg_Upload = 1,
    BinMsg_UploadEOF
} BinMsg;



/*
  Construct the RecData object used when receiving JSON messages and
  sending response data.

  Notice how we use the static allocators if USE_STATIC_ALLOC is
  set. See JsonStaticAlloc.c for details.

  https://realtimelogic.com/ba/doc/en/C/reference/html/structJParserValFact.html
  https://realtimelogic.com/ba/doc/en/C/reference/html/structJParser.html
 */
static void
RecData_constructor(RecData* o)
{
    memset(o, 0, sizeof(RecData));
#ifdef USE_STATIC_ALLOC
    {
        JParserAlloc_constructor(&jpa);
        VAlloc_constructor(&va);
        DAlloc_constructor(&da);
        JParserValFact_constructor(&o->pv, &va, &da);
        JParser_constructor(&o->parser, (JParserIntf*)&o->pv, o->maxMembN,
            sizeof(o->maxMembN), &jpa, 0);
    }
#else
    /* Use dynamic allocation */
    JParserValFact_constructor(&o->pv, AllocatorIntf_getDefault(),
        AllocatorIntf_getDefault());
    JParser_constructor(&o->parser, (JParserIntf*)&o->pv, o->maxMembN,
        sizeof(o->maxMembN), AllocatorIntf_getDefault(), 0);
#endif
}

/* Release memory used by JParserValFact (Reset memory buffers if
 * USE_STATIC_ALLOC is defined).
 */
#define RecData_reset(o) JParserValFact_termFirstVal(&(o)->pv);


 /* Send the one time nonce key to the browser.
   ["nonce", "the-12-byte-nonce-encoded-as-b64"]
  *
static int
RecData_sendNonce(RecData* o, ConnData* cd)
{
    SendData sd;
    {
        int i;
        for (i = 0; i < sizeof(o->nonce); i++)
            o->nonce[i] = (U8)rand() % 0xFF;
    }
    SendData_constructor(&sd, cd);
    beginMessage(&sd, "nonce");
    /* Send the 12 byte binary nonce B64 encoded *
    JEncoder_b64enc(&sd.encoder, o->nonce, sizeof(o->nonce));
    return endMessage(&sd);
}*/


/* We received an 'auth' message from the browser.
   Complete message: ["auth", {name:"string",hash:"string"}]
   Value in argument 'v' is: {name:"string",hash:"string"}
*
static int
RecData_authenticate(RecData* o, ConnData* cd, JVal* v, JErr* e)
{
    const char* name; /* username *
    const char* hash; /* 40 byte SHA-1 password hash in hex representation *
    int i;
    U8 digest[20]; /* Convert and store hash in this buffer *
    JVal_get(v, e, "{ss}", "name", &name, "hash", &hash);
    if (JErr_isError(e)) return -1;

    /* A SHA-1 digest is 20 bytes. Convert the 40 byte hex value
     * received from the browser to a 20 byte binary representation.
     *
    for (i = 0; i < 20; i++)
    {
        int j;
        U8 hex = 0;
        const U8* ptr = (U8*)hash + 2 * i;
        for (j = 0; j < 2; j++)
        { /* Convert each hex value to a byte value. Assume hex is all
           * lower case letters.
           *
            U8 c = *ptr++;
            if (c >= '0' && c <= '9') c -= '0'; /* 0..9 *
            else c = c - 'a' + 10; /* 10..15 *
            hex = (hex << 4) | c;
        }
        digest[i] = hex;
    }
    /* Check if digest matches locally stored data *
    if (!checkCredentials(name, o->nonce, digest))
    {
        o->authenticated = TRUE;
        /*
          Send the initial LED info message if user provided correct
          credentials. We also send the temperature so the thermostat
          shows the correct temperature.
        *
        return sendLedInfo(cd) || sendSetTemp(cd, getTemp()) ? -1 : 0;
    }
    /* Not authenticated *
    return RecData_sendNonce(o, cd);
}
*/
/* Manage binary SMQ and WebSocket frames sent by browser.
   Note that a complete frame may be longer than the data we
   receive. The argument eom (end of message) is true when all data in
   the frame has been consumed. We implement rudimentary state
   management for a frame that is split up into multiple chunks by
   storing the binary message type in RecData:binMsg
*
static int
RecData_manageBinFrame(RecData* o, ConnData* cd, U8* data, int len, BaBool eom)
{
    int status = -1;
    if (o->binMsg == 0) /* Not set if first chunk in a frame *
    {
        o->binMsg = data[0]; /* Save binary message type (first byte in frame) *
        data++; /* Set pointer to start of payload *
        len--;
        if (o->messages == 0)
        {
            if (saveFirmware(0, 0, TRUE, FALSE))
                return -1;
        }
    }
    switch (o->binMsg)
    {
    case BinMsg_Upload:
        status = saveFirmware(data, len, FALSE, FALSE);
        if (!status && eom && ++o->messages % 10 == 0)
            status = sendUploadAck(cd, o->messages);
        break;
    case BinMsg_UploadEOF:
        ++o->messages;
        status = saveFirmware(data, len, FALSE, eom);
        if (!status)
            status = sendUploadAck(cd, o->messages);
        o->messages = 0;
        break;
    default:
        xprintf(("Received unknown binary message: %u\n",
            (unsigned)data[0]));
    }
    if (eom)
        o->binMsg = 0; /* Reset *
    if (status)
        saveFirmware(0, 0, FALSE, TRUE);
    return status;
}
*/


/*
  Manages JSON async messages sent by browser, except for 'auth',
  which is handled in RecData_openMessage. The user is authenticated
  if this function is called.

static int
RecData_manageMessage(RecData* o, ConnData* cd, const char* msg, JErr* e, JVal* v)
{
    switch (*msg)
    {
    case 'A': /* AJAX is encapsulated as an async message *
        if (!strcmp("AJAX", msg))
            return ajax(cd, e, v);
        goto L_unknown;

    case 's':
        if (!strcmp("setled", msg))
            return manageSetLED(cd, e, v);
        goto L_unknown;

    default:
    L_unknown:
        xprintf(("Received unknown message: %s\n", msg));
    }
    return -1;
}*/


/* All messages have the form ["message-name", 'message-body']
   This function extracts the message name and deals with the initial
   state when we are not authenticated.
*/
static int
RecData_openMessage(RecData* o, ConnData* cd)
{
    JErr e;
    JErr_constructor(&e);
    JVal *data;
    const char* op;

//    v = JVal_getObject(, &e);
//    if (JErr_isError(&e))
//        return -1;
    JVal_get(JParserValFact_getFirstVal(&o->pv), &e, "{sJ}", "op", &op, "data", &data);
    if (JErr_isError(&e))
        return -1;
    if (cd->manageMessage(o, cd, op, &e, data)) //minnowManageMessage(o, cd, message, &e, v))
        return -1;
    
    RecData_reset(o);
    return 0;

    /* The JSON message name *
    /*  A.1: Get JSON message root obj
        A.2: Get the first element in the array, which should be the
             JSON message name
        B.1: Extract the JSON message name
        B.2: Advance to next element, which should be the message body
    *
    v = JVal_getArray(JParserValFact_getFirstVal(&o->pv), &e);
    if ((message = JVal_getString(v, &e)) != 0 && (v = JVal_getNextElem(v)) != 0)
    {
        /* Message OK so far, now let's look at the message content *
        if (!o->authenticated)
        { /* The only message accepted when not authenticated is 'auth' *
            if (strcmp(message, "auth") || RecData_authenticate(o, cd, v, &e))
                goto L_semantic;
            /* Else msg OK. *
        }
        else*/
  /*      {
            if (cd->manageMessage(o, cd, message, &e, v)) //minnowManageMessage(o, cd, message, &e, v))
                return -1;
        }
        RecData_reset(o);
        return 0;
    }
    else
    {
    L_semantic:
        xprintf(("Semantic JSON message error\n"));
    }*/
    
    return -1;
}


/*
  Parse the JSON 'data' with 'len' bytes received either from a
  WebSocket connection or SMQ.  A very large message may be in a
  WebSocket frame that is too large for the Minnow Server receive
  buffer (or SMQ buffer). The Minnow server (and SMQ) include state
  information telling the application about these conditions. The
  argument 'eom' (end of message) is false as long as we receive
  chunks that is not the last chunk. We do not use large JSON messages
  thus eom should always be TRUE, however, this code is designed to
  manage receiving chunks. Note that the code uses the parse state and
  'eom' for integrity validation.
*/
static int
RecData_parse(RecData* o, ConnData* cd, U8* data, int len, BaBool eom)
{
    int status;

#ifdef USE_STATIC_ALLOC
    /* For each new JSON message received, reset the internal static buffer
     * index pointer for the basic allocators.
     */
    JParserAlloc_reset(&jpa);
    VAlloc_reset(&va);
    DAlloc_reset(&da);
#endif
    status = JParser_parse(&o->parser, data, len);
    if (status)
    {
        if (status < 0 || !eom)
        {
            xprintf(("JSON: %s\n", status < 0 ? "parse error" : "expected end of MSG"));
            return -1;
        }
        /* Got a JSON message */
        return RecData_openMessage(o, cd);
    }
    else if (eom)
    {
        xprintf(("Expected more JSON"));
    }
    return 0; /* OK, but need more data */
}


/*
  This function gets called when the Minnow Server's listen socket is
  activated i.e. when socket 'accept' returns a new socket.
  https://realtimelogic.com/ba/doc/en/C/shark/structMS.html
*/
static bool
RecData_runServer(RecData* rd, ConnData* cd, WssProtocolHandshake* wph)
{
    MS* ms = cd->u.ms;
    /* MS_webServer: Manage HTTP GET or upgrade WebSocket request */
    if (!MS_webServer(ms, wph))// && !sendDeviceName(cd))
    { /* We get here if HTTP(S) was upgraded to a WebSocket con. */
        int rc;
        U8* msg;
        /* We send the nonce to the browser so the user can
         * safely authenticate.
         */
        //RecData_sendNonce(rd, cd);
        while ((rc = MS_read(ms, &msg, 50)) >= 0)
        {
            if (rc) /* incomming data from browser */
            {
                if (ms->rs.frameHeader[0] == WSOP_Text)
                {  /* All text frames should contain JSON */
                    if (RecData_parse(
                        rd, cd, msg, rc, ms->rs.frameLen - ms->rs.bytesRead == 0))
                    {
                        break; /* err */
                    }
                }
                else /* Manage binary WebSocket frames */
                {
                    /*if (RecData_manageBinFrame(
                        rd, cd, msg, rc, ms->rs.frameLen - ms->rs.bytesRead == 0))*/
                    {
                        break; /* err */
                    }
                }
                //return true;
            }
            else /* timeout (Ref-D) */
            {
                //return true;
                //if (rd->authenticated && eventSimulator(cd))
                    break; /* on sock error */
            }
        }
        //rd->authenticated = FALSE;
        xprintf(("Closing WS connection: ecode = %d\n", rc));
        //return false;
    }

    return true;
}

/* Attempt to open the default server listening port 80 (secure mode
 * 443) or use an alternative port number if the default port is in
 * use.
 */
static int
openServerSock(SOCKET* sock)
{
    int status;
    U16 port;
    port = 80;
    status = se_bind(sock, port);
    if (status)
    {
        port = 9442;
        while (status == -3 && ++port < 9460)
            status = se_bind(sock, port);
    }
    if (!status)
    {
        xprintf(("WebSocket server listening on %d\n", (int)port));
    }
    return status;
}

void WPH_fetchPageSend(MST* mst, const char* header, const char* data)
{
    int sblen = MST_getSendBufSize(mst);
    int delta = sblen;
    int size = (int)strlen(data);
    U8* sptr = msRespCT(MST_getSendBufPtr(mst), &sblen, size, (const U8*)header);
    delta = delta - sblen;
    while (size)
    {
        if (sblen > size)
            sblen = size;
        memcpy(sptr, data, sblen);
        if (MST_write(mst, 0, sblen + delta) < 0)
            break;
        size -= sblen;
        data += sblen;
        delta = 0;
        sblen = MST_getSendBufSize(mst);
        sptr = MST_getSendBufPtr(mst);
    }
}

int WPH_fetchPage(void* hndl, MST* mst, U8* path)
{
    int result = 0;
    if (hndl)
    {
        FetchPage* fetch = (FetchPage*)hndl;
        result = fetch->operator()(mst, (const char*)path, WPH_fetchPageSend);
    }
    return result;

    /*
    static const U8 egz[] = { "\r\ncontent-type: text/html; charset=UTF-8\r\n" };
    const U8* dptr = "<html>winston</html>";
    *
    U8* egz;
    U8* dptr;
    U8* sptr;
    int result = 0;
    if (hndl)
    {
        FetchPage fetch = (FetchPage)hndl;
        result = fetch(path, &egz, &dptr);
    }

    if (result == 0)
        return 0;

    int sblen = MST_getSendBufSize(mst);
    int delta = sblen;
    int size = strlen(dptr);
    (void)hndl;
 //   if (path[0] != '/' || path[1]) /* if path is not "/" */
 //       return 0; /* not found */
    /*
    sptr = msRespCT(MST_getSendBufPtr(mst), &sblen, size, egz);
    delta = delta - sblen;
    while (size)
    {
        if (sblen > size)
            sblen = size;
        memcpy(sptr, dptr, sblen);
        if (MST_write(mst, 0, sblen + delta) < 0)
            break;
        size -= sblen;
        dptr += sblen;
        delta = 0;
        sblen = MST_getSendBufSize(mst);
        sptr = MST_getSendBufPtr(mst);
    }
    return 1;*/
}

void minnowStart(SOCKET* listenSockPtr, SOCKET* sockPtr, WssProtocolHandshake** wph, ConnData** cd, RecData** rd, MS** ms, ManageMessage manageMessage, FetchPage fetchPage)
{
    static WssProtocolHandshake _wph = { 0 };
    static ConnData _cd;
    static RecData _rd;
    static MS _ms; /* The Minnow Server */

    *wph = &_wph;
    *cd = &_cd;
    *rd = &_rd;
    *ms = &_ms;

    /*
    wph = { 0 };
    static ConnData cd;
    static RecData rd;
    static MS ms; /* The Minnow Server *

    static SOCKET listenSock;
    static SOCKET sock;
    SOCKET* listenSockPtr = &listenSock;
    SOCKET* sockPtr = &sock;

    (void)ctx; /* Not used */

    RecData_constructor(*rd);
    ConnData_setWS(*cd, *ms); /* Set default setup */
    MS_constructor(*ms);

#ifdef LWIP_RAW
    SeCtx* ctx = 0;
#else
    void *ctx = 0;
#endif
    SOCKET_constructor(listenSockPtr, ctx);
    SOCKET_constructor(sockPtr, ctx);

    (*cd)->manageMessage = manageMessage;

    (*wph)->fetchPage = WPH_fetchPage;
    (*wph)->fetchPageHndl = &fetchPage;
    if (openServerSock(listenSockPtr))
    {
        return;
    }
}

int minnowAccept(SOCKET* listenSockPtr, U32 timeout, SOCKET* sockPtr, MS* ms, WssProtocolHandshake* wph)
{
    auto accepted = se_accept(&listenSockPtr, timeout, &sockPtr);
    if (accepted == 1)
    {
        MS_setSocket(ms, sockPtr, msBuf.rec, sizeof(msBuf.rec), msBuf.send, sizeof(msBuf.send));
        if (!MS_webServer(ms, wph))
            return accepted;
        else
            return 0;
    }
    return 0;
}

bool minnowLoop(SOCKET* listenSockPtr, SOCKET* sockPtr, WssProtocolHandshake *wph, ConnData *cd, RecData *rd, MS* ms)
{
    int rc;
    U8* msg;
    /* We send the nonce to the browser so the user can
     * safely authenticate.
     */
     //RecData_sendNonce(rd, cd);
    while ((rc = MS_read(ms, &msg, 50)) >= 0)
    {
        if (rc) /* incomming data from browser */
        {
            if (ms->rs.frameHeader[0] == WSOP_Text)
            {  /* All text frames should contain JSON */
                if (RecData_parse(
                    rd, cd, msg, rc, ms->rs.frameLen - ms->rs.bytesRead == 0))
                {
                    break; /* err */
                }
            }
            else /* Manage binary WebSocket frames */
            {
                /*if (RecData_manageBinFrame(
                    rd, cd, msg, rc, ms->rs.frameLen - ms->rs.bytesRead == 0))*/
                {
                    break; /* err */
                }
}
            //return true;
            }
        else /* timeout (Ref-D) */
        {
            return true;
            //if (rd->authenticated && eventSimulator(cd))
            break; /* on sock error */
        }
    }
    //rd->authenticated = FALSE;
    xprintf(("Closing WS connection: ecode = %d\n", rc));
    return false;

#if 0
    switch (se_accept(&listenSockPtr, 50, &sockPtr))
    {
    case 1: /* Accepted new client connection i.e. new browser conn. */
    {
        MS_setSocket(ms, sockPtr, msBuf.rec, sizeof(msBuf.rec),
            msBuf.send, sizeof(msBuf.send));
        RecData_runServer(rd, cd, wph);
        se_close(sockPtr);
        break;
    }
    case 0: /* se_accept 50ms timeout */
        break;

    default:
        /* We get here if 'accept' fails.
           This is probably where you reboot.
        */
        se_close(listenSockPtr);
        return false; /* Must do system reboot */
    }

    return true;
#endif
}

void minnowClose(SOCKET* listenSockPtr, SOCKET* sockPtr)
{
    se_close(listenSockPtr);
    se_close(sockPtr);
}

/*
  The main function initiates everything and opens a socket
  connection. When in secure mode (TLS), create a SharkSsl object and
  one SharkSslCon (connection) object.

  ctx: Bare Metal only (remove for other platforms), see:
  https://realtimelogic.com/ba/doc/en/C/shark/group__BareMetal.html
 *
void
mainTask(SeCtx* ctx)
{
    static WssProtocolHandshake wph = { 0 };
    static ConnData cd;
    static RecData rd;
    static MS ms; /* The Minnow Server *

    static SOCKET listenSock;
    static SOCKET sock;
    SOCKET* listenSockPtr = &listenSock;
    SOCKET* sockPtr = &sock;

    (void)ctx; /* Not used *

    RecData_constructor(&rd);
    ConnData_setWS(&cd, &ms); /* Set default setup *
    MS_constructor(&ms);

    SOCKET_constructor(listenSockPtr, ctx);
    SOCKET_constructor(sockPtr, ctx);

    wph.fetchPage = fetchPage;
    if (openServerSock(listenSockPtr))
    {
        return;
    }

    for (;;)
    {
        switch (se_accept(&listenSockPtr, 50, &sockPtr))
        {
        case 1: /* Accepted new client connection i.e. new browser conn. *
        {
            MS_setSocket(&ms, sockPtr, msBuf.rec, sizeof(msBuf.rec),
                msBuf.send, sizeof(msBuf.send));
            RecData_runServer(&rd, &cd, &wph);
            se_close(sockPtr);
        }
        break;

        case 0: /* se_accept 50ms timeout *
            break;

        default:
            /* We get here if 'accept' fails.
               This is probably where you reboot.
            *
            se_close(listenSockPtr);
            return; /* Must do system reboot *
        }
    }
}*/

SendData::SendData()
    : super(), err(), encoder(&err, &super)
{
}

RecData::RecData()
    : parser((JParserIntf*)&pv, this->maxMembN, sizeof(this->maxMembN), AllocatorIntf_getDefault(), 0), pv(AllocatorIntf_getDefault(), AllocatorIntf_getDefault())
{
    messages = 0;
    memset(maxMembN, 0, sizeof(maxMembN));
}
