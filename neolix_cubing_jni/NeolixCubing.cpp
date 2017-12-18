#include <jni.h>
#include "Mars04_SDK_Wrapper.h"
#include "neolixMV.h"
#include <opencv2/opencv.hpp>
#include <android/log.h>
#ifdef __cplusplus
extern "C" {
#endif
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
static neolix::depthData depthData;
//存储算法建模时候的参数.
static double parameter[3] = {0.0};
//存储算法需要的像素坐标
static neolix::rect safeZone, measureZone;
FrameDataRgb_t *pColorImage = NULL;    //指向伪彩色缓存,size:w*h*bpp3
DepthPixel_t   *pDepthData = NULL;     //指向深度数据缓存,size:w*h*bpp2
DeviceInfo_t devInfo;                  //存储Mars04的基本设备信息.

//调用app的线程所需要
JavaVM *m_vm;
jmethodID m_amplitudeCallbackID;
jobject m_obj;

void getMars04DepthDataCb(void *psrc, int len)
{
	static neolix::depthData depthData;
#ifdef JNI_LOG_VERBOSE
	LOGI("getMars04DepthDataCb\n");
#endif
	//储存,根据需要做转换,比如需要转换为伪彩色.
	if (psrc != NULL && pColorImage != NULL && pDepthData != NULL) {
		#ifdef JNI_LOG_VERBOSE
		LOGI("getMars04DepthDataCb ===>psrc:%p, h:%d, w:%d, len:%d\n", psrc, depthData.height, depthData.width, len);
		#endif
		depthData.width =  devInfo.DepthFrameWidth;
		depthData.height = devInfo.DepthFrameHeight;
		depthData.data = psrc;
#ifdef JNI_LOG_VERBOSE
		LOGI("prepare to memcpy depth data to jni sapce!!!!\n");
#endif
		memcpy(pDepthData, psrc, len);
		neolix::getDepthColor(depthData,pColorImage);
		#ifdef JNI_LOG_VERBOSE
		LOGI("center pixel depth data:%d, r:%x, g:%x, b:%x\n", ((DepthPixel_t*)(depthData.data))[86*224+112], 
			pColorImage[86*224+112].r, 
			pColorImage[86*224+112].g,
			pColorImage[86*224+112].b);
		LOGI("get depth color success!!!\n");
		#endif
		// fill a temp structure to use to populate the java int array
        jint fill[depthData.width * depthData.height];
		 int i;
        for (i = 0; i < depthData.width * depthData.height; i++)
        {
            fill[i] = pColorImage[i].r | pColorImage[i].g << 8 | pColorImage[i].b << 16 | 255 << 24;
        }
#ifdef JNI_LOG_VERBOSE
		LOGI("convert rgb to argb!!!!!!\n");
#endif
		// attach to the JavaVM thread and get a JNI interface pointer
        JNIEnv *env;
        m_vm->AttachCurrentThread ( (JNIEnv **) &env, NULL);
		//LOGI("AttachCurrentThread!!!!!!\n");

        // create java int array
        jintArray intArray = env->NewIntArray (depthData.height * depthData.width);

        // populate java int array with fill data
        env->SetIntArrayRegion (intArray, 0, depthData.height * depthData.width, fill);

#ifdef JNI_LOG_VERBOSE
		LOGI("invoke env->CallVoidMethod!!!!!!\n");
#endif
        // call java method and pass amplitude array
        env->CallVoidMethod (m_obj, m_amplitudeCallbackID, intArray);
#ifdef JNI_LOG_VERBOSE		
		LOGI("invoke env->CallVoidMethod OK!!!!!!\n");
#endif
		// detach from the JavaVM thread
        m_vm->DetachCurrentThread();
		
	}
	else {
		LOGE("getMars04DepthDataCb:find nullptr!!!!!!!!!!\n");
	}	
	//回调APP的方法,将数据复制过去.
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
	
	//分配存储伪彩色的区域.
	pDepthData = new DepthPixel_t[devInfo.DepthFrameHeight*devInfo.DepthFrameWidth];
	if (pDepthData == NULL){
        LOGE("new pDepthData fail\n");
        return false;
    }
	pColorImage = new FrameDataRgb_t[devInfo.DepthFrameHeight*devInfo.DepthFrameWidth];
	if (pColorImage == NULL){
        LOGE("new pColorImage fail\n");
        return false;
    }
	depthData.width =  devInfo.DepthFrameWidth;
	depthData.height = devInfo.DepthFrameHeight;
	depthData.data = pDepthData;
	LOGI("pDepthData:%p, h:%d, w:%d, len:%d\n", 
		depthData.data, 
		depthData.height, 
		depthData.width, 
		sizeof(DepthPixel_t)*depthData.width*depthData.height);
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
	rs = sunny::DisconnectMars04();
	//释放资源.
	if (pColorImage == NULL)
		delete[] pColorImage;
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
		  //开始背景建模.
		static int n = 3;
		ret = neolix::backgroundReconstruction(depthData, parameter, n);
		if (!ret) {
			LOGE("backgroundReconstruction fail!!\n");
			return ret;
			}
		neolix::setParameter(parameter);
		LOGI("backgroundReconstruction success!!!!!\n");
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
	 if (neolix::measureVol(depthData,t))
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

void JNICALL EXTAND_FUNC_NAME(CLASS_NAME, RegisterDispCallback)
  (JNIEnv *env, jclass jc)
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
    }
	
    // save method ID to call the method later in the listener
    m_amplitudeCallbackID = env->GetMethodID (g_class, "amplitudeCallback", "([I)V");
	LOGI("Java_com_neolix_neolixcubing_NeolixCubing_RegisterDispCallback end\n");
  }
#ifdef __cplusplus
}
#endif
