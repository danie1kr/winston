/* 
	Editor: https://www.visualmicro.com/
			This file is for intellisense purpose only.
			Visual micro (and the arduino ide) ignore this code during compilation. This code is automatically maintained by visualmicro, manual changes to this file will be overwritten
			The contents of the _vm sub folder can be deleted prior to publishing a project
			All non-arduino files created by visual micro and all visual studio project or solution files can be freely deleted and are not required to compile a sketch (do not delete your own code!).
			Note: debugger breakpoints are stored in '.sln' or '.asln' files, knowledge of last uploaded breakpoints is stored in the upload.vmps.xml file. Both files are required to continue a previous debug session without needing to compile and upload again
	
	Hardware: LOLIN(WEMOS) D1 mini Lite                                                                                              (esp8266_d1_mini_lite), Platform=esp8266, Package=esp8266
*/

#if defined(_VMICRO_INTELLISENSE)

#ifndef _VSARDUINO_H_
#define _VSARDUINO_H_
#define __ESP8266_esp8266__ 1
#define __ESP8266_ESP8266__ 1
#define _VMDEBUG 1
#define __ets__ 1
#define ICACHE_FLASH 1
#define _GNU_SOURCE 1
#define MMU_IRAM_SIZE 0x8000
#define MMU_ICACHE_SIZE 0x8000
#define NONOSDK22x_190703 1
#define F_CPU 80000000L
#define LWIP_OPEN_SRC 1
#define TCP_MSS 536
#define LWIP_FEATURES 1
#define LWIP_IPV6 0
#define ARDUINO 108019
#define ARDUINO_ESP8266_WEMOS_D1MINILITE 1
#define ARDUINO_ARCH_ESP8266 1
#define ARDUINO_BOARD "ESP8266_WEMOS_D1MINILITE"
#define FLASHMODE_DOUT 1
#define ESP8266 1
#define __cplusplus 201103L
#undef __cplusplus
#define __cplusplus 201103L
#define __STDC__
#define __ARM__
#define __arm__
#define __inline__
#define __asm__(x)
#define __asm__
#define __extension__
#define __ATTR_PURE__
#define __ATTR_CONST__
#define __volatile__


#define __ASM
#define __INLINE
#define __attribute__(noinline)

//#define _STD_BEGIN
//#define EMIT
#define WARNING
#define _Lockit
#define __CLR_OR_THIS_CALL
#define C4005
#define _NEW

//typedef int uint8_t;
//#define __ARMCC_VERSION 400678
//#define PROGMEM
//#define string_literal
//
//#define prog_void
//#define PGM_VOID_P int
//

typedef int _read;
typedef int _seek;
typedef int _write;
typedef int _close;
typedef int __cleanup;

//#define inline 

#define __builtin_clz
#define __builtin_clzl
#define __builtin_clzll
#define __builtin_labs
#define __builtin_va_list
typedef int __gnuc_va_list;

#define __ATOMIC_ACQ_REL

#define __CHAR_BIT__
#define _EXFUN()

typedef unsigned char byte;
extern "C" void __cxa_pure_virtual() {;}


typedef long __INTPTR_TYPE__ ;
typedef long __UINTPTR_TYPE__ ;
typedef long __SIZE_TYPE__ 	;
typedef long __PTRDIFF_TYPE__;

// Additions needed for v3.0.0 Core - Needs to be conditional on it being this core really!!
#ifndef isnan
#undef _Lockit
#undef __STDC__
#define __STDC__ 1
#define __CHAR_BIT__ 1
extern int isinf(double);
extern int isnan(double);
extern int fpclassify(double);
extern int signbit(double);
extern int isfinite(double);
extern int isnormal(double);
extern int isgreater(double, double);
extern int isgreaterequal(double);
extern int isless(double, double);
extern int islessequal(double, double);
extern int islessgreater(double, double);
extern int isunordered(double, double);
#endif

#include "new"
#include "Esp.h"


#include <arduino.h>
#include <pins_arduino.h> 

#include "..\generic\Common.h"
#include "..\generic\pins_arduino.h"

#undef F
#define F(string_literal) ((const PROGMEM char *)(string_literal))
#undef PSTR
#define PSTR(string_literal) ((const PROGMEM char *)(string_literal))
//current vc++ does not understand this syntax so use older arduino example for intellisense
//todo:move to the new clang/gcc project types.
#define interrupts() sei()
#define noInterrupts() cli()

#include "winston-esp-signal.ino"
#endif
#endif
