#include <iostream>
#include <ins_stitcher.h>

#include <iostream>
#include <algorithm>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <chrono>
#include <vector>
#include <sstream>

#ifdef WIN32
#include <direct.h>
#include <Windows.h>
#endif // WIN32

using namespace std::chrono;
using namespace ins;

const std::string helpstr =
"{-help                   | default               | print this message                  }\n"
"{-inputs                 | None                  | input files                         }\n"
"{-output                 | None                  | out path                            }\n"
"{-stitch_type            | template              | template                            }\n"
"{                                                | optflow                             }\n"
"{                                                | dynamicstitch                       }\n"
"{                                                | aistitch                            }\n"
"{-ai_stitching_model     |                       | ai stitching model path             }\n"
"{-bitrate                | same as input video   | the bitrate of ouput file           }\n"
"{-enable_flowstate       | OFF                   | enable flowstate                    }\n"
"{-enable_directionlock   | OFF                   | enable directionlock                }\n"
"{-output_size            | 1920x960              | the resolution of output            }\n"
"{-enable_h265_encoder    | h264                  | encode format                       }\n"
"{-disable_cuda           | true                  | disable cuda                        }\n"
"{-enable_soft_encode     | false                 | use soft encoder                    }\n"
"{-enable_soft_decode     | false                 | use soft decoder                    }\n"
"{-enable_stitchfusion    | OFF                   | stitch_fusion                       }\n"
"{-enable_denoise         | OFF                   | enable denoise                      }\n"
"{-image_denoise_model    | OFF                   | image denoise model path            }\n"
"{-enable_colorplus       | OFF                   | enable colorplus                    }\n"
"{-colorplus_model        |                       | colorplus model path                }\n"
"{-enable_deflicker       | OFF                   | enable deflicker                    }\n"
"{-deflicker_model        |                       | deflicker model path                }\n"
"{-image_sequence_dir     | None                  | the output dir of image sequence    }\n"
"{-image_type             | jpg                   | jpg                                 }\n"
"{                                                | png                                 }\n"
"{-camera_accessory_type  | default 0             | refer to 'common.h'                 }\n"
"{-export_frame_index     |                       | Derived frame number sequence, example: 20-50-30 }\n";

static std::string stringToUtf8(const std::string& original_str) {
#ifdef WIN32
    int nwLen = MultiByteToWideChar(CP_ACP, 0, original_str.c_str(), -1, NULL, 0);

    wchar_t* pwBuf = new wchar_t[nwLen + 1];
    ZeroMemory(pwBuf, nwLen * 2 + 2);

    MultiByteToWideChar(CP_ACP, 0, original_str.c_str(), original_str.length(), pwBuf, nwLen);

    int nLen = WideCharToMultiByte(CP_UTF8, 0, pwBuf, -1, NULL, NULL, NULL, NULL);

    char* pBuf = new char[nLen + 1];
    ZeroMemory(pBuf, nLen + 1);

    WideCharToMultiByte(CP_UTF8, 0, pwBuf, nwLen, pBuf, nLen, NULL, NULL);

    std::string ret_str(pBuf);

    delete[] pwBuf;
    delete[] pBuf;

    pwBuf = NULL;
    pBuf = NULL;

    return ret_str;
#else
    return original_str;
#endif
}

std::vector<std::string> split(const std::string& s, char delimiter) {
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(s);

    while (std::getline(tokenStream, token, delimiter)) {
        tokens.push_back(token);
    }

    return tokens;
}

int main(int argc, char* argv[]) {
    ins::SetLogLevel(ins::InsLogLevel::ERR);
    ins::InitEnv();

    std::vector<std::string> input_paths;
    std::string output_path;
    std::string image_sequence_dir;
    std::string ai_stitching_model;
    std::string colorpuls_model_path;
    std::string denoise_model_path;
    std::string exported_frame_number_sequence;
    std::string deflicker_model_path;

    STITCH_TYPE stitch_type = STITCH_TYPE::OPTFLOW;
    IMAGE_TYPE image_type = IMAGE_TYPE::JPEG;
    CameraAccessoryType accessory_type = CameraAccessoryType::kNormal;

    int output_width = 1920;
    int output_height = 960;
    int output_bitrate = 0;

    bool enable_flowstate = false;
    bool enable_cuda = true;
    bool enable_soft_encode = false;
    bool enable_soft_decode = false;
    bool enalbe_stitchfusion = true;
    bool enable_colorplus = false;
    bool enable_directionlock = false;
    bool enable_sequence_denoise = false;
    bool enable_H265_encoder = false;
    bool enable_deflicker = false;

    for (size_t i = 1; i < argc; i++) {
        if (std::string("-inputs") == std::string(argv[i])) {
            std::string input_path = argv[++i];
            while (input_path[0] != '-') {
                input_paths.push_back(stringToUtf8(input_path));
                input_path = argv[++i];
            }
        }

        if (std::string("-output") == std::string(argv[i])) {
            output_path = stringToUtf8(argv[++i]);
        }
        if (std::string("-colorplus_model") == std::string(argv[i])) {
            colorpuls_model_path = stringToUtf8(argv[++i]);
        }
        else if (std::string("-stitch_type") == std::string(argv[i])) {
            std::string stitchType = argv[++i];
            if (stitchType == std::string("optflow")) {
                stitch_type = STITCH_TYPE::OPTFLOW;
            }
            else if (stitchType == std::string("dynamicstitch")) {
                stitch_type = STITCH_TYPE::DYNAMICSTITCH;
            }
            else if (stitchType == std::string("aistitch")) {
                stitch_type = STITCH_TYPE::AIFLOW;
            }
        }
        else if (std::string("-enable_flowstate") == std::string(argv[i])) {
            enable_flowstate = true;
        }
        else if (std::string("-disable_cuda") == std::string(argv[i])) {
            enable_cuda = false;
        }
        else if (std::string("-enable_stitchfusion") == std::string(argv[i])) {
            enalbe_stitchfusion = true;
        }
        else if (std::string("-enable_denoise") == std::string(argv[i])) {
            enable_sequence_denoise = true;
        }
        else if (std::string("-enable_colorplus") == std::string(argv[i])) {
            enable_colorplus = true;
        }
        else if (std::string("-enable_directionlock") == std::string(argv[i])) {
            enable_directionlock = true;
        }
        else if (std::string("-enable_h265_encoder") == std::string(argv[i])) {
            enable_H265_encoder = true;
        }
        else if (std::string("-bitrate") == std::string(argv[i])) {
            output_bitrate = atoi(argv[++i]);
        }
        else if (std::string("-output_size") == std::string(argv[i])) {
            auto res = split(std::string(argv[++i]), 'x');
            if (res.size() == 2) {
                output_width = std::atoi(res[0].c_str());
                output_height = std::atoi(res[1].c_str());
            }
        }
        else if (std::string("-image_sequence_dir") == std::string(argv[i])) {
            image_sequence_dir = std::string(argv[++i]);
        }
        else if (std::string("-image_type") == std::string(argv[i])) {
            std::string type = argv[++i];
            if (type == std::string("jpg")) {
                image_type = IMAGE_TYPE::JPEG;
            }
            else if (type == std::string("png")) {
                image_type = IMAGE_TYPE::PNG;
            }
        }
        else if (std::string("-camera_accessory_type") == std::string(argv[i])) {
            accessory_type = static_cast<CameraAccessoryType>(std::atoi(argv[++i]));
        }
        else if (std::string("-ai_stitching_model") == std::string(argv[i])) {
            ai_stitching_model = stringToUtf8(argv[++i]);
        }
        else if (std::string("-image_denoise_model") == std::string(argv[i])) {
            denoise_model_path = stringToUtf8(argv[++i]);
        }
        else if (std::string("-export_frame_index") == std::string(argv[i])) {
            exported_frame_number_sequence = argv[++i];
        }
        else if (std::string("-deflicker_model") == std::string(argv[i])) {
            deflicker_model_path = stringToUtf8(argv[++i]);
        }
        else if (std::string("-enable_deflicker") == std::string(argv[i])) {
            enable_deflicker = true;
        }
        else if (std::string("-enable_soft_encode") == std::string(argv[i])) {
            enable_soft_encode = true;
        }
        else if (std::string("-enable_soft_decode") == std::string(argv[i])) {
            enable_soft_decode = true;
        }
        else if (std::string("-help") == std::string(argv[i])) {
            std::cout << helpstr << std::endl;
        }
    }

    if (input_paths.empty()) {
        std::cout << "can not find input_file" << std::endl;
        std::cout << helpstr << std::endl;
        return -1;
    }

    if (output_path.empty() && image_sequence_dir.empty()) {
        std::cout << "can not find output_file" << std::endl;
        std::cout << helpstr << std::endl;
        return -1;
    }

    std::vector<uint64_t> export_frame_nums;
    if (!image_sequence_dir.empty()) {
        auto frame_index_vec = split(exported_frame_number_sequence, '-');
        for (auto& frame_index : frame_index_vec) {
            int index = atoi(frame_index.c_str());
            export_frame_nums.push_back(index);
        }
    }

    if (colorpuls_model_path.empty()) {
        enable_colorplus = false;
    }

    std::mutex mutex_;
    std::condition_variable cond_;
    bool is_finisned = false;
    bool has_error = false;
    bool is_cancel = false;
    int stitch_progress = 0;
    int count = 1;

    while (count--) {
        is_finisned = false;
        has_error = false;
        is_cancel = false;
        stitch_progress = 0;
        std::string suffix = input_paths[0].substr(input_paths[0].find_last_of(".") + 1);
        std::transform(suffix.begin(), suffix.end(), suffix.begin(), ::tolower);
        if (suffix == "insp" || suffix == "jpg") {
            auto image_stitcher = std::make_shared<ImageStitcher>();
            image_stitcher->SetInputPath(input_paths);
            image_stitcher->SetStitchType(stitch_type);
            image_stitcher->SetOutputPath(output_path);
            image_stitcher->SetOutputSize(output_width, output_height);
            image_stitcher->EnableFlowState(enable_flowstate);
            image_stitcher->EnableDenoise(enable_sequence_denoise, denoise_model_path);
            image_stitcher->EnableCuda(enable_cuda);
            image_stitcher->EnableStitchFusion(enalbe_stitchfusion);
            image_stitcher->SetCameraAccessoryType(accessory_type);
            image_stitcher->SetAiStitchModelFile(ai_stitching_model);
            image_stitcher->EnableColorPlus(enable_colorplus, colorpuls_model_path);
            image_stitcher->Stitch();
        }
        else if (suffix == "mp4" || suffix == "insv" || suffix == "lrv") {
            auto start_time = steady_clock::now();
            auto video_stitcher = std::make_shared<VideoStitcher>();
            video_stitcher->SetInputPath(input_paths);
            if (image_sequence_dir.empty()) {
                video_stitcher->SetOutputPath(output_path);
            }
            else {
                if (!export_frame_nums.empty()) {
                    video_stitcher->SetExportFrameSequence(export_frame_nums);
                }

                video_stitcher->SetImageSequenceInfo(image_sequence_dir, image_type);
            }
            video_stitcher->SetStitchType(stitch_type);
            video_stitcher->EnableCuda(enable_cuda);
            video_stitcher->EnableStitchFusion(enalbe_stitchfusion);
            video_stitcher->EnableColorPlus(enable_colorplus, colorpuls_model_path);
            video_stitcher->SetOutputSize(output_width, output_height);
            video_stitcher->SetOutputBitRate(output_bitrate);
            video_stitcher->EnableFlowState(enable_flowstate);
            video_stitcher->SetAiStitchModelFile(ai_stitching_model);
            video_stitcher->EnableDenoise(enable_sequence_denoise);
            video_stitcher->EnableDirectionLock(enable_directionlock);
            video_stitcher->SetCameraAccessoryType(accessory_type);
            video_stitcher->SetSoftwareCodecUsage(enable_soft_encode, enable_soft_decode);
            if (enable_H265_encoder) {
                video_stitcher->EnableH265Encoder();
            }
            video_stitcher->EnableDeflicker(enable_deflicker, deflicker_model_path);
            video_stitcher->SetStitchProgressCallback([&](int process, int error) {
                if (stitch_progress != process) {
                    const std::string process_desc = "process = " + std::to_string(process) + std::string("%");
                    std::cout << "\r" << process_desc << std::flush;
                    stitch_progress = process;
                }

                if (stitch_progress == 100) {
                    std::cout << std::endl;
                    std::unique_lock<std::mutex> lck(mutex_);
                    cond_.notify_one();
                    is_finisned = true;
                }
            });

            video_stitcher->SetStitchStateCallback([&](int error, const char* errinfo) {
                std::cout << "error: " << errinfo << std::endl;
                has_error = true;
                cond_.notify_one();
            });

            std::cout << "start stitch " << std::endl;
            video_stitcher->StartStitch();

            std::unique_lock<std::mutex> lck(mutex_);
            cond_.wait(lck, [&] {
                std::cout << "progress: " << video_stitcher->GetStitchProgress() << "; finisned: " << is_finisned << std::endl;
                return is_finisned || has_error;
            });

            std::cout << "end stitch " << std::endl;

            auto end_time = steady_clock::now();
            std::cout << "cost = " << duration_cast<duration<double>>(end_time - start_time).count() << std::endl;
        }
    }
    return 0;
}