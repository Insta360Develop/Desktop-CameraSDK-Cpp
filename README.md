## How to get?
Please visit [https://www.insta360.com/sdk/apply](https://www.insta360.com/sdk/apply) to apply for the latest SDK.

## Support
Developers' Page: https://www.insta360.com/developer/home  
Insta360 Enterprise: https://www.insta360.com/enterprise  
Issue Report: https://insta.jinshuju.com/f/hZ4aMW  

# CameraSDK-Cpp
CameraSDK-Cpp is a C++ library to control Insta360 cameras.

### Supported cameras
| Model | Link |
| :------- | :-------- |
| ONE X (Discontinued)| http://insta360.com/product/insta360-onex/ |
| ONE R Twin Edition (Discontinued) | http://insta360.com/product/insta360-oner_twin-edition |
| ONE X2 | https://www.insta360.com/product/insta360-onex2 |
| ONE RS 1-Inch 360 Edition (Discontinued)| https://www.insta360.com/product/insta360-oners/1inch-360 |
| X3 | https://www.insta360.com/product/insta360-x3 |
| X4 | https://www.insta360.com/product/insta360-x4 |
| X5 | https://www.insta360.com/cn/product/insta360-x5 |

### Supported platforms

| Platform | Version |
| :--- | :--- |
| Windows | Windows 7 or later, only x64 supported |
| Linux | Ubuntu 22.04 (x86_64), other distributions need to be tested |


## Table of contents
- [Prerequisite](#prerequistie)
  - [Set up Your Camera Connection Mode](#setup-camera)
  - [Install Driver](#install-driver)
- [Run Test Demo](#running-demo)
- [Get Started](#get-started)
  + [Discover and Connect to Camera](#connect)
  + [Preview](#preview)
  + [Take Picture](#takepicture)
  + [Record Video](#recording)
  + [Set Exposure Settings](#exposure-settings)
  + [Set Capture Settings](#capture-settings)


## <a name="prerequistie" />Prerequisite
Before running the test demo, there are several steps that need to be prepared. 

### <a name="setup-camera" />Set Up Your Camera Connection Mode

By default, when you connect Insta360 cameras to your computer, the camera will switch to udisk mode automatically, making the camera act as a USB storage device.
You need to switch the camera to the correct mode before you can connect and control it.

#### ONE X

You need to upgrade to  a special version of firmware, [download it here](https://insta360-dev.oss-cn-hangzhou.aliyuncs.com/developer/releases/a33b3362-4767-47c3-ba9d-6ed07febb210.zip)

After upgrading, on camera, go to settings, find USB, set it to **Android** mode.

#### ONE R/RS

On the camera, swipe down on the screen to the main menu, go to Settings->General, set USB Mode to **Android**, and set U-Disk Mode to **Mobile** Mode.

#### ONE X2

On the camera, swipe down on the screen to the main menu, go to Settings->General, and set USB Mode to **Android**.

#### X3 

On the camera, swipe down on the screen to the main menu, go to Settings->General, and set USB Mode to **Android**.

#### X4

Connect the device via USB and select **Android Mode** on the camera screen.




### <a name="install-driver" />Install Driver

**On Linux**, please make sure your distribution has libusb installed. 

You may install via yum or apt-get

```shell
sudo apt-get install libusb-dev
sudo apt-get install libudev-dev
```

or build from source

```shell
wget http://sourceforge.net/projects/libusb/files/libusb-1.0/libusb-1.0.9/libusb-1.0.9.tar.bz2
tar xjf libusb-1.0.9.tar.bz2
cd libusb-1.0.9
./configure 
make
sudo make install
```

After installing driver, check whether the camera is detected via `lsusb` command, if any USB device with vender id 0x2e1a is found, congratulations, your driver is successfully installed.

###### **Note**: On Linux，demo program must be run by "**sudo**", for example

```
sudo ./CameraSDKDemo //for ubuntu
```

**On Windows**, please make sure libusbK driver is installed. You could do that by install [libusbK](https://sourceforge.net/projects/libusbk/files/libusbK-release/3.0.7.0/) directly, or using [zadig](https://zadig.akeo.ie/) to help install the libusbK driver.	

## <a name="get-started" />Get Started
### <a name="connect" />Discover and connect to camera
When both of your camera and computer are prepared, you can connect your camera to your computer via USB type-c.
#### Discover your camera
You can use class DeviceDiscovery to easily find available cameras.

```C++  
#include <camera/device_discovery.h>
...
ins_camera::DeviceDiscovery discovery;
auto list = discovery.GetAvailableDevices();
for(int i = 0;i < list.size(); ++i) {
    std::cout << "device:" << list[i].serial_number << std::endl;
}
```
The DeviceDiscovery class may establish a connection with the detected camera to retrieve some basic information, such as serial number.
The connection will be closed right after information retrieved. Remember to call **discovery.FreeDeviceDescriptors** to free allocated.resources when you no longer use the DeviveDescriptor in the list.

#### Connect to the camera
The **discovery.GetAvailableDevices()** method returns a list of **DeviceDescriptor**, which contains neccessary **DeviceConnectionInfo** of the camera.
```C++
#include <camera/camera.h>
...
ins_camera::Camera cam(list[0].info);
if(!cam.Open()) {
  std::cout << "failed to open camera" << std::endl;
  return -1;
}
... 
cam.Close(); // close the connection after use.
```
After **cam.Open()**, the DeviceDescriptor is no longer used.

### <a name='preview' />Preview
The SDK allows you to obtain unstitched dual-fisheye preview stream encoded in H.264.  
For the X4, the preview stream resolution cannot be adjusted and is set to 832x1664 by default. For other models, the preview stream resolution must be set first.

### <a name='takepicture' />Take Picture
We try to simplify the SDK to make integration for developers more easily. The SDK is designed for 360 mode and does not support operations in single-lens mode.

```
// Only for X4.  When shooting X4, you need to switch to the corresponding shooting mode first.
bool ret = cam->SetPhotoSubMode(ins_camera::SubPhotoMode::PHOTO_SINGLE);
if (!ret) {
    std::cout << "change submode failed!" << std::endl;
    continue;
}
```

```C++ 
#include <camera/camera.h>
...


const auto url = cam.TakePhoto();
if (url.Empty()) {
    std::cout << "failed to take picture" << std::endl;
    return -1;
}
std::cout << "Take picture done: " << url.GetSingleOrigin() << std::endl;
```
Once the camera is opened, an http tunnel is created at http://localhost:9099. You can make an http request to access the images on the camera.
The full url might look like this: http://localhost:9099/DCIM/Camera01/IMG_20200312_113006_00_261.jpg

### <a name='recording' />Record
Recording is as simple as taking picture:

```
// only for X4.  When using X4, you need to switch to the corresponding shooting mode first.    
bool ret = cam->SetVideoSubMode(ins_camera::SubVideoMode::VIDEO_NORMAL);
if (!ret) {
    std::cout << "change submode failed!" << std::endl;
    continue;
}
```



```C++
#include <camera/camera.h>
...
if (!cam.SetVideoCaptureParams({ins_camera::VideoResolution::RES_3840_2160p30,
  1024 * 1024 * 60
})) {
  std::cout << "Failed to set recording params." << std::endl;
} else {
  auto success = cam.StartRecording();
  if (success) {
      std::cout << "Success!" << std::endl;
  } else {
      std::cout << "Failed to start recording" << std::endl;
  }
}
```

```C++
#include <camera/camera.h>
...
auto url = cam.StopRecording();
if(url.Empty()) {
    LOG(ERROR) << "stop recording failed";
    continue;
}
auto& origins = url.OriginUrls();
LOG(INFO) << "stop recording success";
for(auto& origin_url: origins) {
    LOG(INFO) << "url:" << origin_url;
}
```
Same as taking picture, you can access the video via http request. The returned MediaUrl instance may contain multiple origin urls as well as low bitrate video urls.

### <a name="exposure-settings" />Set Exposure Settings
Exposure settings are independent between function modes (like TakePicture and Recording). Therefore, you need to specify the function mode when you set exposure settings.
```C++
#include <camera/camera.h>
...
auto exposure = std::make_shared<ins_camera::ExposureSettings>();
exposure->SetExposureMode(ins_camera::PhotographyOptions_ExposureMode::PhotographyOptions_ExposureOptions_Program_MANUAL);//set to manual exposure mode
exposure->SetIso(400); // set iso to 400
exposure->SetShutterSpeed(1.0/120.0); // set shutter to 1/120 second.
auto success = cam.SetExposureSettings(ins_camera::CameraFunctionMode::FUNCTION_MODE_NORMAL_VIDEO, exposure);
if (success) {
  std::cout << "Success!" << std::endl;
} else {
  std::cout << "Failed to set exposure settings" << std::endl;
}
```
### <a name="capture-settings"/>Set Capture Settings
Capture settings are similar to [Exposure Settings](#exposure-settings)

```C++
#include <camera/camera.h>
...
auto settings = std::make_shared<ins_camera::CaptureSettings>();
settings->SetValue(ins_camera::CaptureSettings::CaptureSettings_Saturation, 60);// set saturation to 60
settings->SetWhiteBalance(ins_camera::PhotographyOptions_WhiteBalance_WB_AUTO); // set white balance to AUTO mode
settings->SetValue(ins_camera::CaptureSettings::CaptureSettings_Brightness, 30); // set brightness to 30
settings->SetValue(ins_camera::CaptureSettings::CaptureSettings_Sharpness,4); // set sharpness to 4
settings->SetValue(ins_camera::CaptureSettings::CaptureSettings_Contrast,100); // set contrast to 100
auto ret = cam.SetCaptureSettings(ins_camera::CameraFunctionMode::FUNCTION_MODE_NORMAL_IMAGE, settings);
auto success = cam.SetCaptureSettings(ins_camera::CameraFunctionMode::FUNCTION_MODE_NORMAL_VIDEO, settings);
if (success) {
  std::cout << "Success!" << std::endl;
} else {
  std::cout << "Failed to set exposure settings" << std::endl;
}
```

For more details, please view documentations under /doc directory.

Note: On Windows, use Visual Studio 2015; on Ubuntu, use g++.  

For X4 HDR photos, the output results are already integrated in machine.
