#include <iostream>
#include <ins_realtime_stitcher.h>
#include <camera/camera.h>
#include <camera/device_discovery.h>

#include <iostream>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <chrono>
#include <vector>
#include <sstream>
#include <opencv2/opencv.hpp>

const std::string window_name = "realtime_stitcher";

std::vector<std::string> split(const std::string& s, char delimiter) {
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(s);

    while (std::getline(tokenStream, token, delimiter)) {
        tokens.push_back(token);
    }

    return tokens;
}

class StitchDelegate : public ins_camera::StreamDelegate {
public:
    StitchDelegate(const std::shared_ptr<ins::RealTimeStitcher>& stitcher) :stitcher_(stitcher) {
    }

    virtual ~StitchDelegate() {
    }

    void OnAudioData(const uint8_t* data, size_t size, int64_t timestamp) override {}

    void OnVideoData(const uint8_t* data, size_t size, int64_t timestamp, uint8_t streamType, int stream_index) override {
        stitcher_->HandleVideoData(data, size, timestamp, streamType, stream_index);
    }

    void OnGyroData(const std::vector<ins_camera::GyroData>& data) override {
        std::vector<ins::GyroData> data_vec(data.size());
        memcpy(data_vec.data(), data.data(), data.size() * sizeof(ins_camera::GyroData));
        stitcher_->HandleGyroData(data_vec);
    }

    void OnExposureData(const ins_camera::ExposureData& data) override {
        ins::ExposureData exposure_data{};
        exposure_data.exposure_time = data.exposure_time;
        exposure_data.timestamp = data.timestamp;
        stitcher_->HandleExposureData(exposure_data);
    }

private:
    std::shared_ptr<ins::RealTimeStitcher> stitcher_;
};

int main(int argc, char* argv[]) {
    ins::InitEnv();
    std::cout << "begin open camera" << std::endl;
    ins_camera::SetLogLevel(ins_camera::LogLevel::WARNING);
    ins::SetLogLevel(ins::InsLogLevel::WARNING);
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
    if (list.size() <= 0) {
        std::cerr << "no device found." << std::endl;
        discovery.FreeDeviceDescriptors(list);
        return -1;
    }
  
    for (const auto& camera : list) {
        std::cout << "serial:" << camera.serial_number << "\t"
            << ";camera type:" << camera.camera_name << "\t"
            << ";fw version:" << camera.fw_version << "\t"
            << std::endl;
    }

    auto cam = std::make_shared<ins_camera::Camera>(list[0].info);
    if (!cam->Open()) {
        std::cerr << "failed to open camera" << std::endl;
        return -1;
    }

    const auto serial_number = list[0].serial_number;

    discovery.FreeDeviceDescriptors(list);

    cv::Mat show_image_;
    std::thread show_thread_;
    std::mutex show_image_mutex_;
    bool is_stop_ = true;
    std::condition_variable show_image_cond_;

    std::shared_ptr<ins::RealTimeStitcher> stitcher = std::make_shared<ins::RealTimeStitcher>();
    ins::CameraInfo camera_info;
    auto preview_param = cam->GetPreviewParam();
    camera_info.cameraName = preview_param.camera_name;
    camera_info.decode_type = static_cast<ins::VideoDecodeType>(preview_param.encode_type);
    camera_info.offset = preview_param.offset;
    auto window_crop_info = preview_param.crop_info;
    camera_info.window_crop_info_.crop_offset_x = window_crop_info.crop_offset_x;
    camera_info.window_crop_info_.crop_offset_y = window_crop_info.crop_offset_y;
    camera_info.window_crop_info_.dst_width = window_crop_info.dst_width;
    camera_info.window_crop_info_.dst_height = window_crop_info.dst_height;
    camera_info.window_crop_info_.src_width = window_crop_info.src_width;
    camera_info.window_crop_info_.src_height = window_crop_info.src_height;
    camera_info.gyro_timestamp = preview_param.gyro_timestamp;

    stitcher->SetCameraInfo(camera_info);
    stitcher->SetStitchType(ins::STITCH_TYPE::DYNAMICSTITCH);
    stitcher->EnableFlowState(true);
    stitcher->SetOutputSize(960, 480);
    stitcher->SetStitchRealTimeDataCallback([&](uint8_t* data[4], int linesize[4], int width, int height, int format, int64_t timestamp) {
        std::unique_lock<std::mutex> lck(show_image_mutex_);
        show_image_ = cv::Mat(height, width, CV_8UC4, data[0]).clone();
        show_image_cond_.notify_one();
    });

    std::shared_ptr<ins_camera::StreamDelegate> delegate = std::make_shared<StitchDelegate>(stitcher);
    cam->SetStreamDelegate(delegate);

    std::cout << "Succeed to open camera..." << std::endl;

    std::cout << "Usage:" << std::endl;
    std::cout << "1: start preview live streaming:" << std::endl;
    std::cout << "2: stop preview live streaming:" << std::endl;

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
            if (!is_stop_) {
                std::cout << "" << std::endl;
                continue;
            }
            ins_camera::LiveStreamParam param;
            param.video_resolution = ins_camera::VideoResolution::RES_1440_720P30;
            param.lrv_video_resulution = ins_camera::VideoResolution::RES_1440_720P30;
            param.video_bitrate = 1024 * 1024 / 2;
            param.enable_audio = false;
            param.using_lrv = false;
            if (cam->StartLiveStreaming(param)) {
                stitcher->StartStitch();
                std::cout << "successfully started live stream" << std::endl;
            }

            show_thread_ = std::thread([&]() {
                cv::namedWindow(window_name, cv::WINDOW_NORMAL);
                is_stop_ = false;
                while (!is_stop_)
                {
                    std::unique_lock<std::mutex> lck(show_image_mutex_);
                    show_image_cond_.wait(lck, [&]() {
                        return  is_stop_ || !show_image_.empty();
                    });

                    if (is_stop_) {
                        break;
                    }

                    auto temp = show_image_.clone();
                    show_image_ = cv::Mat();
                    lck.unlock();
                    cv::cvtColor(temp, temp, cv::COLOR_RGBA2BGRA);
                    cv::imshow(window_name, temp);
                    cv::waitKey(5);
                }
            });
        }

        if (option == 2) {
            std::unique_lock<std::mutex> lck(show_image_mutex_);
            is_stop_ = true;
            show_image_cond_.notify_one();
            lck.unlock();
            if (show_thread_.joinable()) {
                show_thread_.join();
            }
            cv::destroyWindow(window_name);
            if (cam->StopLiveStreaming()) {
                stitcher->CancelStitch();
                std::cout << "success!" << std::endl;
            }
            else {
                std::cerr << "failed to stop live." << std::endl;
            }
        }
    }
    std::unique_lock<std::mutex> lck(show_image_mutex_);
    is_stop_ = true;
    show_image_cond_.notify_one();
    lck.unlock();
    if (show_thread_.joinable()) {
        show_thread_.join();
    }
    cv::destroyWindow(window_name);
    cam->Close();
    return 0;
}