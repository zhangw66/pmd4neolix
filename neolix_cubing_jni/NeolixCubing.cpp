#include <jni.h>
#include "Mars04_SDK_Wrapper.h"
#include "neolixMV.h"
#include <opencv2/opencv.hpp>
#include <android/log.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef enum {
	VIEW_DEPTH_MODE = 0,
	VIEW_GREY_MODE,
	VIEW_VIDEO_MODE,
	UNKNOWN_VIDEO_MODE = 0Xff,
} view_mode_t;
#define LOG_TAG "neolix_cubing_jni"
#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__))
#define LOGE(...) ((void)__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__))
#define JNI_LOG_VERBOSE
#define CLASS_NAME Java_com_neolix_neolixcubing_MainActivity_
#define JNI_FUNC_NAME(prefix, name) prefix##name 
#define EXTAND_FUNC_NAME(class_name, func_name) JNI_FUNC_NAME(class_name, func_name)

//camera的内参,暂时写死,可以拿到,算法需要
static  double internalCoefficient[] = {216.366592,216.764084,113.697975,86.6220627};
//算法需要的结构{frame.width, frame.height, frame.data}
//主要使用opencv进行转换,如:深度->伪彩 yuv420p->rgb
static neolix::depthData depthData;
static neolix::depthData yuvData;
//存储算法建模时候的参数.
//static double parameter[3] = {0.0};
//存储算法需要的像素坐标
static neolix::rect safeZone, measureZone;
static volatile bool isCubing = false;
static FrameDataRgb_t 	*pColorImage = NULL;    //指向伪彩色数据,size:w*h*bpp3
static DepthPixel_t   	*pDepthData = NULL;     //指向深度数据缓存,size:w*h*bpp2,传递给算法.
static FrameDataRgb_t   *pRGBAData = NULL;      //指向rgb摄像头的数据缓存,size:w*h*bpp4

static DeviceInfo_t devInfo;                  //存储Mars04的基本设备信息.

//view_mode_t cur_view_mode = VIEW_DEPTH_MODE;    //当前预览模式.
static view_mode_t cur_view_mode = VIEW_VIDEO_MODE, last_view_mode = UNKNOWN_VIDEO_MODE; //当前预览模式,默认为彩色

//调用app的线程所需要
JavaVM *m_vm;
jmethodID m_amplitudeCallbackID;
jobject m_obj;
JNIEnv *g_env;
jintArray g_videoBuf= NULL;   //获取app空间new出来的数组.        
jint *pVideoBuf = NULL;       //映射到native空间,修改该数据即可修改java空间.

void getMars04DepthDataCb(Mars04AllData_t& mars04AllData)
{
	static neolix::depthData DataTmp;
#ifdef JNI_LOG_VERBOSE
	LOGI("getMars04DepthDataCb\n");
#endif
	//根据当前应用层需要的预览模式设置
	if (cur_view_mode != last_view_mode) {
	LOGI("change view_mode:%d\n", cur_view_mode);
	switch (cur_view_mode) {
	case VIEW_DEPTH_MODE:
		LOGI("VIEW_DEPTH_MODE\n");
		DataTmp = depthData;
		//depthData.data = mars04AllData.pDepthData;         //传给算法一帧深度.
		if (NULL != mars04AllData.pDepthData)
			DataTmp.data = mars04AllData.pDepthData;
		else {
			LOGE("getMars04DepthDataCb:find nullprt mars04AllData.pDepthData!!!!!!\n");
			return;
		}
		break;
	case VIEW_GREY_MODE:
		break;
	case VIEW_VIDEO_MODE:
		LOGI("VIEW_VIDEO_MODE\n");
		//将yuv转换为rgba
		DataTmp = yuvData;
		if (NULL != mars04AllData.pVideoData)
			DataTmp.data = mars04AllData.pVideoData;
		else {
			LOGE("getMars04DepthDataCb:find nullprt mars04AllData.pVideoData!!!!!!\n");
			return;
		}
		break;
	default:;
	}
	last_view_mode = cur_view_mode;
	}
	//储存,根据需要做转换,比如需要转换为伪彩色.
	#ifdef JNI_LOG_VERBOSE
	LOGI("prepare to convert format!!!!!\n");
	#endif
	static int i = 0;
	static FrameDataRgb_t *pRGB = NULL;
	if (cur_view_mode == VIEW_DEPTH_MODE) {
			if (!isCubing) {
				LOGI("the cubing alorg is not running! memcpy the latest frame\n");
				memcpy(pDepthData, DataTmp.data, mars04AllData.depth_data_size);
				LOGI("center pixel info:%d\n", ((DepthPixel_t *)DataTmp.data)[DataTmp.width * DataTmp.height / 2]);
				LOGI("memcpy from [%p]to [%p] OK, size:%d\n", DataTmp.data, pDepthData, mars04AllData.depth_data_size);
			}
			LOGI("prepare depth2color process!!!!!\n");
			if (NULL != pColorImage)
        		pRGB = pColorImage;
			else {
				LOGE("no mem to save the Converted RGB data, pColorImage is null!!!!\n");
				return;
			}
			neolix::getDepthColor(DataTmp,pColorImage);
			LOGI("depth2color success!!!!\n");
	} else if (cur_view_mode == VIEW_VIDEO_MODE) {
			LOGI("prepare yuv2rgb process!!!!!\n");
			if (NULL != pRGBAData)
        		pRGB = pRGBAData;
			else {
				LOGE("no mem to save the Converted RGB data, pRGBAData is NULL!!!!\n");
				return;
			}
			yuv2rgb(DataTmp,pRGBAData);
			LOGI("yuv2rgb success!!!\n");
	}
#ifdef JNI_LOG_VERBOSE
		LOGI("convert format OK!!!!!!\n");
#endif
		// attach to the JavaVM thread and get a JNI interface pointer
        m_vm->AttachCurrentThread ( (JNIEnv **) &g_env, NULL);
		if (pVideoBuf == NULL) {
			LOGE("pVideoBuf is NULL,can't mmap java intarray!!!!!!!!\n");
			return;
		} else {
			LOGI("pvideobuf is valid!!addr:%p\n", pVideoBuf);
			LOGI("prepare to fill the video image buf within app\n");
			for (i = 0; i < DataTmp.width * DataTmp.height; i++)
        	{
            	pVideoBuf[i] = pRGB[i].r | pRGB[i].g << 8 | pRGB[i].b << 16 | 255 << 24;
        	}
			LOGI("fill the video image buf within app OK!!!!!!!!!!\n");
		}
#ifdef JNI_LOG_VERBOSE
		LOGI("invoke env->CallVoidMethod!!!!!!\n");
#endif
        // call java method and pass amplitude array
        g_env->CallVoidMethod (m_obj, m_amplitudeCallbackID, NULL);
#ifdef JNI_LOG_VERBOSE		
		LOGI("invoke env->CallVoidMethod OK!!!!!!\n");
#endif
		// detach from the JavaVM thread
        m_vm->DetachCurrentThread();
}
/*
 * Class:     com_neolix_neolixcubing_NeolixCubing
 * Method:    connectCamera
 * Signature: (Lcom/neolix/neolixcubing/DeviceInfo;)Z
 */

jboolean JNICALL EXTAND_FUNC_NAME(CLASS_NAME, connectCamera)
  (JNIEnv *env, jclass jc, jobject dev_obj)
{
	LOGI("Java_com_neolix_neolixcubing_NeolixCubing_connectCamera\n");
	TOF_ErrorCode_t rs = LTOF_SUCCESS; 
	rs = sunny::ConnectMars04(devInfo);
	if (rs != LTOF_SUCCESS) {
		LOGE("ConnectMars04 fail\n");
        return false;
	}
	LOGI("device ID:%s \ndeviceInfo:%s\nDepth Data:w-%d h-%d \nvisiableData:w-%d h-%d \n"
            ,devInfo.DeviceId,
            devInfo.TofAlgVersion,
            devInfo.DepthFrameWidth,
        	--(devInfo.DepthFrameHeight),//减去一行头长度，這裏注意
       		devInfo.VisibleFrameWidth,
        	devInfo.VisibleFrameHeight);
	//注册jni层获得深度数据的回调函数
	sunny::registerJNIGetDepthCB(getMars04DepthDataCb);
	//应用层需要拿到设备的基本信息,如:宽度,长度
	jclass clazz;
    jfieldID fid;
	clazz = (env)->GetObjectClass(dev_obj);
    if (0 == clazz) {
        LOGE("GetObjectClass neolix/neolixcubing/DeviceInfo fail\n");
        return false;
    }
    fid = (env)->GetFieldID(clazz, "DepthFrameWidth", "S");
    (env)->SetShortField(dev_obj, fid, devInfo.DepthFrameWidth);
	fid = (env)->GetFieldID(clazz, "DepthFrameHeight", "S");
    (env)->SetShortField(dev_obj, fid, devInfo.DepthFrameHeight);
	fid = (env)->GetFieldID(clazz, "VisibleFrameWidth", "S");
    (env)->SetShortField(dev_obj, fid, devInfo.VisibleFrameWidth);
	fid = (env)->GetFieldID(clazz, "VisibleFrameHeight", "S");
    (env)->SetShortField(dev_obj, fid, devInfo.VisibleFrameHeight);
	//存储深度数据的区域.如果获得体积,需要用到该数据帧
	pDepthData = new DepthPixel_t[devInfo.DepthFrameHeight*devInfo.DepthFrameWidth];
	if (pDepthData == NULL){
        LOGE("new pDepthData fail\n");
        return false;
    }
	//分配存储伪彩色数据的区域.
	pColorImage = new FrameDataRgb_t[devInfo.DepthFrameHeight*devInfo.DepthFrameWidth];
	if (pColorImage == NULL){
        LOGE("new pColorImage fail\n");
        return false;
    }
	//分配rgb摄像头的数据,是yuv转换后的.
	pRGBAData = new FrameDataRgb_t[devInfo.VisibleFrameWidth*devInfo.VisibleFrameHeight];
	if (pRGBAData == NULL){
        LOGE("new pRGBAData fail\n");
        return false;
    }
	
	depthData.width =  devInfo.DepthFrameWidth;
	depthData.height = devInfo.DepthFrameHeight;
	depthData.data = pDepthData;
	yuvData.width = devInfo.VisibleFrameWidth;
	yuvData.height = devInfo.VisibleFrameHeight;
	
	LOGI("pDepthData:%p, h:%d, w:%d, len:%d\n", 
		pColorImage, 
		depthData.height, 
		depthData.width, 
		sizeof(DepthPixel_t)*depthData.width*depthData.height);
		LOGI("pyuvData:%p, h:%d, w:%d, len:%d\n", 
		pRGBAData, 
		yuvData.height, 
		yuvData.width, 
		sizeof(FrameDataRgb_t)*yuvData.width*yuvData.height);
	sunny::setPreviewStatus(PREVIEW_START);
	return true;
}

/*
 * Class:     com_neolix_neolixcubing_NeolixCubing
 * Method:    disconnectCamera
 * Signature: ()Z
 */
jboolean JNICALL EXTAND_FUNC_NAME(CLASS_NAME, disconnectCamera)
(JNIEnv *je, jclass jc)
{
	LOGI("Java_com_neolix_neolixcubing_NeolixCubing_disconnectCamera\n");
	TOF_ErrorCode_t rs = LTOF_SUCCESS;
	sunny::setPreviewStatus(PREVIEW_STOP);
	rs = sunny::DisconnectMars04();
	cur_view_mode = VIEW_VIDEO_MODE; 
	last_view_mode = UNKNOWN_VIDEO_MODE;
	//释放资源.
	if (pColorImage == NULL)
		delete[] pColorImage;
	if (pRGBAData == NULL)
		delete[] pRGBAData;
	if (pDepthData == NULL)
		delete[] pDepthData;
	return (rs == LTOF_SUCCESS) ? true : false;
}

/*
 * Class:     com_neolix_neolixcubing_NeolixCubing
 * Method:    setUseCase
 * Signature: (I)Z
 */
jboolean JNICALL EXTAND_FUNC_NAME(CLASS_NAME, setUseCase)
 (JNIEnv *je, jclass jc, jint ucase)
 {
      TOF_ErrorCode_t ret = LTOF_SUCCESS;
      LOGI("user choose use case:%d", ucase);
	  ret = sunny::setMars04UseCase((CameraUseCase_e)ucase);
      return (ret == LTOF_SUCCESS) ? true : false;
 }

/*
 * Class:     com_neolix_neolixcubing_NeolixCubing
 * Method:    SetSafeArea
 * Signature: (IIII)Z
 */
jboolean JNICALL EXTAND_FUNC_NAME(CLASS_NAME, SetSafeArea)
   (JNIEnv *je, jclass jc, jint x, jint y, jint width, jint height)
 {
        bool ret = true;
        LOGI("user set safe area: X coor:%d, Y coor:%d, width:%d, height:%d", x, y, width, height);
		memset(&safeZone, 0, sizeof(neolix::rect));
		safeZone.left_x = x;
		safeZone.left_y = y;
		safeZone.width = width;
		safeZone.height = height;
		return ret;
 }

/*
 * Class:     com_neolix_neolixcubing_NeolixCubing
 * Method:    SetMeasureArea
 * Signature: (IIII)Z
 */
jboolean JNICALL EXTAND_FUNC_NAME(CLASS_NAME, SetMeasureArea)
(JNIEnv *je, jclass jc, jint x, jint y, jint width, jint height)
   {
   bool ret = true;
   LOGI("user set measure area: X coor:%d, Y coor:%d, width:%d, height:%d", x, y, width, height);
	memset(&measureZone, 0, sizeof(neolix::rect));
		  measureZone.left_x = x;
		measureZone.left_y = y;
		measureZone.width = width;
		measureZone.height = height;
		  LOGI("safe zone X coor:%d, Y coor:%d, width:%d, height:%d",  
		  safeZone.left_x,
		  safeZone.left_y,
		  safeZone.width,
		  safeZone.height);
		  LOGI("measure zone X coor:%d, Y coor:%d, width:%d, height:%d",  
		  measureZone.left_x,
		  measureZone.left_y,
		  measureZone.width,
		  measureZone.height);
		  ret = neolix::setArea(safeZone, measureZone,internalCoefficient, 4);
			if (!ret) {
			LOGE("set Area fail!!\n");
			return ret;
			}
#if 0
			//开始背景建模.
		static int n = 3;
		ret = neolix::backgroundReconstruction(depthData, parameter, n);
		if (!ret) {
			LOGE("backgroundReconstruction fail!!\n");
			return ret;
			}
		neolix::setParameter(parameter);
		LOGI("backgroundReconstruction success!!!!!\n");
#endif
		  return ret;
}

/*
 * Class:     com_neolix_neolixcubing_NeolixCubing
 * Method:    getCurrentVolume
 * Signature: ()Lcom/neolix/neolixcubing/Volume;
 */
jboolean JNICALL EXTAND_FUNC_NAME(CLASS_NAME, getCurrentVolume)
  (JNIEnv *env, jclass jc, jobject volumeobj)
{
    jclass clazz;
    jfieldID fid;
    neolix::vol t;
	bool ret = true;
	//static neolix::depthData depthDataTmp = depthData;
	//depthDataTmp.data = pDepthData;
	if (NULL == depthData.data) {
		LOGE("no depth data, depthDataTmp.data is NULL!!!!!!!\n");
		return false;
	}
	isCubing = true;
	LOGI("getCurrentVolume:one frame depth info:w:%d, h:%d, addr:%p\n", depthData.width, depthData.height, depthData.data);
	//LOGI("center pixel info:%d\n", ((DepthPixel_t *)depthData.data)[depthData.width * depthData.height / 2]);
	ret = neolix::measureVol2(depthData, t, 0);
	isCubing = false;
	 if (ret)
	 	LOGI("neolix::measureVol return true!!!height:%f, width:%f, length:%f\n",
		t.height,
		t.width,
		t.length);
	 else 
	 	{
	 	LOGI("neolix::measureVol return false!!!\n");
		return false;
		}
	 /*
	 	dummy data!!
        t.length = 6.6;
        t.width = 23.5;
        t.height = 66.5;
     */
    clazz = (env)->GetObjectClass(volumeobj);
    if (0 == clazz) {
        LOGE("GetObjectClass fail\n");
        return false;
    }
    fid = (env)->GetFieldID(clazz, "height", "F");
    (env)->SetFloatField(volumeobj, fid, t.height);
    fid = (env)->GetFieldID(clazz, "width", "F");
    (env)->SetFloatField(volumeobj, fid, t.width);
    fid = (env)->GetFieldID(clazz, "length", "F");
    (env)->SetFloatField(volumeobj, fid, t.length);
    return true;
}
  /*
 * Class:     com_neolix_neolixcubing_NeolixCubing
 * Method:    RegisterDispCallback
 * Signature: ()V
 */
 #define CLASS_NAME Java_com_neolix_neolixcubing_MainActivity_
 #define JNI_FUNC_NAME(prefix, name) prefix##name 
 #define EXTAND_FUNC_NAME(class_name, func_name) JNI_FUNC_NAME(class_name, func_name)

jboolean JNICALL EXTAND_FUNC_NAME(CLASS_NAME, RegisterDispCallback)
  (JNIEnv *env, jclass jc, jintArray buf, jint mode)
{
  	LOGI("Java_com_neolix_neolixcubing_NeolixCubing_RegisterDispCallback in\n");
  	// save JavaVM globally; needed later to call Java method in the listener
    env->GetJavaVM (&m_vm);
	m_obj = env->NewGlobalRef (jc);

    // save refs for callback
    jclass g_class = env->GetObjectClass (m_obj);
    if (g_class == NULL)
    {
        LOGE(" Java_com_neolix_neolixcubing_NeolixCubing_RegisterDispCallback Failed to find class\n");
		return false;
	}
	
    // save method ID to call the method later in the listener
    m_amplitudeCallbackID = env->GetMethodID (g_class, "amplitudeCallback", "([I)V");
	LOGI("Java_com_neolix_neolixcubing_NeolixCubing_RegisterDispCallback end\n");
	LOGI("prepare  pointer to the video buf within APP sapce");
	if (NULL == buf) {
		LOGE("app's jint array is NULL\n");
		return false;
	}
	g_videoBuf = buf;
	pVideoBuf = (jint *)env->GetIntArrayElements(g_videoBuf, NULL);
	if (NULL == pVideoBuf) {
		LOGE("get direct buf fail!!!!!!!");
		return false;
	}
	LOGI("VIDEO BUF MMAP IS OK, addr:%p!!\n", pVideoBuf);
	cur_view_mode = (view_mode_t)mode;
	last_view_mode = UNKNOWN_VIDEO_MODE; 
	return true;
  }
#ifdef __cplusplus
}
#endif
