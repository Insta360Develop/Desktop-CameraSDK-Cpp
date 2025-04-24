# How to get?

Please visit https://www.insta360.com/sdk/apply to apply for the latest SDK.

# Support

Developers' Page: https://www.insta360.com/developer/home

Insta360 Enterprise: https://www.insta360.com/enterprise

Issue Report: https://insta.jinshuju.com/f/hZ4aMW

# [中文文档](README_zh.md)

# Overview

CameraSDK is primarily used for connecting to cameras, configuring and retrieving camera parameters, controlling camera operations for capturing photos and recording videos, downloading files, and upgrading firmware (only supported for cameras from model X4 and later). It supports camera connections exclusively via USB. Designed for enterprise users, it mainly supports panoramic cameras such as the ONE X, ONE X2, R/RS, X3, X4, X5. The primary supported platforms are Windows and Ubuntu 22.04.

- **Supported cameras**

| Model                                    | Link                                                      |
| :--------------------------------------- | :-------------------------------------------------------- |
| ONE X (Discontinued)                     | http://insta360.com/product/insta360-onex/                |
| ONE R Twin Edition (Discontinued)        | http://insta360.com/product/insta360-oner_twin-edition    |
| ONE X2                                   | https://www.insta360.com/product/insta360-onex2           |
| ONE RS 1-Inch 360 Edition (Discontinued) | https://www.insta360.com/product/insta360-oners/1inch-360 |
| X3                                       | https://www.insta360.com/product/insta360-x3              |
| X4                                       | https://www.insta360.com/product/insta360-x4              |
| X5                                       | https://www.insta360.com/product/insta360-x5           |

- **Supported platforms**

| Platform | Version                                                      |
| :------- | :----------------------------------------------------------- |
| Windows  | Windows 7 or later, only x64 supported                       |
| Linux    | Ubuntu 22.04 (x86_64), other distributions need to be tested |

# **Table of contents**

* [Environment Setup](#Environment-Setup)
* [Camera Discovery](#Camera-Discovery)
* [Camera Connection and Disconnection](#Camera-Connection-and-Disconnection)
* [Photo Capture](#Photo-Capture)
* [Video Recording](#Video-Recording)
* [File Download](#File-Download)
* [File Information Retrieval](#File-Information-Retrieval)
* [File Deletion](#File-Deletion)
* [Recorded File Name Retrieval](#Recorded-File-Name-Retrieval)
* [Firmware Upgrade](#Firmware-Upgrade-(Only-Supported-for-X4-and-Later-Cameras))
* [Status Query](#Status-Query)
* [Preview Stream Functionality](#Preview-Stream-Functionality)
* [Log](#Log)
* [Others](#Others)



# **Feature Usage Instructions**

## **Environment Setup**

### **Switching the Camera to Android Mode**

By default, when you connect an Insta360 camera to a computer, the camera automatically switches to U disk mode, causing it to be recognized as a USB storage device. To use the camera’s “Android” control mode, you must switch the camera to **Android** mode.

#### **For ONE X**   

You need to upgrade to a special firmware version, which can be downloaded [[here](https://insta360-dev.oss-cn-hangzhou.aliyuncs.com/developer/releases/a33b3362-4767-47c3-ba9d-6ed07febb210.zip)]. After the upgrade, open the camera’s settings, locate the “Android” option, and enable it.

#### **For ONE R/RS,** **ONE X2**, X3

   On the camera, swipe down to access the main menu, then go to **Settings** → **General** → **USB Mode**, and set **USB Mode** to **Android**.

####  **For** **X4**、X5

Connect the camera via USB. Once connected, a pop-up page will appear; on that page, select Android mode.

### Driver Installation

#### **Linux**

Ensure that the distribution has libusb installed. Installation can be performed using yum or apt-get.

```bash
sudo apt-get install libusb-dev
sudo apt-get install libudev-dev
```

Or build from source:

```bash
wget http://sourceforge.net/projects/libusb/files/libusb-1.0/libusb-1.0.9/libusb-1.0.9.tar.bz2
tar xjf libusb-1.0.9.tar.bz2
cd libusb-1.0.9
./configure 
make
sudo make install
```

Once the driver installation is complete, you can use the lsusb command to check if the camera is detected properly. If you see a device with an ID such as 0x2e1a, it indicates the driver is installed correctly.

**Note**: On Linux systems, if you encounter permission issues, you must run the program with sudo. For example:

```bash
sudo ./CameraSDKDemo   // for Ubuntu
```

#### **Windows**

Please install the [**libusbK**](https://sourceforge.net/projects/libusbk/files/libusbK-release/3.0.7.0/) driver. You can install libusbK directly or use [**Zadig**](https://zadig.akeo.ie/) to install the libusbK driver.

## Camera Discovery

Camera discovery is primarily achieved through the **ins\_camera::DeviceDiscovery** interface.&#x20;

```c++
// Example code
// The DeviceDescriptor struct mainly stores basic camera information for connection purposes.
struct DeviceDescriptor {
    CameraType camera_type;  // Camera type, e.g., X3 or X4
    std::string serial_number;  // Serial number of the current camera
    std::string fw_version;     // Version number of the current camera firmware
    DeviceConnectionInfo info;  // This information is not used; PC SDK only supports USB connections
};

ins_camera::DeviceDiscovery discovery;
// Store discovered camera information in the list
std::vector<DeviceDescriptor> list = discovery.GetAvailableDevices();
```



## Camera Connection and Disconnection

### **Creating a Camera Instance**

After obtaining the camera information, use the **ins\_camera::Camera** and **DeviceDescriptor** interfaces to create an instance that controls the camera.

```c++
// Get the necessary parameter information from the camera list
auto camera_info = list[0].info;
auto camera = std::make_shared<ins_camera::Camera>(camera_info);
// Once successfully created, a camera instance is generated
```

### **Opening the Camera**

Next, you can open the camera using the **Open&#x20;**&#x69;nterface:

```c++
bool success = camera->Open()；
if(!success) {
    std::cout << "failed to open camera" << std::endl;
    return -1;
}
```

### **Disconnecting the Camera**

The camera can be disconnected by calling the **Close** interface.

> **Note**: After using the firmware upgrade interface, it is necessary to call the interface to close the camera. Once the upgrade is successful, recreate the camera instance.

```c++
camera->Close();
```

### **Checking Camera Connection Status**

Use the **IsConnected** interface to determine whether the camera is currently connected.

**Note**: Avoid calling this interface while the camera is switching modes.



## **Photo Capture**

### **Basic Parameter Settings**

#### **Setting Camera Photo Mode**

Use the SetPhotoSubMode interface to switch between different **Photo Modes**. The supported modes are listed below. For specific usage, please refer to the camera’s interface.

```c++
enum SubPhotoMode {
    PHOTO_SINGLE = 0,    // Photo
    PHOTO_HDR = 1,       // HDR Photo
    PHOTO_INTERVAL = 2,  // Interval
    PHOTO_BURST = 3,     // Burst
    PHOTO_STARLAPSE = 7  // Starlapse 
};  
// Use the interface to switch Photo Modes  
camera->SetPhotoSubMode(SubPhotoMode::PHOTO_SINGLE)
```



#### **Setting Photo Resolution**

 The **SetPhotoSize** interface allows setting the photo size. Currently, **X4** **and** **X3** support formats mainly **72M** and **18M**.

 **Note**: This interface can only be applied to the photo modes in the **CameraFunctionMode** enumeration. **Video modes do not support the PhotoSize interface.**

```C++
// Photo Resolution Options
enum PhotoSize {
    Size_6912_3456 = 0,   // X3 18MP
    Size_6272_3136 = 1,
    Size_6080_3040 = 2,
    Size_4000_3000 = 3,
    Size_4000_2250 = 4,
    Size_5212_3542 = 5,
    Size_5312_2988 = 6,
    Size_8000_6000 = 7,
    Size_8000_4500 = 8,
    Size_2976_2976 = 9,
    Size_5984_5984 = 10,
    Size_11968_5984 = 11,  // 72MP
    Size_5952_2976 = 12,   // X4 18MP
};

// Photo Modes
enum CameraFunctionMode {
    FUNCTION_MODE_NORMAL = 0,             // Default Mode
    FUNCTION_MODE_INTERVAL_SHOOTING = 3,  // Interval Shooting Mode
    FUNCTION_MODE_BURST = 5,              // Burst Mode
    FUNCTION_MODE_NORMAL_IMAGE = 6,       // Photo Mode
    FUNCTION_MODE_HDR_IMAGE = 8,          // HDR Photo Mode
    FUNCTION_MODE_AEB_NIGHT_IMAGE = 13,   
    FUNCTION_MODE_STARLAPSE_IMAGE = 18,   // Starlapse Mode
    ...
};

//Example: Setting a Photo Mode to 72MP Resolution
camera->SetPhotoSize(CameraFunctionMode::FUNCTION_MODE_NORMAL_IMAGE,PhotoSize::Size_11968_5984)
```

#### **Exposure Parameters**

 The ExposureSettings interface is used to store exposure parameters. The ExposureSettings interface includes **ISO,** **Shutter** **Speed, Exposure Mode**, and **Exposure Compensation (EV Bias)**. Refer to the following code example:

```C++
class CAMERASDK_API ExposureSettings {
    public:
        friend class Camera;
        ExposureSettings();
 
        void SetIso(int32_t value);
        void SetShutterSpeed(double speed);
        void SetExposureMode(PhotographyOptions_ExposureMode mode);
        void SetEVBias(int32_t value);

        int32_t Iso() const;
        double ShutterSpeed() const;
        PhotographyOptions_ExposureMode ExposureMode() const;
        int32_t EVBias() const;

    private:
        std::shared_ptr<ExposureSettingsPrivate> private_impl_;
    };
```

 Use the **GetExposureSettings** interface to obtain the exposure parameters for each Photo Mode. Refer to the following example:

```C++
// Get exposure parameters for Photo mode
const auto funtion_mode = ins_camera::CameraFunctionMode::FUNCTION_MODE_NORMAL_IMAGE;
auto exposure_settings = camera->GetExposureSettings(funtion_mode);
auto iso = exposure_settings->Iso();
auto shutter_speed = exposure_settings->ShutterSpeed();
// ...
```

 Use the **SetExposureSettings** interface to configure the exposure parameters for each Photo Mode. Refer to the following example:

```C++
// 1. Get exposure parameters for Photo mode
const auto funtion_mode = ins_camera::CameraFunctionMode::FUNCTION_MODE_NORMAL_IMAGE;
auto exposure_settings = camera->GetExposureSettings(funtion_mode);

// 2. Set exposure parameters
exposure_settings->SetEVBias(bias); // range -80 ~ 80, default 0, step 1
exposure_settings->SetIso(800);
exposure_settings->SetShutterSpeed(1.0 / 120.0);

// 3. Apply exposure settings to the camera
camera->SetExposureSettings(funtion_mode,exposure_settings);
```

#### **White Balance Parameters**

 The **White Balance** interface is stored in the **CaptureSettings** interface, which contains **White Balance** and other parameters. 

 The **current White Balance parameters** are obtained using the **GetCaptureSettings** interface. Refer to the following code:

```C++
// Get White Balance parameters for the Photo Mode
const auto funtion_mode = ins_camera::CameraFunctionMode::FUNCTION_MODE_NORMAL_IMAGE;
auto capture_setting = camera->GetCaptureSettings(funtion_mode);
auto value = settings->GetIntValue(ins_camera::CaptureSettings::SettingsType::CaptureSettings_WhiteBalance);
```

 Use the **SetCaptureSettings** interface to configure the White Balance parameters. Example:

```C++
// 1. Get White Balance parameters
const auto funtion_mode = ins_camera::CameraFunctionMode::FUNCTION_MODE_NORMAL_IMAGE;
auto capture_setting = camera->GetCaptureSettings(funtion_mode);

// 2. Set White Balance parameters
capture_setting->SetWhiteBalance(ins_camera::PhotographyOptions_WhiteBalance::WB_6500K);

// 3. Apply the parameters to the camera
camera->SetCaptureSettings(funtion_mode, capture_setting);
```

### **Normal Photo**

 **Normal photos** can be captured using the following reference code:

```C++
// Set to Photo Mode
camera->SetPhotoSubMode(SubPhotoMode::PHOTO_SINGLE)

// Set resolution to 18MP. This applies to X3. For X4, set to Size_5952_2976
ins_camera::PhotoSize photo_size = ins_camera::PhotoSize::Size_6912_3456;
if (camera_type == ins_camera::CameraType::Insta360X4) {
    photo_size = ins_camera::PhotoSize::Size_5952_2976;
}
camera->SetPhotoSize(CameraFunctionMode::FUNCTION_MODE_NORMAL_IMAGE,photo_size);

// Capture photo
camera->takePhoto();
```

### **HDR** **Photo**

 A dedicated interface is provided for **HDR** **Photo**: **StartHDRCapture**. 

 For cameras prior to **X3**, once exposure is complete, the number of photos obtained is typically **3**, **5**, **7**, or **9**. 

 For **X4** cameras, after the capture is complete, **1** photo is obtained, which has already been merged for HDR and can **be** **stitched directly.**

```C++
ins_camera::PhotoSize photo_size = ins_camera::PhotoSize::Size_6912_3456;
if (camera_type == ins_camera::CameraType::Insta360X4) {
    photo_size = ins_camera::PhotoSize::Size_5952_2976;
}
camera->StartHDRCapture(photo_size);
```



## **Video Recording**

### **Basic Parameter Settings**

#### **Setting Camera Video Mode**

Use the **SetVideoSubMode** interface to switch between different **video modes**. The supported modes are listed below. For specific usage, refer to the Camera Menu.

```c++
enum SubVideoMode {
    VIDEO_NORMAL = 0,         // Normal Video
    VIDEO_BULLETTIME = 1,     // Bullet Time
    VIDEO_TIMELAPSE = 2,      // Timelapse
    VIDEO_HDR = 3,            // HDR Video
    VIDEO_TIMESHIFT = 4,      // Timeshift
    VIDEO_LOOPRECORDING = 6   // Loop Recording
};

// Use the interface to switch video modes
camera->SetVideoSubMode(SubVideoMode::VIDEO_NORMAL);
```



#### **Setting Resolution**

The **SetVideoCaptureParams** interface is used to configure recording parameters.

This interface is only applicable to video modes listed under **CameraFunctionMode**.The **SetVideoCaptureParams** interface **does not support** setting parameters for photo modes.

```c++
// Video Modes
enum CameraFunctionMode {
    FUNCTION_MODE_MOBILE_TIMELAPSE = 2,  // Mobile Timelapse Mode
    FUNCTION_MODE_NORMAL_VIDEO = 7,      // Normal Video Mode
    FUNCTION_MODE_HDR_VIDEO = 9,         // HDR Video Mode
    FUNCTION_MODE_INTERVAL_VIDEO = 10,   // Interval Video Mode
    FUNCTION_MODE_STATIC_TIMELAPSE = 11, // Timelapse Mode
    FUNCTION_MODE_TIMESHIFT = 12,        // Timeshift Mode
    FUNCTION_MODE_LOOP_RECORDING_VIDEO = 17 // Loop Recording Mode
};

// This structure defines recording resolution, frame rate, and bitrate
struct RecordParams {
    VideoResolution resolution;
    int32_t bitrate{ 0 };
};

auto function_mode = ins_camera::CameraFunctionMode::FUNCTION_MODE_NORMAL_VIDEO;
ins_camera::RecordParams record_params;
// Set 5.7K 60FPS
record_params.resolution = ins_camera::VideoResolution::RES_2880_2880P60;
// Set bitrate to 10MB. This value may not be valid, and certain bitrates might cause write failures during recording.
record_params.bitrate = 1024 * 1024 * 10;
if (!cam->SetVideoCaptureParams(record_params, function_mode)) {
    std::cerr << "failed to set capture settings." << std::endl;
}
```



#### **Exposure Parameters**

Refer to * [曝光参数](#曝光参数)


#### **White Balance Parameters**

Refer to **4.1.4**

### **Normal Recording**

Normal recording can be demonstrated using the following reference code.

#### **Start Recording**

```c++
// 1. Switch mode to normal video mode
bool ret = camera->SetVideoSubMode(ins_camera::SubVideoMode::VIDEO_NORMAL);
if (!ret) {
    std::cout << "change sub mode failed!" << std::endl;
    continue;
}

// 2. Set current resolution, frame rate, and bitrate
auto function_mode = ins_camera::CameraFunctionMode::FUNCTION_MODE_NORMAL_VIDEO;
ins_camera::RecordParams record_params;
// Set 5.7K 60FPS
record_params.resolution = ins_camera::VideoResolution::RES_2880_2880P60;
record_params.bitrate = 1024 * 1024 * 10;
// Set bitrate to 10MB, this parameter may not take effect, as certain camera models or settings may have a fixed bitrate
if (!camera->SetVideoCaptureParams(record_params, function_mode)) {
    std::cerr << "failed to set capture settings." << std::endl;
}
else {
// 3. Start recording
    const auto ret = camera->StartRecording();
    if (ret) {
        std::cerr << "success!" << std::endl;
    }
    else {
        std::cerr << "failed to start recording" << std::endl;
    }
}
```

#### **Stopping recording**

```c++
// Stopping recording
auto url = cam->StopRecording();
if (url.Empty()) {
    std::cerr << "stop recording failed" << std::endl;
    continue;
}
// Output all original URLs generated by the recording
auto& origins = url.OriginUrls();
std::cout << "stop recording success" << std::endl;
for (auto& origin_url : origins) {
    std::cout << "url:" << origin_url << std::endl;
}
```

### **Timelapse Recording**

Timelapse recording can be demonstrated using the reference code below:

The table below shows the supported **resolution-frame rate** mappings for Timelapse mode on different cameras.

| **Resolution-Frame Rate** |    **X4**、X5    |      **X3**      | **ONE X2** | **R** | **RS** | **ONE X** |
| :-----------------------: | :--------------: | :--------------: | :--------: | :---: | :----: | :-------: |
|         **11K30**         | RES_5632_5632P30 |  Not Supported   |    Todo    | Todo  |  Todo  |   Todo    |
|         **8K30**          | RES_3840_3840P30 | RES_3840_3840P30 |    Todo    | Todo  |  Todo  |   Todo    |
|        **5.7K30**         | RES_2880_2880P30 | RES_2880_2880P30 |    Todo    | Todo  |  Todo  |   Todo    |

#### **Start Recording**

```c++

// 1. Switch the camera mode to timelapse mode
bool ret = cam->SetVideoSubMode(ins_camera::SubVideoMode::VIDEO_TIMELAPSE);
if (!ret) {
    std::cout << "change sub mode failed!" << std::endl;
    continue;
}

// 2. Set the resolution and frame rate for the current mode
auto function_mode = ins_camera::CameraFunctionMode::FUNCTION_MODE_MOBILE_TIMELAPSE;
ins_camera::RecordParams record_params;
// 8K30 resolution
record_params.resolution = ins_camera::VideoResolution::RES_3840_3840P30;
if (!cam->SetVideoCaptureParams(record_params, function_mode )) {
    std::cerr << "failed to set capture settings." << std::endl;
    break;
}

// 3. Set the parameters for mobile timelapse
auto timelapse_mode = ins_camera::CameraTimelapseMode::MOBILE_TIMELAPSE_VIDEO;
const uint32_t record_duration_s = 10;  // Recording duration: 10 seconds
const uint32_t lapseTime_ms = 1000;     // Interval duration: 1 second
ins_camera::TimelapseParam param = { timelapse_mode, 10, 1000, 0 };
if (!cam->SetTimeLapseOption(param)) {
    std::cerr << "failed to set capture settings." << std::endl;
} 
else {
// 4. Start recording
    const auto ret = cam->StartTimeLapse(param.mode);
    if (ret) {
        std::cerr << "success!" << std::endl;
    } 
    else {
        std::cerr << "failed to start timelapse" << std::endl;
    }
}
```

#### **Stop Recording**

```c++
auto timelapse_mode = ins_camera::CameraTimelapseMode::MOBILE_TIMELAPSE_VIDEO;
auto url = cam->StopTimeLapse(timelapse_mode );
if (url.Empty()) {
    std::cerr << "stop timelapse failed" << std::endl;
    continue;
}
// Get the recorded media file location on the camera
auto& origins = url.OriginUrls();
std::cout << "stop timelapse success" << std::endl;
for (auto& origin_url : origins) {
    std::cout << "url:" << origin_url << std::endl;
}
```



## **File Information**

### **Retrieving the Number of Files**

Use the **GetCameraFilesCount** interface to obtain the number of recorded files on the camera’s SD card.

### **Retrieving the File List**

 Use the **GetCameraFilesList** interface to obtain a list of recorded files on the SD card. Refer to the code below:

```C++
auto file_list = camera->GetCameraFilesList();
for(const auto& file : file_list) {
    std::cout << file << std::endl;
}

// print output
/DCIM/Camera01/VID_20250122_071405_00_001.insv
/DCIM/Camera01/LRV_20250122_071405_01_001.lrv
/DCIM/Camera01/VID_20250214_063916_00_002.insv
/DCIM/Camera01/LRV_20250214_063916_01_002.lrv
```



## **Retrieving the List of Files Currently Being Recorded**

Use the **GetRecordingFiles** interface to obtain the names of files currently being recorded on the camera.

## **File Download**

### **Download Files**

Using the **DownloadCameraFile** interface allows downloading existing media from the camera or SD card to the local device.

A callback can be set to obtain the current file download process. This interface is called synchronously; it returns only after the download is complete or has failed.&#x20;

> **Note:**
>
> 1. Before downloading, ensure that the directory path on the local device has been created.
>
> 2. Because the SDK starts an HttpServer, if the port is occupied, the service may fail to start. The SDK provides the **SetServicePort&#x20;**&#x69;nterface to resolve this issue. The default port in the SDK is **9099**. It is necessary for the caller to check whether the port is in use. If the port is already occupied, set the port before calling the **Open** interface.

```c++
// Camera-side file path
std::string camera_file = "/DCIM/Camera01/VID_20250122_071405_00_001.insv";
// Local save path
std::string local_save_file = "/path/to/local/VID_20250122_071405_00_001.insv";

// Start download
bool ret = camera->DownloadCameraFile(camera_file, local_save_file, [](int64_t current, int64_t total_size) {
    //Display Progress
    std::cout << "current :" << current << "; total_size: " << total_size << std::endl;
});

// Download result
if(ret) {
   std::cout << "successed to download file" << std::end; 
}
```



###  **Cancel Download**

Use the **CancelDownload** interface to cancel the file that is currently being downloaded.



## **Delete Files**

Use the **DeleteCameraFile** interface to delete unwanted files from the SD card.

```C++
const auto camera_file = "/DCIM/Camera01/VID_20250122_071405_00_001.insv";
camera_file->DeleteCameraFile(camera_file);
```



## **Firmware Upgrade (Only Supported for X4  and Later Cameras)**

The **UploadFile** interface is used to upgrade the firmware version. Currently, only **X4** and later cameras are supported. The predefined firmware file name is: **Insta360X4FW.bin**. Below is an example code snippet:

```c++
// Set the firmware file name
const std::string fireware_name = "Insta360X4FW.bin";
std::string file_name = "Insta360X4FW.bin";
if (camera_type == ins_camera::CameraType::Insta360X5) {
    file_name = "Insta360X5FW.bin";
}

const auto ret = cam->UploadFile(local_path, file_name,
// Specify the local path of the firmware
const std::string local_file = "/path/to/fireware/Insta360X4FW.bin"；

// Start uploading
bool ret = cam->UploadFile(local_file, fireware_name ,
    [](int64_t current, int64_t total_size) {
      // Firmware upload progress display
      std::cout << "current :" << current << ";total_size: " << total_size << std::endl;
});

if (ret) {
    std::cout << "succeeded to upload file" << std::endl;
} 

//After a successful upload, the camera must be powered off. Once the upgrade is complete, recreate the camera instance (the camera needs to restart for the upgrade).
camera->close();
```

## **Status Query**

### **Battery Status Information**

Use the **GetBatteryStatus** interface to obtain the camera’s current battery information, such as the current battery level.

```c++
// These definitions are provided in the SDK's header file
enum PowerType {
    BATTERY = 0,
    ADAPTER = 1,
};

struct BatteryStatus {
    PowerType power_type;   // Power source type
    uint32_t battery_level; // Current battery percentage (0~100)
    uint32_t battery_scale; // This value is not used.
};

BatteryStatus status;
bool successed = camera->GetBatteryStatus(status);
```



###  **SD Card Storage Information**

Use the **GetStorageState** interface to obtain the current status of the SD card, including its state and available space.

```c++
// These definitions are provided in the SDK's header file
enum CardState {
    STOR_CS_PASS = 0,          // Normal
    STOR_CS_NOCARD = 1,        // No card
    STOR_CS_NOSPACE = 2,       // Insufficient space
    STOR_CS_INVALID_FORMAT = 3,// Incorrect card format
    STOR_CS_WPCARD = 4,        // Write-protected card
    STOR_CS_OTHER_ERROR = 5    // Other errors
};

struct StorageStatus {
    CardState state;       // SD card status
    uint64_t free_space;   // Remaining space on the SD card
    uint64_t total_space;  // Total space on the SD card
};

StorageStatus status;
bool successed = camera->GetStorageState(status);
```

### **Current Camera Capture or Recording Status**

The interface **CaptureCurrentStatus** can be used to check whether the camera is in photo or video mode. "Capture" refers to taking photos.

```c++
auto ret= camera->CaptureCurrentStatus();
if (ret) {
    std::cout << "current statue : capture" << std::endl;
}
else {
    std::cout << "current statue : not capture" << std::endl;
}
```

### **Camera Information Notifications**

This section provides interfaces for the camera’s active notifications, which primarily involve low battery, SD card full, camera overheating, and recording stoppage.

```C++
// This interface is used to set the low battery notification
void SetBatteryLowNotification(BatteryLowCallBack callback);

// This interface is used to set the SD card full notification
void SetStorageFullNotification(StorageFullCallBack callback);

// This interface is used to set the camera recording interruption notification
void SetCaptureStoppedNotification(CaptureStoppedCallBack callback);

// This interface is used to set the camera high-temperature notification
void SetTemperatureHighNotification(TemperatureHighCallBack callback);
```

## **Preview Stream Functionality**

### **Setting the Preview Stream Data Delegate Interface**

The interface **ins\_camera::StreamDelegate** can be inherited to implement access to raw video streams, audio streams, gyro data, and exposure data. Refer to the following code:

```c++
class TestStreamDelegate : public ins_camera::StreamDelegate {
public:
    TestStreamDelegate() {}

    // Callback for audio data
    void OnAudioData(const uint8_t* data, size_t size, int64_t timestamp) override {}

    // Callback for video data
    void OnVideoData(const uint8_t* data, size_t size, int64_t timestamp, uint8_t streamType, int stream_index) override {
        if (stream_index == 0) {
            // First video stream
        }
        if (stream_index == 1) {
            // Second video stream
        }
    }

    // Callback for gyro data
    void OnGyroData(const std::vector<ins_camera::GyroData>& data) override {}

    // Callback for exposure data
    void OnExposureData(const ins_camera::ExposureData& data) override {}
};
```

1. For video data, if the resolution is less than 5.7K (5760×2280), the preview stream consists of a single video stream. If the resolution is 5.7K or higher, there will be **two video streams**. The data format is encoded in either H.265 or H.264, which can be retrieved using the interface **GetVideoEncodeType** to obtain the actual encoding format of the camera. A decoder can be created based on the encoding format to decode the data. The decoded data is not stitched together but consists of two fisheye images. Generally, the resolution of the preview stream is **1920×960**.

2. After implementing this delegate interface, an instance must be created and assigned to the camera instance using the **SetStreamDelegate** interface. The following code demonstrates this:

```c++
 auto delegate = std::make_shared<TestStreamDelegate>();
 camera->SetStreamDelegate(delegate);
```



### **Starting the Preview Stream**

The preview stream can be enabled by setting the preview stream parameters and calling the **StartLiveStreaming** interface. For the preview stream resolution, **1920×960** is currently recommended.

```c++
ins_camera::LiveStreamParam param;
param.video_resolution = ins_camera::VideoResolution::RES_1920_960P30;
param.lrv_video_resulution = ins_camera::VideoResolution::RES_1920_960P30;
param.video_bitrate = 1024 * 1024 / 2;
param.enable_audio = false;
param.using_lrv = false;
if (cam->StartLiveStreaming(param)) {
    std::cout << "successfully started live stream" << std::endl;
}
```

### **Stopping the Preview Stream**

Use the **StopLiveStreaming** interface to stop the preview stream.



## **Logging Feature**

### **Setting the Log Path**

This interface is mainly used to set the SDK’s log path, where the SDK’s log information can be saved.

```C++
void SetLogPath(const std::string log_path)
```

### **Setting the Log Level**

This interface is used to set the log level for the SDK’s log output.

```C++
void SetLogLevel(LogLevel level)
```

### **Retrieving the Camera** **Log** **Path**

This interface is used to retrieve the path of the camera’s log file. Once the path is obtained, you can use the download interface to download the log file locally.

```C++
std::string GetCameraLogFileUrl() const;
```

## **Others**

### **Retrieving Camera Media Time: GetCameraMediaTime()**

This interface retrieves the camera’s current media time, which can be used to synchronize with the tail-end information of recorded files.



### **Switch Camera Lens – SetActiveSensor**

This interface allows switching between different camera lenses.

 **SENSOR_DEVICE_FRONT(1)** – Switch to the lens on the screen side

 **SENSOR_DEVICE_REAR(2)** – Switch to the lens on the back side of the screen

 **SENSOR_DEVICE_ALL(3)** – Switch to panoramic mode

### **Turn Off the Camera (Only Supported on X5 and Later)**

```C++
void ShutdownCamera()
```
