#include <opencv2/opencv.hpp>
#include <opencv2/imgproc.hpp>
#include <iostream>
#include <filesystem>
#include <chrono>
#include <cmath>
#include <fstream>

namespace fs = std::filesystem;
using namespace cv;
using namespace std;

// Функция анализа текстуры с использованием GLCM
double analyze_texture_glcm(const Mat& image) {
    Mat gray;
    cvtColor(image, gray, COLOR_BGR2GRAY);
    gray.convertTo(gray, CV_8U);

    int rows = gray.rows;
    int cols = gray.cols;

    // Построение GLCM
    Mat glcm = Mat::zeros(256, 256, CV_32F);
    for (int i = 0; i < rows - 1; i++) {
        for (int j = 0; j < cols - 1; j++) {
            int row = gray.at<uchar>(i, j);
            int col = gray.at<uchar>(i, j + 1);
            glcm.at<float>(row, col)++;
        }
    }

    // Нормализация GLCM
    glcm /= sum(glcm)[0];

    // Извлечение статистических признаков
    double contrast = 0, energy = 0, entropy = 0;
    for (int i = 0; i < 256; i++) {
        for (int j = 0; j < 256; j++) {
            float p = glcm.at<float>(i, j);
            contrast += p * (i - j) * (i - j);
            energy += p * p;
            if (p > 0) {
                entropy -= p * log(p);
            }
        }
    }

    // Возвращаем confidence на основе энтропии, контраста и энергии
    return max(0.0, min(1.0, (entropy + contrast + energy) / 15.0));
}

double analyze_edges(const Mat& image) {
    Mat gray, edge_sobel;
    cvtColor(image, gray, COLOR_BGR2GRAY);
    Sobel(gray, edge_sobel, CV_64F, 1, 0);
    double edge_score = mean(edge_sobel).val[0];
    return max(0.0, min(1.0, edge_score * 10));
}

double analyze_contrast(const Mat& image) {
    Mat gray;
    cvtColor(image, gray, COLOR_BGR2GRAY);
    Scalar mean_value, stddev_value;
    meanStdDev(gray, mean_value, stddev_value);
    return max(0.0, min(1.0, stddev_value[0] / 64));
}

double analyze_brightness(const Mat& image) {
    Mat gray;
    cvtColor(image, gray, COLOR_BGR2GRAY);
    Scalar mean_value = mean(gray);
    return max(0.0, min(1.0, mean_value[0] / 255.0));
}

// Функция для детекции лица
Rect detect_face(const Mat& image) {
    Mat gray;
    cvtColor(image, gray, COLOR_BGR2GRAY);

    double minVal, maxVal;
    Point minLoc, maxLoc;
    minMaxLoc(gray, &minVal, &maxVal, &minLoc, &maxLoc);

    int face_width = 100;
    int face_height = 100;

    int x = max(0, maxLoc.x - face_width / 2);
    int y = max(0, maxLoc.y - face_height / 2);
    int width = min(face_width, image.cols - x);
    int height = min(face_height, image.rows - y);

    if (width > 0 && height > 0) {
        return Rect(x, y, width, height);
    }

    return Rect(); // Лицо не найдено
}

// Функция для детекции real
pair<string, double> detect_liveness_with_glcm(const string& image_path) {
    Mat image = imread(image_path);
    if (image.empty()) {
        cerr << "Ошибка: изображение не найдено." << endl;
        return { "Ошибка", 0.0 };
    }

    Rect face_rect = detect_face(image);
    if (face_rect.width == 0 || face_rect.height == 0) {
        return { "No face detected", 0.0 };
    }

    Mat face = image(face_rect);
    double texture_score = analyze_texture_glcm(face);
    double edge_score = analyze_edges(face);
    double contrast_score = analyze_contrast(face);
    double brightness_score = analyze_brightness(face);

    // confidence на основе анализа текстуры, контраста и яркости
    double liveness_score = (texture_score * 0.4 + edge_score * 0.2 + contrast_score * 0.3 + brightness_score * 0.1);
    liveness_score = min(liveness_score, 1.0);

    if (liveness_score >= 0.8) {
        return { "real", liveness_score };
    }
    else {
        return { "attack", liveness_score };
    }
}

// Функция для сохранения результатов в CSV файл
void saveResultsToCSV(const string& filepath, const vector<pair<string, pair<string, double>>>& results, const vector<double>& times) {
    ofstream file(filepath);
    if (!file.is_open()) {
        cerr << "Ошибка: Не удалось открыть файл для записи." << endl;
        return;
    }

    file << "Image,Result,Confidence,Time(ms)\n";
    for (size_t i = 0; i < results.size(); ++i) {
        file << results[i].first << "," << results[i].second.first << "," << results[i].second.second << "," << times[i] << "\n";
    }

    file.close();
}

int main() {
    setlocale(LC_ALL, "Russian");
    string attack_folder = "C:/Users/User/PycharmProjects/pythonProject2/Liveness Estimator/raw/ImposterRaw/0003";
    string bona_fide_folder = "C:/Users/User/PycharmProjects/pythonProject2/Liveness Estimator/raw/ClientRaw/0004";

    vector<pair<string, pair<string, double>>> results; // Для хранения: {имя изображения, {результат, confidence}}
    vector<double> times;

    int total_attacks = 0, correct_attacks = 0, errors_attacks = 0;
    int total_bona_fide = 0, correct_bona_fide = 0, errors_bona_fide = 0;

    double total_time = 0;
    int image_count = 0;

    // Обработка attack изображений
    for (const auto& entry : fs::directory_iterator(attack_folder)) {
        string image_path = entry.path().string();
        string image_name = entry.path().filename().string();  // Имя изображения
        if (image_path.find(".jpg") != string::npos) {
            total_attacks++;
            auto start_time = chrono::high_resolution_clock::now();
            auto [result, score] = detect_liveness_with_glcm(image_path);
            auto elapsed_time = chrono::high_resolution_clock::now() - start_time;
            double elapsed_ms = chrono::duration_cast<chrono::milliseconds>(elapsed_time).count();
            total_time += elapsed_ms;
            image_count++;

            results.push_back({ image_name, { result, score } });
            times.push_back(elapsed_ms);

            if (result == "attack") {
                correct_attacks++;
            }
            else {
                errors_attacks++;
            }
        }
    }

    // Обработка реальных изображений
    for (const auto& entry : fs::directory_iterator(bona_fide_folder)) {
        string image_path = entry.path().string();
        string image_name = entry.path().filename().string();  // Имя изображения
        if (image_path.find(".png") != string::npos || image_path.find(".jpg") != string::npos) {
            total_bona_fide++;
            auto start_time = chrono::high_resolution_clock::now();
            auto [result, score] = detect_liveness_with_glcm(image_path);
            auto elapsed_time = chrono::high_resolution_clock::now() - start_time;
            double elapsed_ms = chrono::duration_cast<chrono::milliseconds>(elapsed_time).count();
            total_time += elapsed_ms;
            image_count++;

            results.push_back({ image_name, { result, score } });
            times.push_back(elapsed_ms);

            if (result == "real") {
                correct_bona_fide++;
            }
            else {
                errors_bona_fide++;
            }
        }
    }

    // Среднее время обработки одного изображения
    double average_time_per_image = total_time / image_count;
    cout << "Среднее время обработки одного изображения: " << average_time_per_image << " мс" << endl;

    // Вывод статистики
    cout << "Количество изображений с атакой: " << total_attacks << endl;
    cout << "Верно определенные атаки: " << correct_attacks << endl;
    cout << "Ошибки атак: " << errors_attacks << endl;
    cout << "Количество изображений без атаки: " << total_bona_fide << endl;
    cout << "Верно определенные реальные лица: " << correct_bona_fide << endl;
    cout << "Ошибки без атак: " << errors_bona_fide << endl;

    // Общий процент верных ответов
    int total_correct = correct_attacks + correct_bona_fide;
    int total_images = total_attacks + total_bona_fide;
    double accuracy_percentage = (total_correct / static_cast<double>(total_images)) * 100;
    cout << "Общий процент верных ответов: " << accuracy_percentage << "%" << endl;

    // Сохранение результатов в CSV файл
    saveResultsToCSV("C:/Users/User/PycharmProjects/pythonProject2/Liveness Estimator/liveness_results.csv", results, times);

    return 0;
}
