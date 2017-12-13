#include "Mars04_SDK_Wrapper.h"
namespace sunny {
//global variable
CLibTof *libtof = NULL;
DeviceInfo_t *deviceinfo = NULL;
FrameData_t* frame_data = NULL;		   			//指向存储深度数据的缓存,注意它的深度数据是点云的.
FrameDataRgb_t* frame_data_Rgb = NULL;  		 //指向存储彩图数据的缓存
DepthPixel_t * pNeolixDepthData = NULL;           //新石器算法需要的深度数据格式.
BOOL IS_Running=FALSE;

unsigned char	 *PColorbuffer_s = NULL;
int DEPTHMAP_W;
int DEPTHMAP_H;
int DEPTHVIDEO_W;
int DEPTHVIDEO_H;
int DEPTHVIDEO_FRAME_SIZE;
BOOL IS_RGBD;
BOOL IS_RGB;
//func proto
getDepthFunc_t g_getDepthCallback = NULL;
#define LOG_TAG "mars04_wrapper_native"

#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__))
#define LOGE(...) ((void)__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__))
TOF_ErrorCode_t getMars04DepthData(void);

void registerJNIGetDepthCB(getDepthFunc_t getDepthCallback)
{
	LOGI("registerJNIGetDepthCB\n");
	if (getDepthCallback != NULL)
		g_getDepthCallback = getDepthCallback;
	else 
		LOGI("registerJNIGetDepthCB:fail callback is nullptr!!!\n");
}

void* get_depthdata_fuc(void* param)
{
//    CLibTof* libtoftemp = (CLibTof*)param;
//    TOF_ErrorCode_t rs;
	 while (IS_Running)
    {
        if (!IS_Running)
        {
            usleep(100);
            continue;
        }
		/*rs = */getMars04DepthData();
	 }
	 return NULL;
}

TOF_ErrorCode_t DisconnectMars04(void) {
	//断开设备连接，释放内存
	IS_Running=FALSE;
	usleep(50000);//等待线程中的操作完成
	LOGI("DisconnectMars04\n");
	TOF_ErrorCode_t rv = LTOF_SUCCESS;
	if (frame_data != NULL)
		free(frame_data);
	if (frame_data_Rgb != NULL)
		free(frame_data_Rgb);
	if (PColorbuffer_s != NULL)
		free(PColorbuffer_s);
	if (libtof != NULL)
		delete libtof;
	if (deviceinfo != NULL)
		delete deviceinfo;

	rv = libtof->LibTOF_DisConnect();
	if (rv<0)
    {
        LOGI("LibTOF_DisConnect Failed, errno:[%d]\n", rv);
	}
	 LOGI("LibTOF_DisConnect OK!!!\n");
	return rv;
}

TOF_ErrorCode_t ConnectMars04(DeviceInfo_t &devInfo)
{
	deviceinfo = NULL;
	libtof = NULL;
	frame_data = NULL;
	frame_data_Rgb = NULL;
	g_getDepthCallback = NULL;
	pNeolixDepthData = NULL;

    TOF_ErrorCode_t rv = LTOF_SUCCESS;
	libtof = new CLibTof();
	#ifdef NATIVE_WRAPPER_DEBUG
    LOGI("------------------------------------------------\n");
    LOGI("---------SampleCode  Version  V2.0.1------------\n");
    LOGI("------------------------------------------------\n");
	#endif
	//连接相机，初始化相机
    rv = libtof->LibTOF_Connect();
    if (rv<0)
    {
        LOGI("LibTOF_Connect Failed, errno:[%d]\n", rv);
        return rv;
    }
    //获取设备信息，图像宽高，设备版本信息
    deviceinfo = new DeviceInfo_t;
    memset(deviceinfo, 0, sizeof(DeviceInfo_t));
    rv = libtof->LibTOF_GetDeviceInfo(deviceinfo);
    if (rv<0)
    {
        LOGI(" LibTOF_GetDeviceInfo Failed, errno:[%d]\n", rv);
        return rv;
    }
    else
    {
    	memcpy(&devInfo,deviceinfo, sizeof(DeviceInfo_t));
        DEPTHMAP_W = deviceinfo->DepthFrameWidth;
        DEPTHMAP_H = deviceinfo->DepthFrameHeight-1;//减去一行头长度
        DEPTHVIDEO_W = deviceinfo->VisibleFrameWidth;
        DEPTHVIDEO_H = deviceinfo->VisibleFrameHeight;
        //DEPTHVIDEO_FRAME_SIZE =DEPTHVIDEO_W*DEPTHVIDEO_H*BYTES_PER_POINT;
#ifdef NATIVE_WRAPPER_DEBUG

        LOGI("device ID:%s \ndeviceInfo:%s\nDepth Data:w-%d h-%d \nvisiableData:w-%d h-%d \n"
            ,deviceinfo->DeviceId,deviceinfo->TofAlgVersion,DEPTHMAP_W,DEPTHMAP_H,DEPTHVIDEO_W,DEPTHVIDEO_H);
#endif
        //判断设备版本，设置设备支持的功能
        if (MARS03_TOF_RGB == deviceinfo->DeviceVersion || MARS04_TOF_RGB == deviceinfo->DeviceVersion)
        {
            IS_RGBD=TRUE;
            IS_RGB=TRUE;
        }

        DEPTHVIDEO_FRAME_SIZE = ROUND_UP(DEPTHVIDEO_W*DEPTHVIDEO_H*3/2+DEPTHVIDEO_W,1024);//尾部带一行帧信息
    }

    if (0 == deviceinfo->RgbdEn)
    {
        libtof->LibTOF_RgbdEnSet(0);
    }
    else
    {
        libtof->LibTOF_RgbdEnSet(1);
    }
	//默认参数
    libtof->LibTOF_SetUseCase(MODE_1_5FPS_1200);
    libtof->LibTOF_SetExpourseMode(Auto);
	libtof->LibTOF_SetSceneMode(Scene_No);

	//启动线程,开始拿到数据.
	IS_Running = TRUE;
    //创建取数据线程
    pthread_t pth_depth;
    pthread_create(&pth_depth,NULL,get_depthdata_fuc,libtof);
	LOGI("get depth data thread is running!!!\n");
	return (rv );
}
TOF_ErrorCode_t setMars04UseCase(CameraUseCase_e ucase)
{
	LOGI("setMars04UseCase\n");
	TOF_ErrorCode_t rv = LTOF_SUCCESS;
	rv = libtof->LibTOF_SetUseCase(ucase);
	return rv;
}
TOF_ErrorCode_t setMars04SceneMode(SceneMode_e umode)
{
	LOGI("setMars04SceneMode\n");
	TOF_ErrorCode_t rv = LTOF_SUCCESS;
	//rv = libtof->LibTOF_SetSceneMode(umode);
	return rv;
}


//下边的函数最好使用线程来调用,会发生阻塞.
TOF_ErrorCode_t getMars04DepthData(void)
{
//	LOGI("get Depth data\n");
//	if (nidata == NULL) 
//		return LTOF_ERROR_INVALID_PARAM;
	static 	CLibTof* libtoftemp = libtof;
    static TOF_ErrorCode_t rs = LTOF_SUCCESS;
	if (frame_data == NULL) {
		LOGI("getMars04DepthData: malloc for depth\n");
		frame_data=(FrameData_t*)malloc(sizeof(FrameData_t)*DEPTHMAP_W*DEPTHMAP_H+DEPTHMAP_W);
		if (NULL == frame_data) {
			LOGI("getMars04DepthData malloc frame_data fail\n");
			return LTOF_ERROR_NO_MEM;
		}
	}
    memset(frame_data,0,sizeof(FrameData_t)*DEPTHMAP_W*DEPTHMAP_H+DEPTHMAP_W);
	if (pNeolixDepthData == NULL) {
			LOGI("getMars04DepthData: malloc for neolixdepth\n");
			pNeolixDepthData=(DepthPixel_t*)malloc(sizeof(DepthPixel_t)*DEPTHMAP_W*DEPTHMAP_H);
			if (NULL == pNeolixDepthData) {
				LOGI("getMars04DepthData malloc pNeolixDepthData fail\n");
				return LTOF_ERROR_NO_MEM;
			}
			LOGI("depth addr:%p, size:%d\n", pNeolixDepthData, sizeof(DepthPixel_t)*DEPTHMAP_W*DEPTHMAP_H);
	}
	memset(pNeolixDepthData,0,sizeof(DepthPixel_t)*DEPTHMAP_W*DEPTHMAP_H);


	if (frame_data_Rgb == NULL) {
		LOGI("getMars04DepthData: malloc for rgb\n");
		frame_data_Rgb = (FrameDataRgb_t*)malloc(sizeof(FrameDataRgb_t)*DEPTHMAP_W*DEPTHMAP_H+DEPTHMAP_W);
		if (NULL == frame_data_Rgb) {
		LOGI("getMars04DepthData malloc  frame_data_Rgb fail\n");
		return LTOF_ERROR_NO_MEM;
		}
	}
	memset(frame_data_Rgb,0,sizeof(FrameDataRgb_t)*DEPTHMAP_W*DEPTHMAP_H+DEPTHMAP_W);
	static	 FrameData_t* frame_data_p;
	static     	FrameDataRgb_t* frame_data_Rgb_tmp;
    frame_data_p = frame_data;
    frame_data_Rgb_tmp = frame_data_Rgb;
	rs = libtoftemp->LibTOF_RcvDepthFrame2(frame_data_p, frame_data_Rgb_tmp,DEPTHMAP_W, DEPTHMAP_H);
	
        if(rs != LTOF_SUCCESS)
        {
			LOGI("getMars04DepthData LibTOF_RcvDepthFrame2 errno[%d]\n", rs);
			usleep(100);
			return rs;
        }
		 if (0/*P_HEAD*/)//获取每帧图像附加在后边的信息.
        {
            S_MetaData depth_head;
            S_MetaData drgb_head;
            //memset(head,0,sizeof(S_MetaData));
            memcpy(&depth_head,(unsigned char*)(frame_data+DEPTHMAP_W*DEPTHMAP_H),sizeof(S_MetaData));
            memcpy(&drgb_head,(unsigned char*)(frame_data_Rgb+DEPTHMAP_W*DEPTHMAP_H),sizeof(S_MetaData));
            LOGI("depth:[%4d] [%2d %2d]  %d\n",depth_head.frameCount,depth_head.width,depth_head.height,depth_head.getFrameTime);
            LOGI("rgbd :[%4d] [%2d %2d]  %d\n",drgb_head.frameCount,drgb_head.width,drgb_head.height,depth_head.getFrameTime);
            //free(head);
        }
        if(1)   //可以写入文件,此处可以将深度数据提取出来.
        {
         static int i,j;
		 //按x,y,z顺序保存深度数据
            for (i = 0; i<172;i++)
            {
                for(j = 0; j<224;j++)
                {
				
					if (pNeolixDepthData != NULL)    //如果jni分配了缓存,我们才给他进行拷贝.
						pNeolixDepthData[i*224+j] = (DepthPixel_t)(frame_data[i*224+j].z*1000);  //原生的数据是m,最好转换为mm
					#ifdef NATIVE_WRAPPER_DEBUG
					//打印中心点
					if (i == 86 && j == 112){
					static time_t nowtime;
                	static struct tm *timeinfo;
                	static int year,month,day,hour,min,sec;
       				time(&nowtime);
                	timeinfo=localtime(&nowtime);
                	year=timeinfo->tm_year+1900;
                	month=timeinfo->tm_mon+1;
                	day=timeinfo->tm_mday;
                	hour=timeinfo->tm_hour;
               		min=timeinfo->tm_min;
                	sec=timeinfo->tm_sec;
                	LOGI("%04d-%02d-%02d-%02d-%02d-%02d",
                    year,month,day,hour,min,sec);
					LOGI("x:%f, y:%f, z:%dcm\n", frame_data[i*224+j].x, frame_data[i*224+j].y,pNeolixDepthData[i*224+j]);
                	}
					#endif
            }
        }
		if (g_getDepthCallback != NULL)
			g_getDepthCallback(pNeolixDepthData, sizeof(DepthPixel_t)*DEPTHMAP_W*DEPTHMAP_H);
		return LTOF_SUCCESS;
	}
}
}
