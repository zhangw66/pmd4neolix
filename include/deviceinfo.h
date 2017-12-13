#ifndef DEVICEINFO_H
#define DEVICEINFO_H
#define MY_DEBUG_LEVEL  1
#define LOG(msg...)\
    if(MY_DEBUG_LEVEL > 0)\
    {\
        printf("#DEBUG:%s\n @%s-%d #",__FILE__,__FUNCTION__,__LINE__);\
        printf(msg);\
    }

/*
 *　VSC Bulk命令传输(兼容PcoFlexx)：
 *　命令包字段：uint32_t uiCmd | 自定义段[packet_size - sizeof(uint32_t)]，一次命令包长度是usb block size.
 *　Host 发送命令包到Device；Device解析命令包，根据不同类型命令解析；
 *　Case 需要向Host返回数据，device准备数据并向Host发送，Device发送完成数据后，再此发送ACK包。
 *　Case Device不需要向Host返回数据，Device解析完命令后，直接返回ACK
 *　ACK: uint32_t uiCmd | uint32_t Status
 *　！！×　设备端,需要将命令取低１６bits，为兼容各设备
 */
//default is no read back data,every command must have a ACK command.
#define  READ_BACK   (1<<16)
#define  CHUNK_READ_BACK  (1<<17)
#define  CHUNK_WRITE_HEADER (1<<15)
//default write mode: command len is less than cmd block size
#define  CHUNK_WRITE			(1<<19)

#define MAX_NUM_DEVICES   4

//火星一
#define MARS01_VID  0x1c28
#define MARS01_PID  0xc012

//火星二
#define MARS02_VID  0x040E //S_MCCI
#define MARS02_PID  0xF410

//火星四
#define MARS04_VID  0x040E
#define MARS04_PID	0x4D35
//散斑结构光
#define SPECKLE_VID  0x040E //S_MCCI
#define SPECKLE_PID  0xF420
//默认命令包最小长度
#define TRNS_LEN     1024
#define CMD_TRNS_LEN     512

enum UsbEndpoint
{
   CONTROL_OUT = 0x01, //CONTROL_OUT = 0x02,sudo
   CONTROL_IN = 0x81, //CONTROL_IN = 0x82,
    /** Unidirectional communication stream containing the depthmap data */
    DEPTH_IN = 0x82,//DEPTH_IN = 0x81,
    /** Unidirectional communication stream containing the rgb image data */
    VIDEO_IN = 0x83,
};
enum DeviceCommand
{
    // READ_FLASH              =   CHUNK_READ_BACK | 1,
    // ERASE_FLASH             =  2,
    // WRITE_FLASH             = CHUNK_WRITE | 3,
    // WRITE_I2C               = 7,
    // READ_I2C                = READ_BACK | 8,
    /** This maps to a single 32-bit number in the protocol, with no extra data */
    START_IMAGER            = 10,
    /** This maps to a single 32-bit number in the protocol, with no extra data */
    STOP_IMAGER             = 11,
    RESET_IMAGER            = 12,
    SET_MODE                   = READ_BACK | 17,
    SET_FPS                        = READ_BACK | 18,         /* Command: Trigger a temperature measurement */
    SET_EXP                        = READ_BACK | 19,         /* Command: Fetch the temperature value */
    GET_LENSPARA            =  READ_BACK | 21,         /* Command: get the lens parameter */
    GET_STATE                    = READ_BACK  | 23,
    STOP_CAMERA             = 24,
    GET_TEMPERATURE             = READ_BACK | 25,
    //以下私有命令
    FETCH_INFO				=  READ_BACK | 80,     /*获取设备信息  */
    UPDATE_CALIBRATION =  CHUNK_WRITE_HEADER | 81, /*升级标定数据*/
    UPDATE_APP	 = READ_BACK | 82, /*升级应用程序*/
    CMD_RGB_SWITCH	 = CHUNK_WRITE | 83,


    ASIC_MODEL = 25,
    GREY_EXP = 26,

    ASIC_COLL_STAND = 29,
    ASIC_SET_PARAMETER = 30,
    ASIC_FREE_FLASH = 31,

    OV9750_EXP = 32,
    OV9750_GAIN = 33,
    OV9750_SWITCH= 34,

    OV4688_EXP = 36,
    OV4688_GAIN = 37,
    OV4688_SWITCH = 38,

    GET_VERSION = READ_BACK | 40,

    FREQUENCY = 41,
   	SET_EXPOSURE_MODE = 90,
	  SET_SCENE_MODE = 91,

};
typedef struct tag_RcvBuffer
{
    uint32_t size;
    short* pframe;
}RcvBuffer_t;


#if 0
#define DEVICE_MARS_ONE  (0x01<<16)
#define DEVICE_MARS_TWO (0x02<<16)
#define DEVICE_SPECKLE      (0x03<<16)
#define DEVICE_DEPTH       (0x01<<0)
#define DEVICE_VISIBLE      (0x01<<1)
enum ASIC_MODE
{
    IdpNormalVideo = 0,
    IdpEnhanceVideo = 1,
    IdpDepth = 2,
    ReadStandard =3,
};
enum SWITCH_MODE
{
    OPEN = 0,
    CLOSE = 1,
};
enum FREQUENCY
{
    Q60HZ = 60,
    Q30HZ = 30,
};
enum FREQUENCY_MODE
{
    SINGLEFREQUENCY = 0,
    DOUBLEFREQUENCY = 1,
    GREY = 2,
};


//command
typedef  struct tag_FlashCMD
{
    uint32_t  Cmd;
    uint32_t  PageAddr;   //读、写、擦除起始地址
    uint32_t  NumPages2; //操作Sectors, pages数量
}FlashCMD_t;

typedef struct tag_Preamble
{
    uint8_t buffer[8];
    uint8_t length;
    uint8_t ctrlMask;
}Preamble_t;

typedef struct tag_I2CCMD
{
    uint32_t Cmd;   //command
    uint32_t Len; //Size of the transfer in bytes
    uint32_t Retry; //Number of times to retry request in case of a NAK response or error
    Preamble_t preamble;
}I2CCMD_t;
#endif


#endif // DEVICEINFO_H

