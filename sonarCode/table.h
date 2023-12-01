/*
 * @Author: oyw 1622945822@qq.com
 * @Date: 2023-11-21 13:20:50
 * @LastEditors: oyw 1622945822@qq.com
 * @LastEditTime: 2023-11-24 22:27:13
 * @FilePath: \sonarCode\table.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
/*
 * @Author: oyw 1622945822@qq.com
 * @Date: 2023-11-21 13:20:50
 * @LastEditors: oyw 1622945822@qq.com
 * @LastEditTime: 2023-11-24 15:31:56
 * @FilePath: \sonarCode\table.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#ifndef _TABLE_H
#define _TABLE_H

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdlib.h>
#define OS_BUFFER_SIZE 10
/* 水的盐度
* 1 : 淡水
* 2 : 海水
*/
#define WATERENVI 1 
enum water{
  FREASH=1, 
  SEA
};

enum CaiTeamMessageType 
{
  messageSimpleFire         = 0x15,
  messagePingResult         = 0x22,
  messageSimplePingResult   = 0x23,
  messageUserConfig			    = 0x55,
  messageDummy              = 0xff,
};

/* 声纳的工作频率 */
enum workFre
{
  flexiMode, 
  lowFreMode,
  fastFreMode
};

enum DataSizeType 
{
  dataSize8Bit,
  dataSize16Bit,
  dataSize24Bit,
  dataSize32Bit,
};

enum PingRateType 
{/* 通过数据手册可以看到最大为40hz */
  pingRateNormal  = 0x00, // 10Hz max ping rate
  pingRateHigh    = 0x01, // 15Hz max ping rate
  pingRateHighest = 0x02, // 40Hz max ping rate
  pingRateLow     = 0x03, // 5Hz max ping rate
  pingRateLowest  = 0x04, // 2Hz max ping rate
  pingRateStandby = 0x05, // Disable ping
};
/* 通信的数据帧头 */
struct _CaiTeamMessageHeader
{
  uint16_t CaiTeamId;         // Fixed ID 0x4f53
  uint16_t srcDeviceId;      // The device id of the source
  uint16_t dstDeviceId;      // The device id of the destination
  uint16_t msgId;            // Message identifier
  uint16_t msgVersion;
  uint32_t payloadSize;      // The size of the message payload (header not included)
  uint16_t spare2;
};
/* 要发送给声纳的数据格式 */
struct _CaiTeamSimpleFireMessage
{
  struct _CaiTeamMessageHeader head; // The standard message header
  uint8_t masterMode;          // mode 0 is flexi mode, needs full fire message (not available for third party developers)
                                // mode 1 - Low Frequency Mode (wide aperture, navigation)
                                // mode 2 - High Frequency Mode (narrow aperture, target identification)
  enum PingRateType pingRate; // Sets the maximum ping rate.
  uint8_t networkSpeed;        // Used to reduce the network comms speed (useful for high latency shared links)
  uint8_t gammaCorrection;     // 0 and 0xff = gamma correction = 1.0  /* 用户不需要调整伽马控制，但经验丰富的用户可能希望在某些情况下通过调整默认值约0.6的伽马值来优化声纳图像。*/
                                // Set to 127 for gamma correction = 0.5
  uint8_t flags;               // bit 0: 0 = interpret range as percent, 1 = interpret range as meters
                                // bit 1: 0 = 8 bit data, 1 = 16 bit data
                                // bit 2: 0 = wont send gain, 1 = send gain
                                // bit 3: 0 = send full return message, 1 = send simple return message
  double range;                 // The range demand in percent or m depending on flags 随着距离的增加，声音到达最远的目标并返回所需的时间也会增加，因此声纳图像更新速度也会变慢
  double gainPercent;           // The gain demand 增益 越大声纳图越亮，我们旨在采集灰度图因此这一项没用
  double speedOfSound;          // ms-1, if set to zero then internal calc will apply using salinity
  double salinity;              // ppt, set to zero if we are in fresh water ；35（ ppt ）in Salt Water
};  
/* 这条信息没有发出 */
struct _CaiTeamSimpleFireMessage2
{
	struct _CaiTeamMessageHeader head;
	uint8_t masterMode;
	enum PingRateType pingRate;
	uint8_t networkSpeed; /* The max network speed in Mbs , set to 0x00 or 0xff to use link speed */
	uint8_t gammaCorrection; /* The gamma correction - 255 is equal to a gamma correction of 1.0 */
	uint8_t flags;
	double rangePercent; /* The range demand (%) */
	double gainPercent; /* The percentage gain */
	double speedOfSound; /* The speed of sound - set to zero to use internal calculations */
	double salinity; /* THe salinity to be used with internal speed of sound calculations (ppt) */
	uint32_t extFlags;
	uint32_t reserved[8];

} ;

struct _CaiTeamSimplePingResult
{

    struct _CaiTeamSimpleFireMessage fireMessage;
    uint32_t pingId; 			/* An incrementing number */
    uint32_t status;
    double frequency;				/* The acoustic frequency (Hz) */
    double temperature;				/* The external temperature (deg C) */
    double pressure;				/* The external pressure (bar) */
    double speeedOfSoundUsed;		/* The actual used speed of sound (m/s). May be different to the speed of sound set in the fire message */
    uint32_t pingStartTime;
    enum DataSizeType dataSize; 			/* The size of the individual data entries */ /* 单个数据线对应的距离 */
    double rangeResolution;			/* The range in metres corresponding to a single range line */
    uint16_t nRanges;			/* The number of range lines in the image*/ /* 图像中距离线的数量 */
    uint16_t nBeams;			/* The number of bearings in the image */ /* 图像中的方位数量 */
    uint32_t imageOffset; 		/* The offset in bytes of the image data from the start of the network message */
    uint32_t imageSize; 		/* The size in bytes of the image data */
    uint32_t messageSize; 		/* The total size in bytes of the network message */

} ;

struct _CaiTeamSimplePingResult2
{
	struct _CaiTeamSimpleFireMessage2 fireMessage;
	uint32_t pingId; 		/* An incrementing number */
	uint32_t status;
	double frequency;		/* The acoustic frequency (Hz) */
	double temperature;		/* The external temperature (deg C) */
	double pressure;			/* The external pressure (bar) */
	double heading;			/* The heading (degrees) */
	double pitch;			/* The pitch (degrees) */
	double roll;			/* The roll (degrees) */
	double speeedOfSoundUsed;	/* The actual used speed of sound (m/s) */
	double pingStartTime;		/* In seconds from sonar powerup (to microsecond resolution) */
	enum DataSizeType dataSize; 		/* The size of the individual data entries */
	double rangeResolution;		/* The range in metres corresponding to a single range line 单个距离线对应的距离解析度。 */
	uint16_t nRanges;		/* The number of range lines in the image*/
	uint16_t nBeams;		/* The number of bearings in the image */
	uint32_t spare0;
	uint32_t spare1;
	uint32_t spare2;
	uint32_t spare3;
	uint32_t imageOffset; 	/* The offset in bytes of the image data from the start */
	uint32_t imageSize; 		/* The size in bytes of the image data */
	uint32_t messageSize; 	/* The total size in bytes of the network message */
	//uint16_t bearings[OSS_MAX_BEAMS]; /* The brgs of the formed beams in 0.01 degree resolution */
} ;


struct _PingParameters
{
	uint32_t u0;
	uint32_t u1;
	double   d1;
	double   d2;
	uint32_t u2;
	uint32_t u3;

	double d3;
	double d4;
	double d5;
	double d6;
	double d7;
	double d8;
	double d9;
	double d10;
	double d11;
	double d12;
	double d13;
	double d14;
	double d15;
	double d16;
	double d17;
	double d18;
	double d19;
	double d20;

	uint32_t u4;
	uint32_t nRangeLinesBfm;
	uint16_t u5;
	uint16_t u6;
	uint16_t u7;
	uint32_t u8;
	uint32_t u9;
	uint8_t  b0;
	uint8_t  b1;
	uint8_t  b2;
	uint32_t imageOffset;              // The offset in bytes of the image data (CHN, CQI, BQI or BMG) from the start of the buffer
	uint32_t imageSize;                // The size in bytes of the image data (CHN, CQI, BQI or BMG)
	uint32_t messageSize;              // The total size in bytes of the network message
	// *** NOT ADDITIONAL VARIABLES BEYOND THIS POINT ***
	// There will be an array of bearings (shorts) found at the end of the message structure
	// Allocated at run time
	// short bearings[];
	// The bearings to each of the beams in 0.01 degree resolution 分辨率为0.01°
} ;
struct _PingConfig
{
	uint8_t       b0;
	double        d0;
	double        range;
	double        d2;
	double        d3;
	double        d4;
	double        d5;
	double        d6;
	uint16_t      nBeams;
	double        d7;
	uint8_t		    b1;
	uint8_t		    b2;
	uint8_t		    b3;
	uint8_t		    b4;
	uint8_t		    b5;
	uint8_t       b6;
	uint16_t      u0;
	uint8_t       b7;
	uint8_t       b8;
	uint8_t       b9;
	uint8_t       b10;
	uint8_t       b11;
	uint8_t       b12;
	uint8_t       b13;
	uint8_t       b14;
	uint8_t       b15;
	uint8_t       b16;
	uint16_t      u1;
} ;
/********************************************** 1 ping 的参数 ****************************************************/
typedef struct
{
  uint8_t b0;
  double d0;
  uint16_t u0;
  uint16_t u1;
} s0;


typedef struct
{
  uint8_t b0;
} s2;

typedef struct
{
  uint8_t b0;
  uint8_t b1;
} s7;

typedef struct
{
  int i0;
  int i1;
  int i2;
  int i3;
  int i4;
  int i5;
} s9;

typedef struct
{
  uint8_t b0;
  double d0;
  double d1;
} s8;

typedef struct
{
  uint8_t  b0;
  uint16_t u0;
  uint8_t  b1;
  double   d0;
} s1;

typedef struct
{
  uint8_t       b0;
  uint8_t        b1;
  uint8_t      b2;
  uint8_t       b3;
  uint8_t            b4;
  uint8_t b5;
  uint8_t      b6;
  uint8_t   b7;
  uint8_t        b8;
  uint8_t      b9;
  uint8_t       b10;
  uint8_t            b11;
  uint8_t    b12;
  uint8_t            b13;
  uint8_t            b14;
  uint8_t            b15;
  uint16_t           u0;
  uint8_t    b16;
  double             d0;
  double             d1;
} s3;

typedef struct
{
  uint8_t  b0;
  uint8_t  b1;
  double   d0;
  double   d1;
  double   d2;
} s4;

typedef struct
{

  uint8_t  b0;
  uint8_t  b1;
  uint16_t u0;
  uint16_t u1;
  uint16_t u2;
  uint16_t u3;
  uint16_t u4;
  uint16_t u5;
} s5;

typedef struct
{
  double d0;
  double d1;
} s6;

typedef struct
{
  int i0;
  int i1;
  int i2;
  int i3;
} s10;

typedef struct
{
  double d0;
  double d1;
  double d2;
  double d3;
  double d4;
} s11;

typedef struct
{
  double d0;
  double d1;
  double d2;
  double d3;
  double d4;
  double d5;
  double d6;
} s12;

struct _CaiTeamReturnFireMessage
{
	struct _CaiTeamMessageHeader head;
	struct _PingConfig          ping;
	s0					t0;
	s1					t1;
	s2          t2;
	s3       t3;
	s4         t4;
	s5          t5;
	s6        t6;
	s7         t7;
	s8   t8;
	s9        t9;
	s10         t10;
	s11           t11;
	s12          t12;
	struct _PingParameters      ping_params;
} ;

struct _OsBufferEntry
{
  // Data
  struct _CaiTeamSimplePingResult  m_rfm;  // The fixed length return fire message
  struct _CaiTeamReturnFireMessage m_rff;
  struct _CaiTeamSimplePingResult2 m_rfm2;	
  u_char*                 m_pImage;      // The image data
  short*                  m_pBrgs;       // The bearing table /* 方位表 */
  uint16_t				        m_version;
  uint8_t*                m_pRaw;        // The raw data
  uint32_t                m_rawSize;     // Size of the raw data record
  bool					  m_simple;
};
struct _dataBuffer{
  // The raw receive buffer
  char*           m_pRxBuffer; // The rx buffer for incomming data
  int32_t         m_nRxMax;    // The maximum size of the rx Buffer
  int64_t         m_nRxIn;     // The current amount of unprocessed data in the buffer
  // The raw send buffer
  char*           m_pToSend;
  int64_t         m_nToSend;
  // buffer 
  int32_t         m_nFlushes;  /* 刷新缓冲区的次数 */ 
  // The recieve buffer for messages
  struct _OsBufferEntry m_osBuffer[OS_BUFFER_SIZE];
  unsigned        m_osInject;   // The position for the next inject
};




extern struct _dataBuffer dataBuffer ;
extern struct _OsBufferEntry OsBufferEntry ;

#endif
