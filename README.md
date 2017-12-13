# Mars03/04 Sample for Android

## 编译环境
在Ubuntu14.04LTS 64bit上, 用Android NDK r13b编译测试通过
理论上其他平台用NDK也可以编译  

## 编译步骤
进入sample所在目录，运行ndk_build.sh  
将会在libs/$ARCH_ABI/下生成mars_test可执行文件及libTof.so和libusb1.0.so两个依赖库

## 运行测试
*注意手机需要root权限*  
1. adb root  
2. adb remount  
3. adb push libusb1.0.so /system/lib/  (64位为/system/lib64/)
4. adb push libTof.so /system/lib/   (64位为/system/lib64/)
5. adb push mars_test /system/bin/  
6. 运行测试程序  
   cd /system/bin/
   ./mars_test(运行以后可以根据提示输入不同的选项对应不同的功能）
   
## 输出结果示例
运行程序输出
>Current VID: 40e, PID: 4d35
>Matching the VID: 40e, PID: f420
>Current VID: 40e, PID: 4d35
>Matching the VID: 40e, PID: 4d35
>Device Opened
>LibTOF_Connect-296,code;0 Lensparam[214.835831]
>LibTOF_Connect-296,code;0 Lensparam[214.835831]
>LibTOF_Connect-296,code;0 Lensparam[112.895622]
>LibTOF_Connect-296,code;0 Lensparam[82.400398]
>LibTOF_Connect-296,code;0 Lensparam[-0.104662]
>LibTOF_Connect-296,code;0 Lensparam[-1.496823]
>LibTOF_Connect-296,code;0 Lensparam[0.000202]
>LibTOF_Connect-296,code;0 Lensparam[0.001741]
>LibTOF_Connect-296,code;0 Lensparam[3.065900]
>Current firmware version: M04_V2.29.29.170808
>Current firmware version: M04_V2.29.29.170808
>device ID:0A10-5304-0462-0458 
>deviceInfo:M04_V2.29.29.170808
>Depth Data:w-224 h-172 
>visiableData:w-1344 h-760 
>DeviceVersion:

输入p拍照
>photo finish 

输入t获取温度
>cpu temperture: 60.72  sensor temperture: 53.00

输入a设置曝光模式
>a
>mode (1:manual 2:auto) :
>input c to cancel set
>2
>LibTOF_SetExpourseMode Success

输入s设置场景模式
>s
mode ( 0:Scene_No, 1:Scene_30cm, 2:Scene_75cm, 3: Scene_100cm)  :input c to cancel set
0
LibTOF_SetSceneMode Success

   