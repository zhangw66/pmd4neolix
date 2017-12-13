//#include "com_neolix_neolixcubing_NeolixCubing.h"
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
//camera的内参,暂时写死,可以拿到,算法需要
static  double internalCoefficient[] = {216.366592,216.764084,113.697975,86.6220627};
//算法需要的结构{frame.width, frame.height, frame.data}
static neolix::depthData depthData;
//存储算法建模时候的参数.
static double parameter[3] = {0.0};
//存储算法需要的像素坐标
static neolix::rect safeZone, measureZone;
/*typedef struct VOLUME
{
       float length;
       float width;
       float height;
}vol_t;
typedef struct {
	unsigned char red;
	unsigned char green;
	unsigned char blue;
} FrameDataRgb_t;*/

FrameDataRgb_t *pColorImage = NULL;    //指向伪彩色缓存,size:w*h*bpp3
DepthPixel_t   *pDepthData = NULL;     //指向深度数据缓存,size:w*h*bpp2
DeviceInfo_t devInfo;                  //存储Mars04的基本设备信息.
DeviceInfo_t *deviceinfo = &devInfo;
//
JavaVM *m_vm;
jmethodID m_amplitudeCallbackID;
jobject m_obj;

void getMars04DepthDataCb(void *psrc, int len)
{
	static neolix::depthData depthData;
	LOGI("getMars04DepthDataCb\n");
	//储存,根据需要做转换,比如需要转换为伪彩色.
	if (psrc != NULL && pColorImage != NULL && pDepthData != NULL) {
		LOGI("getMars04DepthDataCb ===>psrc:%p, h:%d, w:%d, len:%d\n", psrc, depthData.height, depthData.width, len);
		depthData.width =  devInfo.DepthFrameWidth;
		depthData.height = devInfo.DepthFrameHeight;
		depthData.data = psrc;
		LOGI("prepare to memcpy depth data to jni sapce!!!!\n");
		memcpy(pDepthData, psrc, len);
		neolix::getDepthColor(depthData,pColorImage);
		LOGI("center pixel depth data:%d, r:%x, g:%x, b:%x\n", ((DepthPixel_t*)(depthData.data))[86*224+112], 
			pColorImage[86*224+112].r, 
			pColorImage[86*224+112].g,
			pColorImage[86*224+112].b);
		LOGI("get depth color success!!!\n");
		// fill a temp structure to use to populate the java int array
        jint fill[depthData.width * depthData.height];
		 int i;
        for (i = 0; i < depthData.width * depthData.height; i++)
        {
            // use min value and span to have values between 0 and 255 (for visualisation)
            //fill[i] = (int) ( ( (data->points.at (i).grayValue - min) / (float) span) * 255.0f);
			//pColorImage[i];
			// set same value for red, green and blue; alpha to 255; to create gray image
            fill[i] = pColorImage[i].b | pColorImage[i].g << 8 | pColorImage[i].r << 16 | 255 << 24;
        }
		LOGI("convert rgb to argb!!!!!!\n");
        // attach to the JavaVM thread and get a JNI interface pointer
        JNIEnv *env;
        m_vm->AttachCurrentThread ( (JNIEnv **) &env, NULL);
		LOGI("AttachCurrentThread!!!!!!\n");

        // create java int array
        jintArray intArray = env->NewIntArray (depthData.height * depthData.width);

        // populate java int array with fill data
        env->SetIntArrayRegion (intArray, 0, depthData.height * depthData.width, fill);

		LOGI("invoke env->CallVoidMethod!!!!!!\n");

        // call java method and pass amplitude array
        env->CallVoidMethod (m_obj, m_amplitudeCallbackID, intArray);
		
		LOGI("invoke env->CallVoidMethod OK!!!!!!\n");
		// detach from the JavaVM thread
        m_vm->DetachCurrentThread();
		
	}
		
	//回调APP的方法,将数据复制过去.
}
/*
 * Class:     com_neolix_neolixcubing_NeolixCubing
 * Method:    connectCamera
 * Signature: (Lcom/neolix/neolixcubing/DeviceInfo;)Z
 */

jboolean JNICALL Java_com_neolix_neolixcubing_NeolixCubing_connectCamera
  (JNIEnv *env, jclass jc, jobject dev_obj)
{
	LOGI("Java_com_neolix_neolixcubing_NeolixCubing_connectCamera\n");
	TOF_ErrorCode_t rs = LTOF_SUCCESS; 
			/*
	  uint32_t  DeviceVersion;
    uint32_t  DeviceType;
    uint16_t  DepthFrameWidth;
    uint16_t  DepthFrameHeight;
    uint16_t  BitsPerPoint;
    uint16_t  VisibleFrameWidth;
    uint16_t  VisibleFrameHeight;
    uint16_t  BitsPerPixel;
    uint16_t  BlockSizeIn;
    uint16_t BlockSizeOut;
    char    TofAlgVersion[20];
    char    DeviceId[19];
    uint8_t RgbdEn;
    */
	rs = sunny::ConnectMars04(devInfo);
			
	if (rs != LTOF_SUCCESS) {
		LOGE("ConnectMars04 fail\n");
        return false;
	}
	LOGI("device ID:%s \ndeviceInfo:%s\nDepth Data:w-%d h-%d \nvisiableData:w-%d h-%d \n"
            ,deviceinfo->DeviceId,
            deviceinfo->TofAlgVersion,
            deviceinfo->DepthFrameWidth,
        	--(deviceinfo->DepthFrameHeight),//减去一行头长度，這裏注意
       		deviceinfo->VisibleFrameWidth,
        	deviceinfo->VisibleFrameHeight);
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
    (env)->SetShortField(dev_obj, fid, deviceinfo->DepthFrameWidth);
	fid = (env)->GetFieldID(clazz, "DepthFrameHeight", "S");
    (env)->SetShortField(dev_obj, fid, deviceinfo->DepthFrameHeight);
	
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
jboolean JNICALL Java_com_neolix_neolixcubing_NeolixCubing_disconnectCamera
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
jboolean JNICALL Java_com_neolix_neolixcubing_NeolixCubing_setUseCase
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
jboolean JNICALL Java_com_neolix_neolixcubing_NeolixCubing_SetSafeArea
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
jboolean JNICALL Java_com_neolix_neolixcubing_NeolixCubing_SetMeasureArea
(JNIEnv *je, jclass jc, jint x, jint y, jint width, jint height)
   {
          bool ret = true;
          LOGI("user set measure area: X coor:%d, Y coor:%d, width:%d, height:%d", x, y, width, height);
		  memset(&measureZone, 0, sizeof(neolix::rect));
		  safeZone.left_x = 62;
		  safeZone.left_y = 6;
		  safeZone.width = 159;
		  safeZone.height = 116;
		  measureZone.left_x = 62;
		  measureZone.left_y = 6;
		  measureZone.width = 159;
		  measureZone.height = 116;
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
jboolean JNICALL Java_com_neolix_neolixcubing_NeolixCubing_getCurrentVolume
  (JNIEnv *env, jclass jc, jobject volumeobj)
{
    jclass clazz;
    jfieldID fid;
    neolix::vol t;
	
	LOGI("w:%d, h:%d, [addr:%p]\n", depthData.width,
		depthData.height,
		depthData.data);
					 //((DepthPixel_t*)(depthData.data))[86*224+112]
	LOGI("z:%dcm\n", ((DepthPixel_t*)(depthData.data))[86*224+112]);
	 if (neolix::measureVol(depthData,t))
	 	LOGI("neolix::measureVol return true!!!\n");
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
void JNICALL Java_com_neolix_neolixcubing_MainActivity_RegisterDispCallback
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