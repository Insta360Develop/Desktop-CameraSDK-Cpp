# 简要

CameraSDK主要用于连接、设置和获取相机参数、控制相机进行拍照和录制、文件下载和固件升级(仅支持X4)等。仅支持通过usb进行连接相机。用于toB用户，主要支持的相机有ONE X，ONE X2、R/RS、X3、X4以及X5全景相机。主要支持的平台有windows、ubuntu22.04。

- 支持机型

| 型号                             | 连接                                                      |
| :------------------------------- | :-------------------------------------------------------- |
| ONE X (停产)                     | http://insta360.com/product/insta360-onex/                |
| ONE R Twin Edition (停产)        | http://insta360.com/product/insta360-oner_twin-edition    |
| ONE X2                           | https://www.insta360.com/product/insta360-onex2           |
| ONE RS 1-Inch 360 Edition (停产) | https://www.insta360.com/product/insta360-oners/1inch-360 |
| X3                               | https://www.insta360.com/product/insta360-x3              |
| X4                               | https://www.insta360.com/product/insta360-x4              |
| X5                               | https://www.insta360.com/cn/product/insta360-x5           |

- **支持平台**

| 平台    | 版本                                       |
| :------ | :----------------------------------------- |
| Windows | Windows 7 或更高版本，仅支持 x64           |
| Linux   | Ubuntu 22.04（x86_64），其他发行版需要测试 |

# 功能简要说明

* 环境准备

* 相机发现

* 相机连接和断连

* 拍照

* 录制

* 下载文件

* 获取文件信息

* 删除文件

* 获取录制中的文件名称

* 固件升级

* 状态查询

* 预览流功能

* 日记

* 其他

# 功能使用说明

## 环境准备

### 相机切换到安卓模式下

默认情况下，当你将Insta360相机连接到计算机时，相机会自动切换到U盘模式，使相机成为一个USB存储设备。 我们需要将相机切换到正确模式，才能连接并控制它。

#### For ONE X

你需要升级到一个特殊版本的固件，[在此处下载](https://insta360-dev.oss-cn-hangzhou.aliyuncs.com/developer/releases/a33b3362-4767-47c3-ba9d-6ed07febb210.zip) 

升级后，在相机上进入设置，找到USB选项，将其设置为**Android**模式。 

#### For ONE R/RS、ONE X2、X3

在摄像头界面，向下滑动屏幕进入主菜单，进入“设置”->“常规”，将USB模式设置为“Android”，将U盘模式设置为“Android”模式。  

#### For X4、X5

先连接usb线，然后弹出模式选择的界面，选择“**Android**”模式，等待切换成功。

### 驱动安装

**On Linux**, 请确保你的发行版已安装libusb。你可以通过yum或apt-get进行安装。

```Bash
sudo apt-get install libusb-dev
sudo apt-get install libudev-dev
```

或者从源代码构建

```Bash
wget http://sourceforge.net/projects/libusb/files/libusb-1.0/libusb-1.0.9/libusb-1.0.9.tar.bz2
tar xjf libusb-1.0.9.tar.bz2
cd libusb-1.0.9
./configure 
make
sudo make install
```

  安装驱动程序后，通过“lsusb”命令检查是否检测到摄像头。如果找到供应商ID为0x2e1a的任何USB设备，恭喜，您的驱动程序已成功安装。 

  **注意：在Linux系统上，演示程序必须使用“sudo”运行，例如**

```Bash
sudo ./CameraSDKDemo //for ubuntu
```

  **在Windows系统上，请确保已安装libusbK驱动程序。你可以直接安装[libusbK](https://sourceforge.net/projects/libusbk/files/libusbK-release/3.0.7.0/)，也可以使用[zadig](https://zadig.akeo.ie/)来协助安装libusbK驱动程序。**       

## 相机发现

相机发现主要通过**ins\_camera::DeviceDiscovery**这个接口去实现的

```C++
//示例代码
//这个DeviceDescriptor结构体主要保存了相机的基本信息，主要用于连接
struct DeviceDescriptor {
    CameraType camera_type;    //相机类型，比如x3或者x4
    std::string serial_number; //当前相机的序列号
    std::string fw_version;    //当前相机固件的版本号
    DeviceConnectionInfo info; //这个信息没有用，主要PC端的SDK仅支持USB连接
};

ins_camera::DeviceDiscovery discovery;
//遍历到的相机信息保存这个list中。
std::vector<DeviceDescriptor> list = discovery.GetAvailableDevices();
```

## 相机连接和断连

### 创建相机实例

在获取到相机信息后，通过接口**ins_camera::Camera** 和 **DeviceDescriptor** 去创建控制相机的实例**。**

```C++
// 从相机列表中获取创建实例所需要的信息
auto camera_info = list[0].info;
auto camera = std::make_shared<ins_camera::Camera>(camera_info);
//到此已经创建好一个相机实例
```

### 打开相机

接下来打开相机，可以通过接口**Open**去实现

```C++
bool success = camera->Open()；
if(!success) {
    std::cout << "failed to open camera" << std::endl;
    return -1;
}
```

### 相机断连

相机断连，可以通过接口**Close**去实现

> ⚠️ **注意：** 在使用升级固件按钮完成后，需要切换电源开关。等待升级成功后，再重新创建相机实例。

```C++
camera->Close();
```

### 判断相机是否连接

判断相机目前是否还在连接状态，请使用接口 **IsConnected**

注意：在相机切换状态下，避免调用此接口.

## 拍照

### 基本参数设置

#### 设置相机照片模式&#x20;

通过接口 **SetPhotoSubMode&#x20;**&#x8FDB;行各个照片模式的切换，下面是目前相机支持的模式，具体实际情况可以参考相机的界面。

```C++
enum SubPhotoMode {
    PHOTO_SINGLE = 0,          // 普通拍照
    PHOTO_HDR = 1,             // HDR拍照
    PHOTO_INTERVAL = 2,        // Interval拍照
    PHOTO_BURST = 3,           // Burst拍照 
    PHOTO_STARLAPSE = 7,       // 星空模式拍照
};

// 可以通过接口进行照片模式的切换
camera->SetPhotoSubMode(SubPhotoMode::PHOTO_SINGLE)
```

#### 设置分辨率大小

可以通过接口 **SetPhotoSize** 设置照片分辨率。目前，X4 和 X3 支持 72MP 和 18MP 两种格式。

接口如下：这个接口只能使用下面 **CameraFunctionMode&#x20;**&#x679A;举的中照片的模式。视频的模式是不支持PhotoSize这个接口的。

```c++
//照片大小
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

// 照片模式
enum CameraFunctionMode {
    FUNCTION_MODE_NORMAL = 0,                // 默认模式
    FUNCTION_MODE_INTERVAL_SHOOTING = 3,     // Interval拍照模式
    FUNCTION_MODE_BURST = 5,                 // Brust拍照模式
    FUNCTION_MODE_NORMAL_IMAGE = 6,          // 普通拍照模式
    FUNCTION_MODE_HDR_IMAGE = 8,             // HDR拍照模式  
    FUNCTION_MODE_AEB_NIGHT_IMAGE = 13,      
    FUNCTION_MODE_STARLAPSE_IMAGE = 18,      // 星空模式
    ...
};

// 设置普通拍照的照片大小为72MP
camera->SetPhotoSize(CameraFunctionMode::FUNCTION_MODE_NORMAL_IMAGE,PhotoSize::Size_11968_5984)
```

#### 曝光参数

曝光的接口是通过接口**ExposureSettings**去保存的 ,接口 **ExposureSettings 中**包含了ISO、快门、曝光模式以及曝光补偿。参考如下代码

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

  通过这个接口 **GetExposureSettings** 去获取各个模式下曝光参数。参考如下代码：

```C++
// 可以通过GetExposureSettings获取普通拍照模式的曝光参数
const auto funtion_mode = ins_camera::CameraFunctionMode::FUNCTION_MODE_NORMAL_IMAGE;
auto exposure_settings = camera->GetExposureSettings(funtion_mode);
auto iso = exposure_settings->Iso();
auto shutter_speed = exposure_settings->ShutterSpeed();
// ...
```

   通过这个接口 **SetExposureSettings** 去设置各个模式下的曝光参数。参数如下代码：

```C++
//1. 可以通过GetExposureSettings获取普通拍照模式的曝光参数
const auto funtion_mode = ins_camera::CameraFunctionMode::FUNCTION_MODE_NORMAL_IMAGE;
auto exposure_settings = camera->GetExposureSettings(funtion_mode);

//2. 设置参数
exposure_settings->SetEVBias(bias); // range -80 ~ 80, default 0, step 1
exposure_settings->SetIso(800);
exposure_settings->SetShutterSpeed(1.0 / 120.0);
 
//3. 发送参数可以相机 
camera->SetExposureSettings(funtion_mode,exposure_settings);
```

#### 白平衡参数

白平衡的接口通过这个接口 **CaptureSettings&#x20;**&#x53BB;保存的 ,这个接口 **CaptureSettings&#x20;**&#x4E2D;包含了白平衡等其他参数。

获取当前白平衡参数通过接口 GetCaptureSettings 去获取。参考如下代码：

```C++
// 可以通过GetCaptureSettings获取普通拍照模式的白平衡参数
const auto funtion_mode = ins_camera::CameraFunctionMode::FUNCTION_MODE_NORMAL_IMAGE;
auto capture_setting = camera->GetCaptureSettings(funtion_mode);
auto value = settings->GetIntValue(ins_camera::CaptureSettings::SettingsType::CaptureSettings_WhiteBalance);
```

设置白平衡参数可以通过接口 SetCaptureSettings 去设置。参考如下代码：

```C++
// 1. 获取当前参数
const auto funtion_mode = ins_camera::CameraFunctionMode::FUNCTION_MODE_NORMAL_IMAGE;
auto capture_setting = camera->GetCaptureSettings(funtion_mode);

// 2. 设置白平衡参数
capture_setting->SetWhiteBalance(ins_camera::PhotographyOptions_WhiteBalance::WB_6500K);

// 3. 设置给相机生效
camera->SetCaptureSettings(funtion_mode, capture_setting);
```

### 普通拍照

普通拍照可以通过以下参考代码:

```C++
// 设置为普通模式
camera->SetPhotoSubMode(SubPhotoMode::PHOTO_SINGLE)

// 设置为18M，这个是针对X3的。对于X4,需要设置为 Size_5952_2976
ins_camera::PhotoSize photo_size = ins_camera::PhotoSize::Size_6912_3456;
if (camera_type == ins_camera::CameraType::Insta360X4) {
    photo_size = ins_camera::PhotoSize::Size_5952_2976;
}
camera->SetPhotoSize(CameraFunctionMode::FUNCTION_MODE_NORMAL_IMAGE,photo_size);

//进行拍照
camera->takePhoto();
```

### HDR拍照

对于HDR拍照，我们单独提供了一个接口：**StartHDRCapture。**

 对于X3之前的相机，拍照完成后，获取到照片的数量，一般是3，5，7或者9张素材。

 对于X4相机以及之后的相机，拍照完成后，默认是获取到1张素材，并且这张素材已经是进行了HDR的融合操作，可以直接进行拼接。

```C++
ins_camera::PhotoSize photo_size = ins_camera::PhotoSize::Size_6912_3456;
if (camera_type == ins_camera::CameraType::Insta360X4) {
    photo_size = ins_camera::PhotoSize::Size_5952_2976;
}
camera->StartHDRCapture(photo_size);
```

## 录制

### 基本参数设置

#### 设置相机视频模式&#x20;

通过接口 **SetVideoSubMode**进行各个视频模式的切换，下面是目前相机支持的模式，具体实际情况可以参考相机的界面

```c++
enum SubVideoMode {
    VIDEO_NORMAL = 0,        // 普通录制
    VIDEO_BULLETTIME = 1,    // 子弹时间
    VIDEO_TIMELAPSE = 2,     // timelapse录制
    VIDEO_HDR = 3,           // HDR录制
    VIDEO_TIMESHIFT = 4,     // timeshift录制
    VIDEO_LOOPRECORDING = 6, // 循环录影
};

// 可以通过接口进行照片模式的切换
camera->SetVideoSubMode(SubVideoMode::VIDEO_NORMAL);
```

#### 设置分辨率大小

可以通过接口 **SetVideoCaptureParams**的设置录制时的参数

接口如下：这个接口只能使用下面 CameraFunctionMode 枚举的中视频的模式。**SetVideoCaptureParams&#x20;**&#x8FD9;个接口是不支持设置照片的模式的。

```c++
// 视频模式
enum CameraFunctionMode {
    FUNCTION_MODE_MOBILE_TIMELAPSE = 2,      // 移动延时视频模式
    FUNCTION_MODE_NORMAL_VIDEO = 7,          // 普通视频模式
    FUNCTION_MODE_HDR_VIDEO = 9,             // HDR视频模式
    FUNCTION_MODE_INTERVAL_VIDEO = 10,       // Interval视频模式
    FUNCTION_MODE_STATIC_TIMELAPSE = 11,     // 延时视频模式 
    FUNCTION_MODE_TIMESHIFT = 12,            // Timeshift模式    
    FUNCTION_MODE_LOOP_RECORDING_VIDEO = 17, // 循环录影模式
};

// 这个结构体定义录制时的分辨率、帧率和码率
struct RecordParams {
    VideoResolution resolution;
    int32_t bitrate{ 0 };
};

auto function_mode = ins_camera::CameraFunctionMode::FUNCTION_MODE_NORMAL_VIDEO;
ins_camera::RecordParams record_params;
// 设置5.7K60帧
record_params.resolution = ins_camera::VideoResolution::RES_2880_2880P60;
// 设置码率10MB，这个参数可能不生效，录制里面可能在某些挡位会写死码率
record_params.bitrate = 1024 * 1024 * 10;
if (!cam->SetVideoCaptureParams(record_params,function_mode )) {
    std::cerr << "failed to set capture settings." << std::endl;
}
```

#### 曝光参数

参考 4.1.3

#### 白平衡参数

参数4.1.4

### 普通录制

 普通录制可以通过以下参考代码进行演示

#### 开始录制

```C++
// 1.将模式切换为视频普通模式
bool ret = camera->SetVideoSubMode(ins_camera::SubVideoMode::VIDEO_NORMAL);
if (!ret) {
    std::cout << "change sub mode failed!" << std::endl;
    continue;
}

// 2.设置当前的分辨率和帧率以及码率
auto function_mode = ins_camera::CameraFunctionMode::FUNCTION_MODE_NORMAL_VIDEO;
ins_camera::RecordParams record_params;
// 设置5.7K60帧
record_params.resolution = ins_camera::VideoResolution::RES_2880_2880P60;
record_params.bitrate = 1024 * 1024 * 10;
// 设置码率10MB，这个参数可能不生效，相机里面可能在某些挡位或者某些机型会写死码率，可以不进行设置
if (!camera->SetVideoCaptureParams(record_params, function_mode)) {
    std::cerr << "failed to set capture settings." << std::endl;
}
else {
// 3. 开始录制
    const auto ret = camera->StartRecording();
    if (ret) {
        std::cerr << "success!" << std::endl;
    }
    else {
        std::cerr << "failed to start recording" << std::endl;
    }
}
```

#### 停止录制

```C++
// 停止录制操作
auto url = cam->StopRecording();
if (url.Empty()) {
    std::cerr << "stop recording failed" << std::endl;
    continue;
}
// 获取到录制素材在相机中地址
auto& origins = url.OriginUrls();
std::cout << "stop recording success" << std::endl;
for (auto& origin_url : origins) {
    std::cout << "url:" << origin_url << std::endl;
}
```

### Timelapse录制

Timelapse录制可以通过以下参考代码进行演示:

这个表是timelapse下目前相机支持的分辨率帧率的枚举对应关系

| 分辨率/帧率 |      X4、X5      |        X3        | ONE X2 |  R   |  RS  | ONE X |
| :---------: | :--------------: | :--------------: | :----: | :--: | :--: | :---: |
|  **11K30**  | RES_5632_5632P30 |      不支持      |  Todo  | Todo | Todo | Todo  |
|  **8K30**   | RES_3840_3840P30 | RES_3840_3840P30 |  Todo  | Todo | Todo | Todo  |
| **5.7K30**  | RES_2880_2880P30 | RES_2880_2880P30 |  Todo  | Todo | Todo | Todo  |

#### 开始录制

```C++
// 1.将相机模式切换到timelapse模式
bool ret = cam->SetVideoSubMode(ins_camera::SubVideoMode::VIDEO_TIMELAPSE);
if (!ret) {
    std::cout << "change sub mode failed!" << std::endl;
    continue;
}

// 2. 设置当前模式的分辨率和帧率
auto function_mode = ins_camera::CameraFunctionMode::FUNCTION_MODE_MOBILE_TIMELAPSE;
ins_camera::RecordParams record_params;
// 8K30的分辨率
record_params.resolution = ins_camera::VideoResolution::RES_3840_3840P30;
if (!cam->SetVideoCaptureParams(record_params, function_mode )) {
    std::cerr << "failed to set capture settings." << std::endl;
    break;
}

// 3. 设置延时摄影的参数
auto timelapse_mode = ins_camera::CameraTimelapseMode::MOBILE_TIMELAPSE_VIDEO;
const uint32_t record_duration_s = 10; // 拍摄时间为10秒
const uint32_t lapseTime_ms = 1000;    // 间隔时长为1秒
ins_camera::TimelapseParam param = { timelapse_mode, 10,1000,0 };
if (!cam->SetTimeLapseOption(param)) {
    std::cerr << "failed to set capture settings." << std::endl;
}
else {
// 4. 开始录制
    const auto ret = cam->StartTimeLapse(param.mode);
    if (ret) {
        std::cerr << "success!" << std::endl;
    }
    else {
        std::cerr << "failed to start timelapse" << std::endl;
    }
}
```

#### 停止录制

```C++
auto timelapse_mode = ins_camera::CameraTimelapseMode::MOBILE_TIMELAPSE_VIDEO;
auto url = cam->StopTimeLapse(timelapse_mode );
if (url.Empty()) {
    std::cerr << "stop timelapse failed" << std::endl;
    continue;
}
// 获取到录制素材在相机中地址
auto& origins = url.OriginUrls();
std::cout << "stop timelapse success" << std::endl;
for (auto& origin_url : origins) {
    std::cout << "url:" << origin_url << std::endl;
}
```

## 获取文件信息

### 获取文件个数

通过接口 **GetCameraFilesCount&#x20;**&#x83B7;取当前相机SD卡中录制的文件个数

### 获取文件列表

通过接口 **GetCameraFilesList&#x20;**&#x83B7;取当前相机SD卡中录制素材的列表.具体使用参考以下代码

```c++
auto file_list = camera->GetCameraFilesList();
for(const auto& file : file_list) {
    std::cout << file << std::endl;
}

//print output 
/DCIM/Camera01/VID_20250122_071405_00_001.insv
/DCIM/Camera01/LRV_20250122_071405_01_001.lrv
/DCIM/Camera01/VID_20250214_063916_00_002.insv
/DCIM/Camera01/LRV_20250214_063916_01_002.lrv
```

## 获取录制中的文件列表

通过接口 **GetRecordingFiles&#x20;**&#x53EF;以当前相机中正在录制的文件名称

## 文件下载

### 下载文件

通过接口 `DownloadCameraFile` 可以将相机 SD 卡中已经存在的素材下载到本地。

通过设置回调可以获取当前文件的下载进度，这个接口是同步调用，下载完成或者下载失败后才能退出。以下是演示代码：

> **注意：**
>
> 1. 在下载之前，确保创建好下载到本地的路径目录
> 2. 由于在SDK中启动了HttpServer, 可能会存在端口占用导致服务启动失败的情况，SDK中提供了 **SetServicePort** 这个接口去避免这个问题。SDK中默认的端口号是 9099。端口占用的情况需要调用者去进行判断。如果有占用的情况，需要在**Open**接口调用前进行端口设置。

```C++
//相机的文件路径：
std::string camera_file = "/DCIM/Camera01/VID_20250122_071405_00_001.insv";
//本地的文件路径：
std::string local_save_file = "/path/to/local/VID_20250122_071405_00_001.insv";

//开始下载
bool ret = camera->DownloadCameraFile(camera_file, local_save_file, [](int64_t current, int64_t total_size) {
    // 展示进度
    std::cout << "current :" << current << "; total_size: " << total_size << std::endl;
});

// 下载结束
if(ret) {
   std::cout << "successed to download file" << std::end; 
}
```

### 取消下载

通过接口 **CancelDownload&#x20;**&#x53EF;以取消正在下载的文件

## 删除文件

通过接口 **DeleteCameraFile&#x20;**&#x5220;除SD卡中不需要的文件

```C++
const auto camera_file = "/DCIM/Camera01/VID_20250122_071405_00_001.insv";
camera_file->DeleteCameraFile(camera_file);
```

## 固件升级(仅适用于X4以及后续机型)

通过接口 **UploadFile&#x20;**&#x53BB;进行固件版本的升级，目前仅适用于X4以及后续机型，固件的相机名称为：**Insta360X4FW.bin** 这个是定义好的. 下面是演示代码：

```c++
//固件名称
const std::string fireware_name = "Insta360X4FW.bin";
std::string file_name = "Insta360X4FW.bin";
if (camera_type == ins_camera::CameraType::Insta360X5) {
    file_name = "Insta360X5FW.bin";
}

const auto ret = cam->UploadFile(local_path, file_name,
//固件的本地保存路径
const std::string local_file = "/path/to/fireware/Insta360X4FW.bin"；
 
// 开始上传
bool ret = cam->UploadFile(local_file, fireware_name ,
    [](int64_t current, int64_t total_size) {
      // 固件上传进度展示
      std::cout << "current :" << current << ";total_size: " << total_size << std::endl;
});

if (ret) {
    std::cout << "succeed to Upload file !!!" << std::endl;
}

// 上传成功后，必须关闭相机，等待相机升级成功后重新创建相机实例(原因是相机升级需要重新启动)
camera->close();
```

## 状态查询

### 电池状态信息

通过接口 **GetBatteryStatus&#x20;**&#x53EF;以获取当前相机的电池信息，可以获取当前电池的使用情况，比如电池电量

```c++
//这些定义都有SDK的头文件中提供
enum PowerType {
    BATTERY = 0,
    ADAPTER = 1,
};

struct BatteryStatus {
    PowerType power_type;   //电源类型
    uint32_t battery_level; //这个就是当前的电池电量(0~100)
    uint32_t battery_scale; //这个值无用
};

BatteryStatus status;
bool successed = camera->GetBatteryStatus(status);
```

### SD卡存储信息

通过接口 **GetStorageState&#x20;**&#x53EF;以获取到当前SD卡的状态. 可以获取当前SD卡的状态以及空间大小情况

```c++
//这些定义都有SDK的头文件中提供
 enum CardState {
    STOR_CS_PASS = 0,          // SD卡可以正常使用
    STOR_CS_NOCARD = 1,        // 相机中没有SD卡
    STOR_CS_NOSPACE = 2,       // SD卡没有剩余空间
    STOR_CS_INVALID_FORMAT = 3,// SD卡的格式不对
    STOR_CS_WPCARD = 4,
    STOR_CS_OTHER_ERROR = 5    // 其他错误
};
    
struct StorageStatus {
    CardState state;
    uint64_t free_space;  // SD卡的剩余空间
    uint64_t total_space; // SD卡的总空间
};

StorageStatus status;
bool successed = camera->GetStorageState(status);
```

### 当前相机捕获或者录制状态

通过接口 CaptureCurrentStatus 可以查询状态是否处于捕获或者录制状态。

```c++
auto ret= camera->CaptureCurrentStatus();
if (ret) {
    std::cout << "current statue : capture" << std::endl;
}
else {
    std::cout << "current statue : not capture" << std::endl;
}
```

### 相机信息通知

 这部分的接口主要来自于相机的主动通知，涉及到低电量、SD卡满、相机高温以及相机录制异常中断

```c++
//这个接口用于通知当前相机电量低
void SetBatteryLowNotification(BatteryLowCallBack callback);

//这个接口用于通知当前相机SD卡已经满了
void SetStorageFullNotification(StorageFullCallBack callback);

//这个接口用于通知录制异常中断
void SetCaptureStoppedNotification(CaptureStoppedCallBack callback);

//这个接口用于通知相机温度过高
void SetTemperatureHighNotification(TemperatureHighCallBack callback);
```

## 预览流功能

### 设置预览流数据的委托接口

可以继承接口 **ins\_camera::StreamDelegate&#x20;**&#x6765;实现获取原始视频流、音频流、防抖数据以及曝光数据。可以参考一下代码：

```c++
class TestStreamDelegate : public ins_camera::StreamDelegate {
public:
    TestStreamDelegate() {}

    // 音频数据的回调
    void OnAudioData(const uint8_t* data, size_t size, int64_t timestamp) override {}

    // 视频数据的回调
    void OnVideoData(const uint8_t* data, size_t size, int64_t timestamp, uint8_t streamType, int stream_index) override {
        if (stream_index == 0) {
            // 第一路视频
        }
        if (stream_index == 1) {
            // 第二路视频
        }
    }
    
    // 防抖数据的回调
    void OnGyroData(const std::vector<ins_camera::GyroData>& data) override {}
    
    // 曝光数据的回调
    void OnExposureData(const ins_camera::ExposureData& data) override {}
    
};
```

1、对于视频数据，小于5.7k(5760x2280)的预览流只有一路视频，大于等于5.7k才有两路视频。数据的格式是h265或者h264编码的，可以通过接口 **GetVideoEncodeType&#x20;**&#x83B7;取到当前相机的实际编码格式。通过编码格式创建解码器进行数据的解码，解码后的数据是没有进行拼接的，是两个鱼眼图片。一般情况下，预览流的分辨率都是1920x960的。

&#x20;2、这个委托接口实现后，需要生成实例后，通过接口 **SetStreamDelegate&#x20;**&#x8BBE;置给相机实例。如下代码：

```c++
 auto delegate = std::make_shared<TestStreamDelegate>();
 camera->SetStreamDelegate(delegate);
```

### 开启预览流

可以通过设置预览流参数和接口 **StartLiveStreaming&#x20;**&#x5F00;启预览流功能,对于预览流的分辨率，目前建议使用1920x960

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

### 关闭预览流

通过接口 **StopLiveStreaming&#x20;**&#x53BB;关闭预览流功能.

## 日记功能

### 设置日记路径

这个接口主要用于设置SDK中的日记路径，可以保存SDK中的日记信息

```C++
void SetLogPath(const std::string log_path)
```

### 设置日记打印等级

这个接口用于设置SDK中日记打印的等级

```C++
void SetLogLevel(LogLevel level)
```

### 获取相机日记路径

   这个接口用于获取相机中的日记路径，获取后可以通过下载接口将日记下载到本地

```C++
std::string GetCameraLogFileUrl() const;
```

## 其他

### 获取相机mediaTime

这个接口可以获取相机中当前的media时间，可以和录制文件的文件尾信息做时间同步。

```C++
int64_t GetCameraMediaTime()
```

### 切换相机镜头 SetActiveSensor

可以通过这个接口去进行镜头切换

> SENSOR\_DEVICE\_FRONT(1)  切换到屏幕这边的镜头

> SENSOR\_DEVICE\_REAR (2)   切换到屏幕后面的镜头

> SENSOR\_DEVICE\_ALL (3)      切换到全景模式

### 关闭相机(仅支持X5之后的相机)

```C++
void ShutdownCamera()
```
