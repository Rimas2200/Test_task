/**
	\file test_videocap.cpp
	\brief Example of face tracking and liveness estimation.
*/


#include <iostream>
#include <vector>
#include <fstream>
#include <string>
#include <chrono>
#include <filesystem>

#include <opencv2/opencv.hpp>

#include <facerec/import.h>
#include <facerec/libfacerec.h>
#include <pbio/example/CVRawImage.h>


#ifndef CV_AA
#define CV_AA cv::LINE_AA
#endif

#include "../console_arguments_parser/ConsoleArgumentsParser.h"

namespace fs = std::filesystem;

void saveResultsToCSV(const std::string& filename, const std::vector<std::string>& results, const std::vector<double>& times) {
    std::ofstream file(filename);
    file << "Image,Result,Time(ms)\n";
    for (size_t i = 0; i < results.size(); ++i) {
        file << i << "," << results[i] << "," << times[i] << "\n";
    }
}

int main() {
    try {
        const std::string dll_path = "facerec.dll";
        const std::string conf_dir_path = "conf/facerec";
        const std::string license_dir = "license";
        const std::string liveness2d_config = "liveness_2d_estimator_v2.xml";

        // Распознавание лиц
        const pbio::FacerecService::Ptr service = pbio::FacerecService::createService(dll_path, conf_dir_path, license_dir);
        const pbio::Capturer::Ptr capturer = service->createCapturer("fda_tracker_capturer_blf.xml");
        const pbio::Liveness2DEstimator::Ptr liveness_2d_estimator = service->createLiveness2DEstimator(liveness2d_config);

        // Директории с изображениями
        std::string attack_folder = "C:/Users/User/test_task/raw/ImposterRaw";
        std::string bona_fide_folder = "C:/Users/User/test_task/raw/ClientRaw";

        std::vector<std::string> results;
        std::vector<double> times;

        // Обработка изображения
        for (const auto& folder : { attack_folder, bona_fide_folder }) {
            for (const auto& entry : fs::directory_iterator(folder)) {
                pbio::CVRawImage image;
                image.mat() = cv::imread(entry.path().string());

                if (image.mat().empty()) continue;

                // Оценка
                auto start = std::chrono::high_resolution_clock::now();

                const std::vector<pbio::RawSample::Ptr> samples = capturer->capture(image);
                std::string result_str;

                for (const auto& sample : samples) {
                    const pbio::Liveness2DEstimator::LivenessAndScore result = liveness_2d_estimator->estimate(*sample);

                    // Результат
                    std::stringstream ss;
                    ss << std::fixed << std::setprecision(3) << result.score;

                    switch (result.liveness) {
                    case pbio::Liveness2DEstimator::FAKE:
                        result_str = ss.str() + " - fake";
                        break;
                    case pbio::Liveness2DEstimator::REAL:
                        result_str = ss.str() + " - real";
                        break;
                    default:
                        result_str = "?";
                        break;
                    }
                }

                auto end = std::chrono::high_resolution_clock::now();
                double elapsed_time = std::chrono::duration<double, std::milli>(end - start).count();

                results.push_back(result_str);
                times.push_back(elapsed_time);
            }
        }

        saveResultsToCSV("C:/Users/User/test_task/liveness_results.csv", results, times);

    }
    catch (const pbio::Error& e) {
        std::cerr << "facerec exception catched: '" << e.what() << "' code: " << std::hex << e.code() << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "exception catched: '" << e.what() << "'" << std::endl;
    }

    return 0;
}

