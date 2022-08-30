#pragma once

#define WINSTON_SIGNAL_LIGHT_DIV 4

//#define WINSTON_NFC_DETECTORS

//#define WINSTON_STATISTICS
//#define WINSTON_STATISTICS_DETAILLED
//#define WINSTON_STATISTICS_SECONDS_PER_PRINT	(5)

//#define WINSTON_REALWORLD
#ifndef WINSTON_REALWORLD
#define WINSTON_RAILWAY_DEBUG_INJECTOR
#endif

#define WINSTON_HAS_CHRONO

#ifdef WINSTON_PLATFORM_TEENSY
//#define WINSTON_TEENSY_FLASHSTRING
//#define WINSTON_TEENSY_SPI_DEBUG
#endif

#define WINSTON_ETHERNET_IP			IPAddress(192, 168, 188, 63)
#define WINSTON_ETHERNET_DNS		IPAddress(192, 168, 188, 1)
#define WINSTON_ETHERNET_GATEWAY	IPAddress(192, 168, 188, 1)
#define WINSTON_ETHERNET_SUBNET		IPAddress(255, 255, 255, 0)

#define WINSTON_WITH_WEBSOCKET

#define WINSTON_LOCO_TRACKING
#define WINSTON_LOCO_POSITION_TRACK_RATE				 40 //ms => 25hz (1000U/40U)		// 25hz
#define WINSTON_LOCO_SPEED_TRACK_RATE					100 //ms => 10hz (1000U/100U)	// 10hz
#define WINSTON_LOCO_UPDATE_POSITION_WEBSOCKET_RATE	   1000 //ms =>  1hz //(1000U/250U)	//  4hz	

/*
in case teensy does not start up:
	update fnet config:
		#define FNET_CFG_BENCH_CLN (0) // kaukerdl (1)  //Benchmark
		#define FNET_CFG_BENCH_CLN_BUFFER_SIZE          ((0) // kaukerdl 64*1024)
		#define FNET_CFG_SOCKET_TCP_TX_BUF_SIZE     (32U * 1024U)
		#define FNET_CFG_SOCKET_TCP_RX_BUF_SIZE     (32U * 1024U)
		#define FNET_CFG_BENCH_SRV (0) // kaukerdl (1)  //Benchmark
		#define FNET_CFG_BENCH_SRV_BUFFER_SIZE          (0) // kaukerdl (128*1024)
	remove everything not needed

	avoid PROGMEM routines
*/