#include "Mars04_SDK_Wrapper.h"
#include <stdio.h>
#include <unistd.h>
using namespace sunny;
void test()
{
		DeviceInfo_t dev;
	TOF_ErrorCode_t ret;
	memset(&dev, 0, sizeof(dev));
	printf("prepare to connect Mars04!!\n");
	ret = ConnectMars04(dev);
	if(ret != LTOF_SUCCESS)
		printf("connect fail!! errno:%d\n", ret);
	printf("device ID:%s \ndeviceInfo:%s\nDepth Data:w-%d h-%d \nvisiableData:w-%d h-%d \n"
            ,dev.DeviceId,
            dev.TofAlgVersion,
            dev.DepthFrameWidth,
        	dev.DepthFrameHeight-1,//减去一行头长度
       		dev.VisibleFrameWidth,
        	dev.VisibleFrameHeight);
	printf("prepare to connect Mars04!!\n");
	sleep(1);
	ret = DisconnectMars04();
	if(ret != LTOF_SUCCESS)
		printf("disconnect fail!! errno:%d\n", ret);
}
int main()
{
	
	int i = 3;
	while (i-- != 0) {
			test();
		}
	return 0;
}
