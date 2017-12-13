# Mars03/04 Sample for Android

## build environment
Test on Ubuntu14.04 LTS 64bit, and NDK version is r13b   
Other OS and NDK version may work, but not tested.

## build step
Enter SampleCode directory，run ndk_build.sh  
Will generate mars_test, libTof.so & libusb1.0.so under libs/armeabi

## run test
*Notice: Need rooted device and system partition writable *
1. adb root  
2. adb remount  
3. adb push libusb1.0.so /system/lib/  
4. adb push libTof.so /system/lib/  
5. adb push mars_test /system/bin/  
6. run test, suggest connect phone by ADB-wireless cause usb is 
connect to mars device.
   cd /system/bin/
   ./mars_test
   
## Output example
Device info
>Matching the VID: 1c28, PID: c012
>Device Opened
>LibTOF_Connect-255,code;0
>LibTOF_Connect-270,code;0
>Current firmware version: M03_V1.2.9.031010
>GetDeviceInfo 
>LibTof_GetFpnData
>get fpn
>deviceInfo:M03_V1.2.9.031010
>Depth Data:w-224 h-172 
>visiableData:w-1344 h-760 
>DeviceVersion:

input 't' to read temperature
>cpu temperture: 60.72  sensor temperture: 53.00

input 'a' to set exposure mode
>a
>mode (1:manual 2:auto) :
>input c to cancel set
>2
>LibTOF_SetExpourseMode Success

input 's' to set Scene mode
>s
mode ( 0:Scene_No, 1:Scene_30cm, 2:Scene_75cm, 3: Scene_100cm)  :input c to cancel set
0
LibTOF_SetSceneMode Success

   