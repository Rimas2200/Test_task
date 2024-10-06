#include <iostream>
#include <exception>
#include <fstream>
#include <vector>
#include <string>
#include <filesystem>
#include <sstream>
#include <facerec/import.h>
#include <facerec/libfacerec.h>

namespace fs = std::filesystem;

// Функция для чтения путей изображений из текстового файла
std::vector<std::string> read_image_paths(const std::string& file_path)
{
    std::vector<std::string> image_paths;
    std::ifstream file(file_path);
    std::string line;
    while (std::getline(file, line)) {
        image_paths.push_back(line);
    }
    return image_paths;
}

// Функция для тестирования одного изображения
std::pair<int, double> test_image(const std::string& image_path, pbio::ProcessingBlock& processing_block, pbio::FacerecService::Ptr& service)
{
    // Открытие файла изображения
    std::ifstream imageFile(image_path, std::ios::binary);
    if (!imageFile.is_open()) {
        std::cerr << "Ошибка при открытии файла изображения: " << image_path << std::endl;
        return { -1, 0.0 };  // Возвращаем ошибочный результат
    }

    // Чтение данных изображения
    std::vector<char> imageData((std::istreambuf_iterator<char>(imageFile)), std::istreambuf_iterator<char>());
    pbio::Context ioData = service->createContextFromEncodedImage(imageData);

    // Вызов процессинг-блока для обработки изображения
    processing_block(ioData);

    // Обработка результата
    for (size_t i = 0; i < ioData["objects"].size(); ++i) {
        pbio::Context obj = ioData["objects"][i];
        if (obj.contains("liveness")) {
            double liveness_score = obj["liveness"]["confidence"].getDouble();
            int liveness_result = obj["liveness"]["value"].getLong();
            return { liveness_result, liveness_score };  // Возвращаем результат и liveness_score
        }
    }
    return { -1, 0.0 };  // Если результата нет, возвращаем ошибку
}

// Функция для записи результатов в CSV файл
void save_results_to_csv(const std::vector<std::tuple<std::string, std::string, double>>& results, const std::string& output_file)
{
    std::ofstream csvFile(output_file);
    csvFile << "Image Name,Result,Liveness Score\n";
    for (const auto& [image_name, result, score] : results) {
        csvFile << image_name << "," << result << "," << score << "\n";
    }
    csvFile.close();
}

int main()
{
    try
    {
        // Инициализация сервиса
        pbio::FacerecService::Ptr service;
        service = pbio::FacerecService::createService("../bin/facerec.dll", "../conf/facerec/");

        // Настройка процессинг-блока
        auto configCtx = service->createContext();
        configCtx["unit_type"] = "LIVENESS_ESTIMATOR";
        configCtx["modification"] = "2d_light";
        configCtx["version"] = 3;
        configCtx["use_cuda"] = false;

        pbio::ProcessingBlock processing_block = service->createProcessingBlock(configCtx);

        // Пути к папкам
        std::string client_test_file = "../ClientRaw/client_test_raw.txt";
        std::string imposter_test_file = "../ImposterRaw/imposter_test_raw.txt";

        // Чтение путей изображений
        std::vector<std::string> client_images = read_image_paths(client_test_file);
        std::vector<std::string> imposter_images = read_image_paths(imposter_test_file);

        // Сбор результатов
        std::vector<std::tuple<std::string, std::string, double>> results;

        // Тестирование клиентских изображений
        for (const auto& img : client_images) {
            std::string image_path = "../ClientRaw/" + img;
            auto [result, score] = test_image(image_path, processing_block, service);
            if (result != -1) {
                std::string error = (result == 1) ? "Error" : "No Error";
                results.push_back({ img, error, score });
            }
        }

        // Тестирование изображений импостеров
        for (const auto& img : imposter_images) {
            std::string image_path = "../ImposterRaw/" + img;
            auto [result, score] = test_image(image_path, processing_block, service);
            if (result != -1) {
                std::string error = (result == 1) ? "No Error" : "Error";
                results.push_back({ img, error, score });
            }
        }

        // Сохранение результатов в CSV файл
        std::string output_csv = "../test_2d_light_v3.csv";
        save_results_to_csv(results, output_csv);
        std::cout << "Results saved to " << output_csv << std::endl;
    }
    catch (const pbio::Error& e) {
        std::cerr << "Ошибка Facerec: '" << e.what() << "', код: " << std::hex << e.code() << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "Общая ошибка: '" << e.what() << "'" << std::endl;
    }

    return 0;
}
