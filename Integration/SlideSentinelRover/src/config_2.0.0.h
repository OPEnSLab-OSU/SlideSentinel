#pragma once

/****** ComController ******/
#define CLIENT_ADDR 1         	// Node_ID
#define SERVER_ADDR 2
#define RADIO_BAUD 115200
#define INIT_TIMEOUT 2000      	// seconds
#define INIT_RETRIES 3        	// Default
#define RST 6
#define CD 10
#define IS_Z9C true
#define SPDT_SEL 14
#define FORCEOFF_N A5

/****** PMController ******/
#define VCC2_EN 13
#define MAX_CS 9
#define BAT 15

/****** FSController ******/
#define SD_CS 18  				// A4
#define SD_RST 16 				// A2

/****** IMUController ******/
#define INIT_SENSITIVITY 0x1F  	// uint8_t
#define ACCEL_INT A3

/****** GNSSController ******/
#define GNSS_BAUD 115200
#define INIT_LOG_FREQ 50000     // useconds
#define GNSS_TX 11
#define GNSS_RX 12

/****** RTCController ******/
#define RTC_INT 5
#define INIT_WAKETIME 2         // minutes
#define INIT_SLEEPTIME 2       	// minutes
