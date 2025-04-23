#include <iostream>
#include <thread>
#include <regex>
#include <iomanip>
#include <sstream>
#include <string>
#include <thread>
#include <chrono>
#include <csignal>
#include <camera/camera.h>
#include <camera/photography_settings.h>
#include <camera/device_discovery.h>

#ifdef _WIN32
#include <io.h>
#define ACCESS_FUNC _access
#define F_OK 0
#else
#include <unistd.h>
#include <time.h>
#define ACCESS_FUNC access
#endif

std::shared_ptr<ins_camera::Camera> cam = nullptr;

std::string getCurrentTime() {
    const auto now = std::chrono::system_clock::now();
    const std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
#ifdef WIN32
    if (localtime_s(&tm, &t))
#else
    if (localtime_r(&t, &tm))
#endif
    {
    }
    std::stringstream ss;
    ss << std::put_time(&tm, "%Y-%m-%d%H%M%S");
    return ss.str();
}

bool fileIsExist(const std::string& file_path) {
    return ACCESS_FUNC(file_path.c_str(), F_OK) == 0;
}

bool endsWithSlash(const std::string& path) {
    if (path.empty()) {
        return false;
    }
    return path.back() == '/' || path.back() == '\\';
}

void parsePath(const std::string& path, std::string& dir, std::string& filename) {
    // 查找最后一个路径分隔符（支持'/'和'\'）
    size_t lastSlash = path.find_last_of("/\\");

    if (lastSlash == std::string::npos) {
        // 没有找到分隔符，整个字符串作为文件名
        dir.clear();
        filename = path;
    }
    else {
        // 分隔符前的部分为路径，包含分隔符
        dir = path.substr(0, lastSlash + 1);
        // 分隔符后的部分为文件名
        filename = path.substr(lastSlash + 1);
    }
}

std::string GetFileName(const std::string& path) {
    std::smatch sm;
    std::string dir, name;
    std::regex_match(path, sm, std::regex("(.+?)([^\\/\\\\]+$)"));
    if (sm.size() <= 2) {
        return path;
    }
    return sm[2].str();
}

class TestStreamDelegate : public ins_camera::StreamDelegate {
public:
    TestStreamDelegate() = default;

    virtual ~TestStreamDelegate() {
        StopStream();
    }

    void OnAudioData(const uint8_t* data, size_t size, int64_t timestamp) override {
    }

    void OnVideoData(const uint8_t* data, size_t size, int64_t timestamp, uint8_t streamType, int stream_index) override {
        if (stream_index == 0 && fp1) {
            fwrite(data, size, 1, fp1);
        }
        if (stream_index == 1 && fp2) {
            fwrite(data, size, 1, fp2);
        }
    }

    void OnGyroData(const std::vector<ins_camera::GyroData>& data) override {}

    void OnExposureData(const ins_camera::ExposureData& data) override {}

    void StopStream() {
        if (fp1) {
            fclose(fp1);
            fp1 = nullptr;
        }

        if (fp2) {
            fclose(fp2);
            fp2 = nullptr;
        }
    }

    void StartStream() {
        StopStream();
        const std::string cur_time = getCurrentTime();
        const std::string stream_1 = std::string("./01_") + cur_time + std::string(".h264");
        const std::string stream_2 = std::string("./02_") + cur_time + std::string(".h264");
#ifdef WIN32
        const errno_t err1 = fopen_s(&fp1, stream_1.c_str(), "wb");
        if (err1 != 0) {
            std::cerr << "failed to create file " << std::endl;
        }
        const errno_t err2 = fopen_s(&fp2, stream_2.c_str(), "wb");
        if (err2 != 0) {
            std::cerr << "failed to create file " << std::endl;
        }
#else
        fp1 = fopen(stream_1.c_str(), "wb");
        fp2 = fopen(stream_2.c_str(), "wb");
#endif
    }

private:
    FILE* fp1 = nullptr;
    FILE* fp2 = nullptr;
};

void signalHandle(int sig) {
    if (sig == SIGINT || sig == SIGTERM) {
        std::cout << "signal handler: " << sig << std::endl;
        if (cam) {
            cam->Close();
        }
    }
}

int main(int argc, char* argv[]) {
    (void)(signal(SIGINT, signalHandle));
    (void)(signal(SIGTERM, signalHandle));

    std::cout << "begin open camera" << std::endl;
    ins_camera::SetLogLevel(ins_camera::LogLevel::ERR);
    for (int i = 1; i < argc; i++) {
        const std::string arg = argv[i];
        if (arg == std::string("--debug")) {
            ins_camera::SetLogLevel(ins_camera::LogLevel::VERBOSE);
        }
        else if (arg == std::string("--log_file")) {
            const std::string log_file = argv[++i];
            ins_camera::SetLogPath(log_file);
        }
    }

    ins_camera::DeviceDiscovery discovery;
    auto list = discovery.GetAvailableDevices();
    if (list.empty()) {
        std::cerr << "no device found." << std::endl;
        return -1;
    }

    for (const auto& camera : list) {
        std::cout << "serial:" << camera.serial_number << "\t"
            << ";camera type:" << camera.camera_name << "\t"
            << ";fw version:" << camera.fw_version << "\t"
            << std::endl;
    }

    cam = std::make_shared<ins_camera::Camera>(list[0].info);
    if (!cam->Open()) {
        std::cerr << "failed to open camera" << std::endl;
        return -1;
    }

    const auto camera_type = list[0].camera_type;
    const auto serial_number = list[0].serial_number;

    discovery.FreeDeviceDescriptors(list);

    std::shared_ptr<ins_camera::StreamDelegate> delegate = std::make_shared<TestStreamDelegate>();
    cam->SetStreamDelegate(delegate);

    std::cout << "Succeed to open camera..." << std::endl;

    std::cout << "Usage:" << std::endl;
    std::cout << "1: get file list(only video and photo)" << std::endl;
    std::cout << "2: delete file" << std::endl;
    std::cout << "3: download file" << std::endl;
    std::cout << "4: switch video mode" << std::endl;
    std::cout << "5: switch photo mode" << std::endl;
    std::cout << "6: start normal video mode recording" << std::endl;
    std::cout << "7: stop recording" << std::endl;
    std::cout << "8: start timelapse " << std::endl;
    std::cout << "9: stop timelapse " << std::endl;
    std::cout << "10: start preview live streaming:" << std::endl;
    std::cout << "11: stop preview live streaming:" << std::endl;
    std::cout << "12: enable In-camera stitching (Compatible with X4 and later camera only)" << std::endl;
    std::cout << "13: set EV" << std::endl;
    std::cout << "14: set ISO" << std::endl;
    std::cout << "15: set shutter" << std::endl;
    std::cout << "16: set WB" << std::endl;
    std::cout << "17: set expouse mode:" << std::endl;
    std::cout << "18: get current capture status " << std::endl;
    std::cout << "19: test take photo and download " << std::endl;
    std::cout << "20: start recording " << std::endl;
    std::cout << "21: get battery info " << std::endl;
    std::cout << "22: get storage info " << std::endl;
    std::cout << "23: get recording file " << std::endl;
    std::cout << "24: Check if the camera is still connected" << std::endl;
    std::cout << "25: take photo " << std::endl;
    std::cout << "30: batch download list files " << std::endl;
    std::cout << "31: delete camera all files " << std::endl;
    std::cout << "34: start hdr capture " << std::endl;
    std::cout << "33: get file list count " << std::endl;
    std::cout << "35: upload Firmware (Compatible with X4 and later camera only)" << std::endl;
    std::cout << "36: set active sensor (switch lens)" << std::endl;
    std::cout << "37: get media time from camera " << std::endl;
    std::cout << "38: Shutdown camera " << std::endl;
    std::cout << "39: Get camera log" << std::endl;
    std::cout << "0: exit" << std::endl;

    time_t now = time(nullptr);
    std::tm tm{};
    time_t time_seconds = now;
#ifdef WIN32
    (void)(localtime_s(&tm, &now));
    time_seconds = _mkgmtime(&tm);
#else
    localtime_r(&now, &tm);
    time_seconds = timegm(&tm);
#endif

    std::cout << "UTC time: " << now << std::endl;
    std::cout << "GMT time: " << time_seconds << std::endl;
    cam->SyncLocalTimeToCamera(time_seconds);

    /*************************demo for 普通照片72MP和 HDR录制5.7K******************/
    //cam->SetPhotoSize(ins_camera::FUNCTION_MODE_NORMAL_IMAGE, ins_camera::PhotoSize::Size_6912_3456);
    //ins_camera::RecordParams record_params_test;
    //record_params_test.resolution = ins_camera::VideoResolution::RES_2880_2880P30;
    //cam->SetVideoCaptureParams(record_params_test, ins_camera::CameraFunctionMode::FUNCTION_MODE_HDR_VIDEO);
    /*************************demo for 普通照片72MP和 HDR录制5.7K******************/

    ins_camera::CameraFunctionMode FuntionMode = ins_camera::CameraFunctionMode::FUNCTION_MODE_NORMAL_IMAGE;

    int option = 0;
    while (true) {
        std::cout << "please enter index: ";
        std::cin >> option;
        if (option < 0 || option > 39) {
            std::cout << "Invalid index" << std::endl;
            continue;
        }

        if (option == 0) {
            break;
        }

        if (option == 1) {
            const auto file_list = cam->GetCameraFilesList();
            for (const auto& file : file_list) {
                std::cout << "File: " << file << std::endl;
            }
        }

        if (option == 2) {
            const auto file_list = cam->GetCameraFilesList();
            for (const auto& file : file_list) {
                std::cout << "File: " << file << std::endl;
            }

            std::string file_to_delete;
            std::cout << "please input full file path to delete: ";
            std::cin >> file_to_delete;
            if (file_to_delete.empty()) {
                std::cout << "Invalid argument" << std::endl;
                continue;
            }

            auto exist = std::find_if(file_list.begin(), file_list.end(), [file_to_delete](const std::string& file)
            {
                return  file_to_delete == file;
            }) != file_list.end();

            if (!exist) {
                std::cout << "the file that deleted is not exist " << std::endl;
                continue;
            }

            const auto ret = cam->DeleteCameraFile(file_to_delete);
            if (ret) {
                std::cout << "deleted succeed" << std::endl;
            }
        }

        if (option == 3) {
            const auto file_list = cam->GetCameraFilesList();
            for (const auto& file : file_list) {
                std::cout << "File: " << file << std::endl;
            }

            std::string file_to_download;
            std::string file_to_save;
            std::cout << "please input full file path to download: ";
            std::cin >> file_to_download;

            bool exist = std::find_if(file_list.begin(), file_list.end(), [file_to_download](const std::string& file)
            {
                return  file_to_download == file;
            }) != file_list.end();

            if (!exist) {
                std::cout << "the file that deleted is not exist " << std::endl;
                continue;
            }

            std::cout << "please input full file path to save: ";
            std::cin >> file_to_save;

            std::string file_to_save_dir;
            std::string file_to_save_file_name;
            parsePath(file_to_save, file_to_save_dir, file_to_save_file_name);
            if (!fileIsExist(file_to_save_dir)) {
                std::cout << "dir is not exist, please create it: " << file_to_save_dir << std::endl;
                continue;
            }

            if (file_to_save_file_name.empty()) {
                std::cout << "please input file name" << std::endl;
                continue;
            }

            int64_t current_process = -1;
            const auto ret = cam->DownloadCameraFile(file_to_download,
                file_to_save, [&](int64_t current, int64_t total_size) {
                const int64_t  process = current * 100 / total_size;
                if (process != current_process) {
                    std::cout << "\r";
                    std::cout << "process = " << process << "%";
                    std::cout << std::flush;
                }
            });
            std::cout << std::endl;
            if (ret) {
                std::cout << "Download " << file_to_download << " succeed!!!" << std::endl;
            }
            else {
                std::cout << "Download " << file_to_download << " failed!!!" << std::endl;
            }
        }

        if (option == 4) {
            std::vector<int> support_mode = { 0,2,3,4,11 };
            std::cout << "normal video      [0]" << std::endl;
            std::cout << "timeplase video   [2]" << std::endl;
            std::cout << "hdr video         [3]" << std::endl;
            std::cout << "timeshift video   [4]" << std::endl;
            std::cout << "pure video        [11]" << std::endl;
            int video_mode = 0;
            std::cout << "Please enter the video mode: ";
            std::cin >> video_mode;
            if (std::find(support_mode.begin(), support_mode.end(), video_mode) == support_mode.end()) {
                std::cerr << "The input video mode is not supported" << std::endl;
                continue;
            }
            bool ret = cam->SetVideoSubMode(static_cast<ins_camera::SubVideoMode>(video_mode));
            if (ret) {
                std::cout << "Change Sub mode Succeed!" << std::endl;
            }
            else {
                std::cout << "Change Sub mode Failed!" << std::endl;
            }
        }

        if (option == 5) {
            std::cout << "normal photo      [0]" << std::endl;
            std::cout << "HDR photo         [1]" << std::endl;
            std::cout << "INTERVAL photo    [2]" << std::endl;

            int photo_mode = 0;
            std::cout << "Please enter the photo mode: ";
            std::cin >> photo_mode;
            if (photo_mode > 7) {
                std::cerr << "The input photo mode is not supported" << std::endl;
                continue;
            }
            bool ret = cam->SetPhotoSubMode(static_cast<ins_camera::SubPhotoMode>(photo_mode));
            if (ret) {
                std::cout << "Change Sub mode Succeed!" << std::endl;
            }
            else {
                std::cout << "Change Sub mode Failed!" << std::endl;
            }
        }

        if (option == 6) {
            bool ret = cam->SetVideoSubMode(ins_camera::SubVideoMode::VIDEO_NORMAL);
            if (!ret) {
                std::cout << "change sub mode failed!" << std::endl;
                continue;
            }

            ins_camera::RecordParams record_params;
            record_params.resolution = ins_camera::VideoResolution::RES_3840_3840P30;
            record_params.bitrate = 1024 * 1024 * 10;
            if (!cam->SetVideoCaptureParams(record_params, ins_camera::CameraFunctionMode::FUNCTION_MODE_NORMAL_VIDEO)) {
                std::cerr << "failed to set capture settings." << std::endl;
            }
            else {
                ret = cam->StartRecording();
                if (ret) {
                    std::cerr << "success!" << std::endl;
                }
                else {
                    std::cerr << "failed to start recording" << std::endl;
                }
            }
        }

        if (option == 7) {
            auto url = cam->StopRecording();
            if (url.Empty()) {
                std::cerr << "stop recording failed" << std::endl;
                continue;
            }
            auto& origins = url.OriginUrls();
            std::cout << "stop recording success" << std::endl;
            for (auto& origin_url : origins) {
                std::cout << "url:" << origin_url << std::endl;
            }
        }

        if (option == 8) {
            bool ret = cam->SetVideoSubMode(ins_camera::SubVideoMode::VIDEO_TIMELAPSE);
            if (!ret) {
                std::cout << "change sub mode failed!" << std::endl;
                continue;
            }

            // 11K30 RES_5632_5632P30
            // 8K    RES_3840_3840P30
            // 5.7K  RES_2880_2880P30
            // X4支持11K30, X3不支持
            std::vector<ins_camera::VideoResolution> support_list = {
                ins_camera::VideoResolution::RES_5632_5632P30,
                ins_camera::VideoResolution::RES_3840_3840P30,
                ins_camera::VideoResolution::RES_2880_2880P30
            };

            static int count = 0;
            ins_camera::RecordParams record_params;
            record_params.resolution = support_list[(count++) % 3];
            if (!cam->SetVideoCaptureParams(record_params, ins_camera::CameraFunctionMode::FUNCTION_MODE_MOBILE_TIMELAPSE)) {
                std::cerr << "failed to set capture settings." << std::endl;
                break;
            }

            //mode 是你相机所支持的模式
            ins_camera::TimelapseParam param = { ins_camera::CameraTimelapseMode::MOBILE_TIMELAPSE_VIDEO, 10,5000,5 };
            if (!cam->SetTimeLapseOption(param)) {
                std::cerr << "failed to set capture settings." << std::endl;
            }
            else {
                ret = cam->StartTimeLapse(param.mode);
                if (ret) {
                    std::cerr << "success!" << std::endl;
                }
                else {
                    std::cerr << "failed to start timelapse" << std::endl;
                }
            }
        }

        if (option == 9) {
            auto url = cam->StopTimeLapse(ins_camera::CameraTimelapseMode::MOBILE_TIMELAPSE_VIDEO);
            if (url.Empty()) {
                std::cerr << "stop timelapse failed" << std::endl;
                continue;
            }

            std::cout << "stop timelapse success" << std::endl;
            auto& origins = url.OriginUrls();
            for (auto& origin_url : origins) {
                std::cout << "url:" << origin_url << std::endl;
            }
        }

        if (option == 10) {
            // 对于X4相机 设置预览流，相机里面固定写死的，设置也不会生效
            // 对于X3相机，支持  RES_3840_1920P30 和  RES_1440_720P30
            // 开启 using_lrv 为true时。获取到的分辨率时1024x512.
            ins_camera::LiveStreamParam param;
            param.video_resolution = ins_camera::VideoResolution::RES_3840_1920P30;
            param.lrv_video_resulution = ins_camera::VideoResolution::RES_1440_720P30;
            param.video_bitrate = 1024 * 1024 / 2;
            param.enable_audio = false;
            param.using_lrv = false;

            auto stream_delegate = std::dynamic_pointer_cast<TestStreamDelegate>(delegate);
            if (stream_delegate) {
                stream_delegate->StartStream();
            }

            if (cam->StartLiveStreaming(param)) {
                std::cout << "successfully started live stream" << std::endl;
            }
        }

        if (option == 11) {
            if (cam->StopLiveStreaming()) {
                auto stream_delegate = std::dynamic_pointer_cast<TestStreamDelegate>(delegate);
                if (stream_delegate) {
                    stream_delegate->StopStream();
                }
                std::cout << "success!" << std::endl;
            }
            else {
                std::cerr << "failed to stop live." << std::endl;
            }
        }

        if (option == 12) {
            if (camera_type < ins_camera::CameraType::Insta360X4) {
                std::cout << "In-camera stitching is only supported on X4/X5" << std::endl;
                continue;
            }
            int enable = 0;
            std::cout << "In-camera stitching: Enable (1) / Disable (0)?" << std::endl;
            std::cin >> enable;
            if (cam->EnableInCameraStitching(enable ? true : false)) {
                std::cout << "In-camera stitching function activated successfully." << std::endl;
            }
        }

        if (option == 13) {
            auto exposure_settings = cam->GetExposureSettings(FuntionMode);
            if (exposure_settings) {
                std::cout << "EVBias : " << exposure_settings->EVBias() << std::endl;
                std::cout << "ISO    : " << exposure_settings->Iso() << std::endl;
                std::cout << "speed  : " << exposure_settings->ShutterSpeed() << std::endl;
                std::cout << "mode   : " << static_cast<int>(exposure_settings->ExposureMode()) << std::endl;
                std::cout << "videoISOLimit: " << exposure_settings->VideoISOTopLimit() << std::endl;
            }

            auto exposure_mode = exposure_settings->ExposureMode();
            if (exposure_mode != ins_camera::PhotographyOptions_ExposureMode::AUTO && exposure_mode != ins_camera::PhotographyOptions_ExposureMode::FULL_AUTO) {
                std::cout << "please switch exposure mode to AUTO mode or FULL_AUTO mode " << std::endl;
                continue;
            }

            int bias;
            std::cout << "please enter EVBIOS: ";
            std::cin >> bias;

            exposure_settings->SetEVBias(bias);
            auto ret = cam->SetExposureSettings(FuntionMode, exposure_settings);
            if (ret) {
                exposure_settings = cam->GetExposureSettings(FuntionMode);
                std::cout << "success! bias " << exposure_settings->EVBias() << std::endl;
            }
        }

        if (option == 14) {
            auto exposure_settings = cam->GetExposureSettings(FuntionMode);
            if (exposure_settings) {
                std::cout << "EVBias : " << exposure_settings->EVBias() << std::endl;
                std::cout << "ISO    : " << exposure_settings->Iso() << std::endl;
                std::cout << "speed  : " << exposure_settings->ShutterSpeed() << std::endl;
                std::cout << "mode   : " << exposure_settings->ExposureMode() << std::endl;
                std::cout << "videoISOLimit: " << exposure_settings->VideoISOTopLimit() << std::endl;
            }

            // ISO只有在手动模式或者优先挡位才可以设置
            auto exposure_mode = exposure_settings->ExposureMode();
            if (exposure_mode != ins_camera::PhotographyOptions_ExposureMode::MANUAL && exposure_mode != ins_camera::PhotographyOptions_ExposureMode::ISO_PRIORITY) {
                std::cout << "please switch exposure mode to MANUAL mode or ISO_PRIORITY mode " << std::endl;
                continue;
            }

            int iso;
            std::cout << "please enter ISO: ";
            std::cin >> iso;
            exposure_settings->SetIso(iso);
            auto ret = cam->SetExposureSettings(FuntionMode, exposure_settings);
            if (ret) {
                exposure_settings = cam->GetExposureSettings(FuntionMode);
                std::cout << "success! ISO " << exposure_settings->Iso() << std::endl;
            }
        }

        if (option == 15) {
            auto exposure_settings = cam->GetExposureSettings(FuntionMode);
            if (exposure_settings) {
                std::cout << "EVBias : " << exposure_settings->EVBias() << std::endl;
                std::cout << "ISO    : " << exposure_settings->Iso() << std::endl;
                std::cout << "speed  : " << exposure_settings->ShutterSpeed() << std::endl;
                std::cout << "mode   : " << exposure_settings->ExposureMode() << std::endl;
                std::cout << "videoISOLimit: " << exposure_settings->VideoISOTopLimit() << std::endl;
            }

            // 快门只有在手动模式或者优先挡位才可以设置
            auto exposure_mode = exposure_settings->ExposureMode();
            if (exposure_mode != ins_camera::PhotographyOptions_ExposureMode::MANUAL && exposure_mode != ins_camera::PhotographyOptions_ExposureMode::SHUTTER_PRIORITY) {
                std::cout << "please switch exposure mode to MANUAL mode or SHUTTER_PRIORITY mode " << std::endl;
                continue;
            }

            double speed;
            std::cout << "please enter Shutter: ";
            std::cin >> speed;
            exposure_settings->SetShutterSpeed(speed);
            auto ret = cam->SetExposureSettings(FuntionMode, exposure_settings);
            if (ret) {
                exposure_settings = cam->GetExposureSettings(FuntionMode);
                std::cout << "success! shutter " << exposure_settings->ShutterSpeed() << std::endl;
            }
        }

        if (option == 16) {
            const auto white_blance_type = ins_camera::CaptureSettings::SettingsType::CaptureSettings_WhiteBalance;
            auto capture_settings = cam->GetCaptureSettings(FuntionMode);
            if (capture_settings) {
                std::cout << "WB : " << capture_settings->GetIntValue(white_blance_type) << std::endl;
            }

            int wb;
            std::cout << "please enter WB: ";
            std::cin >> wb;
            capture_settings->SetValue(white_blance_type, wb);
            auto ret = cam->SetCaptureSettings(FuntionMode, capture_settings);
            if (ret) {
                capture_settings = cam->GetCaptureSettings(FuntionMode);
                if (capture_settings) {
                    std::cout << "WB : " << capture_settings->GetIntValue(white_blance_type) << std::endl;
                }
            }
        }

        if (option == 17) {
            auto exposure_settings = cam->GetExposureSettings(FuntionMode);
            if (exposure_settings) {
                std::cout << "EVBias : " << exposure_settings->EVBias() << std::endl;
                std::cout << "ISO    : " << exposure_settings->Iso() << std::endl;
                std::cout << "speed  : " << exposure_settings->ShutterSpeed() << std::endl;
                std::cout << "mode   : " << static_cast<int>(exposure_settings->ExposureMode()) << std::endl;
                std::cout << "videoISOLimit: " << exposure_settings->VideoISOTopLimit() << std::endl;
            }

            int mode;
            std::cout << "[Auto --------------->(0)]" << std::endl;
            std::cout << "[ISO_PRIORITY ------->(1)]" << std::endl;
            std::cout << "[SHUTTER_PRIORITY --->(2)]" << std::endl;
            std::cout << "[MANUAL ------------->(3)]" << std::endl;
            std::cout << "[ADAPTIVE ----------->(4)]" << std::endl;
            std::cout << "[FULL_AUTO ---------->(5)]" << std::endl;
            std::cout << "please enter mode: ";
            std::cin >> mode;
            exposure_settings->SetExposureMode(static_cast<ins_camera::PhotographyOptions_ExposureMode>(mode));
            auto ret = cam->SetExposureSettings(FuntionMode, exposure_settings);
            if (ret) {
                exposure_settings = cam->GetExposureSettings(FuntionMode);
                std::cout << "success! mode " << exposure_settings->ExposureMode() << std::endl;
            }
        }

        if (option == 18) {
            auto ret = cam->CaptureCurrentStatus();
            if (ret) {
                std::cout << "current statue : capture" << std::endl;
            }
            else {
                std::cout << "current statue : not capture" << std::endl;
            }
        }

        if (option == 19) {
            std::string file_to_save_dir;
            std::cout << "please input dir to download: ";
            std::cin >> file_to_save_dir;
            if (file_to_save_dir.empty()) {
                std::cout << "Invalid argument" << std::endl;
                continue;
            }

            std::replace(file_to_save_dir.begin(), file_to_save_dir.end(), '\\', '/');
            if (!fileIsExist(file_to_save_dir)) {
                std::cout << "not exitst: " << file_to_save_dir << std::endl;
                continue;
            }

            bool ret = cam->SetPhotoSubMode(ins_camera::SubPhotoMode::PHOTO_SINGLE);
            if (!ret) {
                std::cout << "change sub mode failed!" << std::endl;
                continue;
            }

            const auto url = cam->TakePhoto();
            if (!url.IsSingleOrigin() || url.Empty()) {
                std::cout << "failed to take picture" << std::endl;
                continue;
            }

            if (!endsWithSlash(file_to_save_dir)) {
                file_to_save_dir.append("/");
            }

            const std::string download_url = url.GetSingleOrigin();
            const std::string file_name = GetFileName(download_url);

            std::string save_path = file_to_save_dir + file_name;
            ret = cam->DownloadCameraFile(download_url, save_path);
            if (ret) {
                std::cout << "Download " << download_url << " succeed!!!" << std::endl;
            }
            else {
                std::cout << "Download " << download_url << " failed!!!" << std::endl;
            }
            std::this_thread::sleep_for(std::chrono::microseconds(1));
        }

        if (option == 20) {
            auto ret = cam->StartRecording();
            if (ret) {
                std::cerr << "success!" << std::endl;
            }
            else {
                std::cerr << "failed to start recording" << std::endl;
            }
        }

        if (option == 21) {
            ins_camera::BatteryStatus status{};
            bool ret = cam->GetBatteryStatus(status);
            if (!ret) {
                std::cerr << "GetBatteryStatus failed" << std::endl;
                continue;
            }
            std::cout << "PowerType : " << status.power_type << std::endl;
            std::cout << "battery_level : " << status.battery_level << std::endl;
            std::cout << "battery_scale : " << status.battery_scale << std::endl;
        }

        if (option == 22) {
            ins_camera::StorageStatus status{};
            bool ret = cam->GetStorageState(status);
            if (!ret) {
                std::cerr << "GetBatteryStatus failed" << std::endl;
                continue;
            }
            std::cout << "free_space : " << status.free_space << std::endl;
            std::cout << "total_space : " << status.total_space << std::endl;
            std::cout << "state : " << status.state << std::endl;
        }

        if (option == 23) {
            std::vector<std::string> file_list;
            bool ret = cam->GetRecordingFiles(file_list);
            if (!ret) {
                std::cerr << "GetRecordingFiles failed" << std::endl;
                continue;
            }
            for (auto& file : file_list) {
                std::cout << file << std::endl;
            }
        }

        if (option == 24) {
            bool is_connected = cam->IsConnected();
            std::cout << is_connected << std::endl;
        }

        if (option == 25) {
            const auto url = cam->TakePhoto();
            if (!url.IsSingleOrigin() || url.Empty()) {
                std::cout << "failed to take picture" << std::endl;
                continue;
            }
            const std::string download_url = url.GetSingleOrigin();
            std::cout << "url: " << download_url << std::endl;
        }

        if (option == 30) {
            std::vector<std::string> file_lists = cam->GetCameraFilesList();
            if (!file_lists.empty()) {
                std::string file_to_save_dir;
                std::cout << "please input dir to download: ";
                std::cin >> file_to_save_dir;
                std::replace(file_to_save_dir.begin(), file_to_save_dir.end(), '\\', '/');

                if (!endsWithSlash(file_to_save_dir)) {
                    file_to_save_dir.append("/");
                }

                int64_t current_progress = -1;
                for (const auto& url : file_lists) {
                    std::cout << "Download url: " << url << std::endl;
                    const std::string file_name = GetFileName(url);
                    std::string save_path = file_to_save_dir + file_name;
                    int ret = cam->DownloadCameraFile(url, save_path, [&](int64_t current, int64_t total_size) {
                        const int64_t process = current * 100 / total_size;
                        if (current_progress != process) {
                            current_progress = process;
                            std::cout << "\r";
                            std::cout << "process = " << process << "%";
                            std::cout << std::flush;
                        }
                    });
                    std::cout << std::endl;
                    if (ret) {
                        std::cout << "Download " << url << " succeed!!!" << std::endl;
                    }
                    else {
                        std::cout << "Download " << url << " failed!!!" << std::endl;
                    }
                }
            }
        }

        if (option == 31) {
            const auto file_list = cam->GetCameraFilesList();
            for (const auto& file : file_list) {
                const auto ret = cam->DeleteCameraFile(file);
                if (ret) {
                    std::cout << file << " Deletion succeed" << std::endl;
                }
            }
        }

        if (option == 32) {
            ins_camera::RecordParams record_params;
            record_params.resolution = ins_camera::VideoResolution::RES_2944_2880P30;
            record_params.bitrate = { 1024 * 1024 * 10 };
            auto ret = cam->SetVideoCaptureParams(record_params, ins_camera::CameraFunctionMode::FUNCTION_MODE_MOBILE_TIMELAPSE);
            if (!ret) {
                std::cout << "failed to set video capture param";
                continue;
            }

            ins_camera::TimelapseParam param{};
            param.mode = ins_camera::CameraTimelapseMode::MOBILE_TIMELAPSE_VIDEO;
            param.duration = -1;
            param.lapseTime = 500;
            param.accelerate_fequency = 5;
            cam->SetTimeLapseOption(param);

            ret = cam->StartTimeLapse(ins_camera::CameraTimelapseMode::MOBILE_TIMELAPSE_VIDEO);
            if (ret) {
                std::this_thread::sleep_for(std::chrono::seconds(60 * 12));
                auto ret1 = cam->StopTimeLapse(ins_camera::CameraTimelapseMode::MOBILE_TIMELAPSE_VIDEO);
                if (!ret1.Empty()) {
                    std::cout << "Stop timeLapse succeed!" << std::endl;
                }
                else {
                    std::cout << "Stop timeLapse failed!" << std::endl;
                }
            }
            else {
                std::cout << "failed to start timeLapse failed!" << std::endl;
            }
        }

        if (option == 33) {
            int count = 0;
            auto ret = cam->GetCameraFilesCount(count);
            if (ret) {
                std::cout << "The count of files is:" << count << std::endl;
            }
            else {
                std::cout << "get files count failed!!!" << std::endl;
            }
        }

        if (option == 34) {
            std::vector<ins_camera::PhotoSize> photo_size_vec = { ins_camera::PhotoSize::Size_11968_5984 ,ins_camera::PhotoSize::Size_5952_2976 };
            if (camera_type < ins_camera::CameraType::Insta360X4) {
                photo_size_vec = { ins_camera::PhotoSize::Size_11968_5984 ,ins_camera::PhotoSize::Size_6912_3456 };
            }

            static int hdr_count = 0;
            auto urls = cam->StartHDRCapture(photo_size_vec[(hdr_count++) % 2]);
            if (urls.Empty()) {
                std::cout << "failed to take picture" << std::endl;
            }
            else {
                for (const auto& url : urls.OriginUrls()) {
                    std::cout << url << std::endl;
                }
            }
        }

        if (option == 35) {
            if (camera_type != ins_camera::CameraType::Insta360X5 && camera_type != ins_camera::CameraType::Insta360X4) {
                std::cout << "not support " << std::endl;
            }
            std::string local_path;
            std::cout << "please input full file path to upload: ";
            std::cin >> local_path;
            if (local_path.empty()) {
                std::cout << "Invalid argument" << std::endl;
                continue;
            }

            if (!fileIsExist(local_path)) {
                std::cout << "not exist" << std::endl;
                continue;
            }

            std::string file_name = "Insta360X4FW.bin";
            if (camera_type == ins_camera::CameraType::Insta360X5) {
                file_name = "Insta360X5FW.bin";
            }

            int64_t current_progress = -1;
            const auto ret = cam->UploadFile(local_path, file_name,
                [&](int64_t total_size, int64_t upload_size) {
                const int64_t process = upload_size * 100 / total_size;
                if (current_progress != process) {
                    current_progress = process;
                    std::cout << "\r";
                    std::cout << "process = " << process << "%";
                    std::cout << std::flush;
                }
            });
            std::cout << std::endl;
            if (ret) {
                std::cout << "Upload " << local_path << " succeed!!!" << std::endl;
            }
            else {
                std::cout << "Upload " << local_path << " failed!!!" << std::endl;
            }
            cam->Close();
            return 0;
        }

        if (option == 36) {
            int sensor_device;
            std::cout << "FRONT: 1" << std::endl;
            std::cout << "REAR:  2" << std::endl;
            std::cout << "ALL:   3" << std::endl;
            std::cout << "please sensor device index: ";
            std::cin >> sensor_device;
            if (sensor_device > 3 || sensor_device < 1) {
                std::cout << "Invalid argument" << std::endl;
                continue;
            }

            const auto ret = cam->SetActiveSensor(static_cast<ins_camera::SensorDevice>(sensor_device));
            if (ret) {
                std::cout << "Succeed to set active sensor" << std::endl;
            }
            else {
                std::cout << "failed to set active sensor" << std::endl;
            }
        }

        if (option == 37) {
            auto media_time = cam->GetCameraMediaTime();
            std::cout << "media time : " << media_time << std::endl;
        }

        if (option == 38) {
            const auto ret = cam->ShutdownCamera();
            if (ret) {
                std::cout << "Succeed to shut down camera" << std::endl;
            }
            break;
        }

        if (option == 39) {
            const auto log_url = cam->GetCameraLogFileUrl();
            if (log_url.empty()) {
                std::cout << "failed to get log url";
                continue;
            }
            std::cout << "log url: " << log_url << std::endl;
            std::string file_to_save_dir;
            std::cout << "please input dir to download: ";
            std::cin >> file_to_save_dir;
            if (file_to_save_dir.empty()) {
                std::cout << "Invalid argument" << std::endl;
                continue;
            }

            std::replace(file_to_save_dir.begin(), file_to_save_dir.end(), '\\', '/');
            if (!fileIsExist(file_to_save_dir)) {
                std::cout << "not exitst: " << file_to_save_dir << std::endl;
                continue;
            }

            std::string log_dir;
            std::string log_file_name;
            parsePath(log_url, log_dir, log_file_name);
            if (!endsWithSlash(file_to_save_dir)) {
                file_to_save_dir.append("/");
            }

            std::string log_save_path = file_to_save_dir + log_file_name;

            int64_t current_process = -1;
            const auto ret = cam->DownloadCameraFile(log_url,
                log_save_path, [&](int64_t current, int64_t total_size) {
                const int64_t  process = current * 100 / total_size;
                if (process != current_process) {
                    std::cout << "\r";
                    std::cout << "process = " << process << "%";
                    std::cout << std::flush;
                }
            });
            std::cout << std::endl;
            if (ret) {
                std::cout << "Download " << log_save_path << " succeed!!!" << std::endl;
            }
            else {
                std::cout << "Download " << log_save_path << " failed!!!" << std::endl;
            }
        }
    }

    cam->Close();
    return 0;
}