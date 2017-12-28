#include "Mars04_SDK_Wrapper.h"
namespace sunny {


//global variable
CLibTof *libtof = NULL;                         //pmd mars04 sdk class
DeviceInfo_t *deviceinfo = NULL;                //mars04 dev class object
FrameData_t* frame_data = NULL;		   			//指向存储深度数据的缓存,注意它的深度数据是点云的.
FrameDataRgb_t* frame_data_Rgb = NULL;  		//指向存储彩图数据的缓存
DepthPixel_t * pNeolixDepthData = NULL;         //新石器算法需要的深度数据格式.
short * video_data = NULL;                      //获取相机的YUV数据    
Mars04AllData_t mars04_all_data;                //包含Mars04设备中所有有效数据(灰度,彩色,深度).

//下边是获取深度图的线程所用
BOOL IS_Running=FALSE;                          //开始/停止获取深度数据
BOOL thread_exit=FALSE;                         //退出获取深度数据的线程
pthread_t pth_get_data;
//others
unsigned char	 *PColorbuffer_s = NULL;
int DEPTHMAP_W;
int DEPTHMAP_H;
int DEPTHVIDEO_W;
int DEPTHVIDEO_H;
int DEPTHVIDEO_FRAME_SIZE;
BOOL IS_RGBD;
BOOL IS_RGB;

getDepthFunc_t g_getDepthCallback = NULL;       //指向JNI层获取深度数据的func

#define LOG_TAG "mars04_wrapper_native"
#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__))
#define LOGE(...) ((void)__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__))
TOF_ErrorCode_t getMars04AllData(void);

void registerJNIGetDepthCB(getDepthFunc_t getDepthCallback)
{
	LOGI("registerJNIGetDepthCB\n");
	if (getDepthCallback != NULL)
		g_getDepthCallback = getDepthCallback;
	else 
		LOGI("registerJNIGetDepthCB:fail callback is nullptr!!!\n");
}

void* get_mars04data_fuc(void* param)
{
//    CLibTof* libtoftemp = (CLibTof*)param;
	 while (!thread_exit)
    {
        if (!IS_Running)
        {
            usleep(100);
            continue;
        }
		/*rs = */getMars04AllData();
	 }
	 return NULL;
}
void setPreviewStatus(PREVIEW_STATUS sta)
{
	IS_Running = (sta == PREVIEW_START ? true : false); 
}

TOF_ErrorCode_t DisconnectMars04(void) {
	TOF_ErrorCode_t rv = LTOF_SUCCESS;
	LOGI("DisconnectMars04\n");
	//断开设备连接，释放内存
	IS_Running=FALSE;
	thread_exit = TRUE;
	pthread_join(pth_get_data, NULL);
	LOGI("thread exit success!!\n");
	rv = libtof->LibTOF_DisConnect();
	if (rv<0)
	{
		LOGI("LibTOF_DisConnect Failed, errno:[%d]\n", rv);
	}
	LOGI("LibTOF_DisConnect OK!!!\n");
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
	return rv;
}

TOF_ErrorCode_t ConnectMars04(DeviceInfo_t &devInfo)
{
    TOF_ErrorCode_t rv = LTOF_SUCCESS;
	deviceinfo = NULL;
	libtof = NULL;
	frame_data = NULL;
	frame_data_Rgb = NULL;
	g_getDepthCallback = NULL;
	pNeolixDepthData = NULL;
	memset(&mars04_all_data, 0, sizeof(Mars04AllData_t));
	LOGI("ConnectMars04:libtof pointer to addr:%p\n", libtof);
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
	LOGI("ConnectMars04:libtof pointer to addr:%p\n", libtof);
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
		//出来的数据是I420
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
	thread_exit = FALSE;
	IS_Running = FALSE;
    //创建取数据线程
   	pthread_create(&pth_get_data,NULL,get_mars04data_fuc,libtof);
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
	rv = libtof->LibTOF_SetSceneMode(umode);
	return rv;
}


//下边的函数最好使用线程来调用,LibTOF_RcvDepthFrame2函数会发生阻塞.
TOF_ErrorCode_t getMars04AllData(void)
{
    static TOF_ErrorCode_t rs = LTOF_SUCCESS;
	if (frame_data == NULL) {
		LOGI("getMars04AllData: malloc for depth data\n");
		frame_data=(FrameData_t*)malloc(sizeof(FrameData_t)*DEPTHMAP_W*DEPTHMAP_H+DEPTHMAP_W);
		if (NULL == frame_data) {
			LOGI("getMars04AllData malloc frame_data fail\n");
			return LTOF_ERROR_NO_MEM;
		}
		LOGI("[frame_data]addr:%p, size:%d\n", frame_data, sizeof(FrameData_t)*DEPTHMAP_W*DEPTHMAP_H+DEPTHMAP_W);
	}
    memset(frame_data,0,sizeof(FrameData_t)*DEPTHMAP_W*DEPTHMAP_H+DEPTHMAP_W);
	if (pNeolixDepthData == NULL) {
		LOGI("getMars04AllData: malloc for neolixdepth data\n");
		pNeolixDepthData=(DepthPixel_t*)malloc(sizeof(DepthPixel_t)*DEPTHMAP_W*DEPTHMAP_H);
		if (NULL == pNeolixDepthData) {
			LOGI("getMars04AllData malloc pNeolixDepthData fail\n");
			return LTOF_ERROR_NO_MEM;
		}
		LOGI("[neolix depth data] addr:%p, size:%d\n", pNeolixDepthData, sizeof(DepthPixel_t)*DEPTHMAP_W*DEPTHMAP_H);
	}
	memset(pNeolixDepthData,0,sizeof(DepthPixel_t)*DEPTHMAP_W*DEPTHMAP_H);
	if (frame_data_Rgb == NULL) {
		LOGI("getMars04AllData: malloc for rgb data\n");
		frame_data_Rgb = (FrameDataRgb_t*)malloc(sizeof(FrameDataRgb_t)*DEPTHMAP_W*DEPTHMAP_H+DEPTHMAP_W);
		if (NULL == frame_data_Rgb) {
			LOGI("getMars04AllData malloc  frame_data_Rgb fail\n");
			return LTOF_ERROR_NO_MEM;
		}
		LOGI("[frame_data_Rgb] addr:%p size:%d\n", frame_data_Rgb, sizeof(FrameDataRgb_t)*DEPTHMAP_W*DEPTHMAP_H+DEPTHMAP_W);
	}
	memset(frame_data_Rgb,0,sizeof(FrameDataRgb_t)*DEPTHMAP_W*DEPTHMAP_H+DEPTHMAP_W);
	if (video_data == NULL) {
		LOGI("getMars04AllData: malloc for video data\n");
		video_data = (short*)malloc(DEPTHVIDEO_FRAME_SIZE);
		if (NULL == video_data) {
			LOGI("getMars04AllData malloc  video_data fail\n");
			return LTOF_ERROR_NO_MEM;
		}
		LOGI("[video_data] addr:%p size:%d\n", video_data, DEPTHVIDEO_FRAME_SIZE);
	}
	memset(video_data,0,DEPTHVIDEO_FRAME_SIZE);
	static	 FrameData_t* frame_data_p;
	static     	FrameDataRgb_t* frame_data_Rgb_tmp;
    frame_data_p = frame_data;
    frame_data_Rgb_tmp = frame_data_Rgb;
	//LOGI("LibTOF_RcvDepthFrame2:libtof pointer to addr:%p\n", libtof);
	LOGI("LibTOF_RcvDepthFrame2:frame_data_p pointer to addr:%p\n", frame_data_p);
	//LOGI("LibTOF_RcvDepthFrame2:frame_data_Rgb_tmp pointer to addr:%p\n", frame_data_Rgb_tmp);
	LOGI("LibTOF_RcvDepthFrame2:DEPTHMAP_W = %d, DEPTHMAP_H = %d\n", DEPTHMAP_W, DEPTHMAP_H);
	rs = libtof->LibTOF_RcvDepthFrame2(frame_data_p, frame_data_Rgb_tmp,DEPTHMAP_W, DEPTHMAP_H);
	if(rs != LTOF_SUCCESS)
    {
		LOGI("getMars04AllData LibTOF_RcvDepthFrame2 errno[%d]\n", rs);
		//usleep(100);
		return rs;
    }
	LOGI("LibTOF_RcvVideoFrame:video data buf addr:%p, size:%d\n", video_data, DEPTHVIDEO_FRAME_SIZE);
	rs = libtof->LibTOF_RcvVideoFrame(video_data, DEPTHVIDEO_FRAME_SIZE);
	if(rs != LTOF_SUCCESS)
    {
		LOGI("getMars04AllData LibTOF_RcvDepthFrame errno[%d]\n", rs);
		//usleep(100);
		return rs;
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
		mars04_all_data.pDepthData = pNeolixDepthData;
		mars04_all_data.depth_data_size = sizeof(DepthPixel_t)*DEPTHMAP_W*DEPTHMAP_H;
		mars04_all_data.pVideoData = video_data;
		mars04_all_data.video_data_size = DEPTHVIDEO_FRAME_SIZE;
		LOGI("mars04_data_info:\n");
		LOGI("pDepthData:%p depth_data_size:%d pVideoData:%p video_data_size:%d\n", 
			mars04_all_data.pDepthData,
			mars04_all_data.depth_data_size,
			mars04_all_data.pVideoData,
			mars04_all_data.video_data_size
			);
		if (g_getDepthCallback != NULL)
			g_getDepthCallback(mars04_all_data);
		return LTOF_SUCCESS;
	}
}
}
