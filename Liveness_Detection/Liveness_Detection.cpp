#include <iostream>
#include <fstream>
#include <opencv2/opencv.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/objdetect.hpp>
#include <vector>
#include <string>
#include <filesystem>
#include <cmath>
#include <numeric>
#include <iomanip>

// Функция для чтения путей изображений из текстового файла
std::vector<std::string> read_image_paths(const std::string& file_path) {
    std::vector<std::string> image_paths;
    std::ifstream file(file_path);
    if (file.is_open()) {
        std::string line;
        while (std::getline(file, line)) {
            image_paths.push_back(line);
        }
        file.close();
    }
    return image_paths;
}

// Функция для сохранения результатов в CSV файл
void save_results_to_csv(const std::vector<std::tuple<std::string, std::string, double>>& results, const std::string& output_file) {
    std::ofstream csv_file(output_file);
    if (csv_file.is_open()) {
        csv_file << "Image Name,Result,Liveness Score\n";
        for (const auto& result : results) {
            csv_file << std::get<0>(result) << "," << std::get<1>(result) << "," << std::fixed << std::setprecision(3) << std::get<2>(result) << "\n";
        }
        csv_file.close();
    }
    else {
        std::cerr << "Не удалось открыть файл для записи: " << output_file << std::endl;
    }
}

// Функция анализа контраста
double analyze_contrast(const cv::Mat& image) {
    cv::Mat gray;
    cv::cvtColor(image, gray, cv::COLOR_BGR2GRAY);
    cv::Scalar mean, stddev;
    cv::meanStdDev(gray, mean, stddev);
    return stddev[0] / 64.0;
}

// Функция анализа яркости
double analyze_brightness(const cv::Mat& image) {
    cv::Mat gray;
    cv::cvtColor(image, gray, cv::COLOR_BGR2GRAY);
    cv::Scalar mean, stddev;
    cv::meanStdDev(gray, mean, stddev);
    return stddev[0] / 50.0;
}

// Функция анализа текстуры LBP
double analyze_texture_lbp(const cv::Mat& image) {
    cv::Mat gray;
    cv::cvtColor(image, gray, cv::COLOR_BGR2GRAY);

    int radius = 3;
    int neighbors = 24;
    cv::Mat lbp_image;
    lbp_image = cv::Mat::zeros(gray.size(), CV_32SC1);

    for (int i = radius; i < gray.rows - radius; i++) {
        for (int j = radius; j < gray.cols - radius; j++) {
            uchar center = gray.at<uchar>(i, j);
            unsigned int code = 0;
            for (int n = 0; n < neighbors; n++) {
                int y = i + radius * sin(2.0 * CV_PI * n / neighbors);
                int x = j + radius * cos(2.0 * CV_PI * n / neighbors);
                code |= (gray.at<uchar>(y, x) > center) << n;
            }
            lbp_image.at<int>(i, j) = code;
        }
    }

    std::vector<int> hist(256, 0);
    for (int i = 0; i < lbp_image.rows; i++) {
        for (int j = 0; j < lbp_image.cols; j++) {
            hist[lbp_image.at<int>(i, j)]++;
        }
    }

    double uniformity = std::accumulate(hist.begin(), hist.end(), 0.0, [](double sum, int val) {
        return sum + static_cast<double>(val) / (val + 1e-6);
        });
    return uniformity / (gray.rows * gray.cols);
}

// Функция анализа краев с использованием оператора Собеля
double analyze_edges(const cv::Mat& image) {
    cv::Mat gray, grad_x, grad_y, grad;
    cv::cvtColor(image, gray, cv::COLOR_BGR2GRAY);
    cv::Sobel(gray, grad_x, CV_64F, 1, 0, 3);
    cv::Sobel(gray, grad_y, CV_64F, 0, 1, 3);
    cv::magnitude(grad_x, grad_y, grad);
    return cv::mean(grad)[0] / 255.0;
}

// Функция для определения подлинности лица с использованием текстурного анализа (LBP)
std::tuple<std::string, double> detect_liveness_with_texture(const std::string& image_path) {
    cv::Mat image = cv::imread(image_path);
    if (image.empty()) {
        std::cerr << "Ошибка: изображение не найдено: " << image_path << std::endl;
        return std::make_tuple("error", 0.0);
    }

    cv::CascadeClassifier face_cascade(cv::samples::findFile("haarcascade_frontalface_default.xml"));
    std::vector<cv::Rect> faces;
    face_cascade.detectMultiScale(image, faces, 1.3, 5);

    if (faces.empty()) {
        return std::make_tuple("No face detected", 0.0);
    }

    cv::Rect face_rect = faces[0];
    cv::Mat face = image(face_rect);

    double edge_score = analyze_edges(face);
    double contrast_score = analyze_contrast(face);
    double brightness_score = analyze_brightness(face);
    double uniformity_score = analyze_texture_lbp(face);

    double liveness_score = (
        edge_score * 0.25 +
        contrast_score * 0.25 +
        brightness_score * 0.25 +
        (1 - uniformity_score) * 0.25
        );

    liveness_score = std::min(liveness_score, 1.0);

    return liveness_score >= 0.8 ? std::make_tuple("real", liveness_score) : std::make_tuple("attack", liveness_score);
}

int main() {
    std::string raw_folder = "raw";
    std::string client_folder = raw_folder + "/ClientRaw";
    std::string imposter_folder = raw_folder + "/ImposterRaw";

    std::string client_test_file = client_folder + "/client_test_raw.txt";
    std::string imposter_test_file = imposter_folder + "/imposter_test_raw.txt";

    std::vector<std::string> client_images = read_image_paths(client_test_file);
    std::vector<std::string> imposter_images = read_image_paths(imposter_test_file);

    // Тестирование изображений и сохранение результатов
    std::vector<std::tuple<std::string, std::string, double>> results;

    for (const auto& img : client_images) {
        std::string image_path = client_folder + "/" + img;
        auto [result, score] = detect_liveness_with_texture(image_path);
        std::string error = result == "attack" ? "Error" : "No Error";
        results.push_back(std::make_tuple(img, error, score));
        std::cout << img << " " << error << " " << score << std::endl;
    }

    for (const auto& img : imposter_images) {
        std::string image_path = imposter_folder + "/" + img;
        auto [result, score] = detect_liveness_with_texture(image_path);
        std::string error = result == "real" ? "No Error" : "Error";
        results.push_back(std::make_tuple(img, error, score));
        std::cout << img << " " << error << " " << score << std::endl;
    }

    // Сохранение результатов в CSV
    std::string output_csv = "raw/Liveness_Detection.csv";
    save_results_to_csv(results, output_csv);

    std::cout << "Results saved to " << output_csv << std::endl;

    return 0;
}
