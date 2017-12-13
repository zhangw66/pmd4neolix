#pragma once 
#include "libTof.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <android/log.h>

#define NATIVE_WRAPPER_DEBUG
/** 16-bit unsigned integer. */ 
typedef	unsigned short		UInt16_t;
typedef UInt16_t DepthPixel_t;
typedef int BOOL;
#define FALSE 0
#define TRUE 1
#define ROUND_UP(x, align) (x+(align-1))&(~(align-1))
typedef void (*getDepthFunc_t)(void *, int);
namespace sunny {
	TOF_ErrorCode_t DisconnectMars04(void);
	TOF_ErrorCode_t ConnectMars04(DeviceInfo_t &devInfo);
	TOF_ErrorCode_t getMars04DepthData(DepthPixel_t *& pdata);
	TOF_ErrorCode_t setMars04UseCase(CameraUseCase_e ucase);
	TOF_ErrorCode_t setMars04SceneMode(SceneMode_e umode);
	void registerJNIGetDepthCB(getDepthFunc_t getDepth);
}

