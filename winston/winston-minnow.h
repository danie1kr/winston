

/* 'MS_SEC' gets automatically defined in 'MSLib.h' when compiled with
   a SharkSSL delivery. The code excludes TLS if MS_SEC is not defined.
*/
#include <MSLib.h>
#include <functional>


/* JSON and static allocator:

  The JSON parser and the JVal node factory require a memory
  allocator, but you do not need to use a standard dynamic memory
  allocator. In the following which is enabled by default, three
  allocators are used that is internally using some very small static
  buffers. This type of allocation works great for small
  microcontrollers with little memory since we do not run into
  fragmentation issues and we have better use of the memory. See the
  file JsonStaticAlloc.c for more on how the allocators work.

  Remove '#define USE_STATIC_ALLOC' if you want to use standard
  dynamic allocation. You can still control what dynamic allocator to
  use by setting the macros baMalloc, baRealloc, and beFree.
 */
 //#define USE_STATIC_ALLOC
#ifdef USE_STATIC_ALLOC
#include "JsonStaticAlloc.h"
static AllocatorIntf jpa; /* JParser Alloc */
static AllocatorIntf va; /* JVal Node Allocator */
static AllocatorIntf da; /* JVal String Allocator */
#endif


/* JSON lib */
#include <JParser.h>
#include <JEncoder.h> 

#include <stdio.h>
#include <stdlib.h>


#define WinstonMinnowBufferSize 1500

#ifndef MS_SEC

  /* We need a send and receive buffer for the Minnow Server (MS) when
     using the standard (non secure version). The SharkSSL send/receive
     buffers are used in secure mode.
   */
struct {
    U8 rec[WinstonMinnowBufferSize];
    U8 send[WinstonMinnowBufferSize];
} msBuf;
#endif


/* Fetch the SPA. See index.c for details. */
extern int fetchPage(void* hndl, MST* mst, U8* path);

/****************************************************************************
 **************************----------------------****************************
 **************************| GENERIC CODE BELOW |****************************
 **************************----------------------****************************
 ****************************************************************************/

 /******************************  SendData ************************************/

//typedef int (*ManageMessage)(struct RecData* o, struct ConnData* cd, const char* msg, JErr* e, JVal* v);
//typedef void (*FetchPageSend)(MST* mst, const char* header, const char* data);
//typedef int (*FetchPage)(MST* mst, const char* path, FetchPageSend send);


using ManageMessage = std::function<int(struct RecData* o, struct ConnData* cd, const char* msg, JErr* e, JVal* v)>;
using FetchPageSend = std::function<void(MST* mst, const char* header, const char* data)>;
using FetchPage = std::function<int(MST* mst, const char* path, FetchPageSend send)>;

 /* Connection Data: this container object stores either a Minnow
    Server or an SMQ connection.
  */
typedef struct ConnData {
    union { /* union: Minnow or SMQ */
        MS* ms; /* MS: Minnow Server */
    } u; /* Union: MS or SMQ */
    ManageMessage manageMessage;
} ConnData;

#define ConnData_setWS(o,_ms) (o)->u.ms=_ms
#define ConnData_WebSocketMode(o) TRUE


/*   This container object stores data objects used when encoding/sending JSON.
     See function SendData_wsSendJSON for an explanation on how the
     objects are used.
 */
typedef struct SendData {
    BufPrint super; /* Ref-bp: Used as super class. Buffer needed by JEncoder */
    JErr err;
    JEncoder encoder;
    SendData();
    BaBool committed; /* Send: If a complete JSON message assembled */
} SendData;


/*

  This container object stores data objects used when receiving JSON
  messages and sending response data.
  JParser:
    https://realtimelogic.com/ba/doc/en/C/reference/html/structJParser.html
 */
typedef struct RecData {
    JParser parser; /* JSON parser */
    JParserValFact pv; /* JSON Parser Value (JVal) Factory */
    RecData();
    S32 messages; /* Used for counting messages received from browser */
//    BaBool authenticated; /* Set to true if the user is authenticated */
    char maxMembN[12]; /* Buffer for holding temporary JSON member name */
    /* nonce: One time key created and sent to the browser as part of
       logic for preventing relay attacks */
//    U8 nonce[12];
//    U8 binMsg; /* Holds the binary message type 'BinMsg' (enum BinMsg) */
} RecData;
/*
#ifdef __cplusplus
#define EXTERNC extern "C"
#else
#define EXTERNC
#endif*/
//EXTERNC int minnowManageMessage(RecData* o, ConnData* cd, const char* msg, JErr* e, JVal* v);

void minnowSendPrepare(ConnData* cd, SendData* sd, const char* msg);
void minnowSendFill(SendData* sd, const char* msg, const char* fmt, ...);
int minnowSendSubmit(SendData* sd);
void minnowStart(SOCKET* listenSockPtr, SOCKET* sockPtr, WssProtocolHandshake** wph, ConnData** cd, RecData** rd, MS** ms, ManageMessage manageMessage, FetchPage fetchPage);
int minnowAccept(SOCKET* listenSockPtr, U32 timeout, SOCKET* sockPtr, MS* ms, WssProtocolHandshake* wph);
bool minnowLoop(SOCKET* listenSockPtr, SOCKET* sockPtr, WssProtocolHandshake* wph, ConnData* cd, RecData* rd, MS* ms);
void minnowClose(SOCKET* listenSockPtr, SOCKET* sockPtr);