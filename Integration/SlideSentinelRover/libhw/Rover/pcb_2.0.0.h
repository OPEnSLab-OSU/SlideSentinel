#pragma once

/****** ComController ******/
#define RST 6
#define CD 10
#define SPDT_SEL 14
#define FORCEOFF_N A5

/****** PMController ******/
#define VCC2_EN 13
#define MAX_CS 9
#define BAT_PIN 15

/****** FSController ******/
#define SD_CS 18  				// A4
#define SD_RST 16 				// A2

/****** IMUController ******/
#define INIT_SENSITIVITY 0x10  	// uint8_t
#define ACCEL_INT A3

/****** GNSSController ******/
#define GNSS_BAUD 115200
#define INIT_LOG_FREQ 30000     // useconds
#define GNSS_TX 11
#define GNSS_RX 12

/****** RTCController ******/
#define RTC_INT 5
#define INIT_WAKETIME 3         // minutes
#define INIT_SLEEPTIME 2       	// minutes