#include <iostream>
#include <filesystem>
#include <string>
#include <fstream>
#include <locale>
#include <algorithm>
#include <cctype>
#include <cstring> // Добавлен для memcmp

#ifdef _WIN32
#include <Windows.h>
#endif

namespace fs = std::filesystem;

#ifdef _WIN32
std::string ansi_to_utf8(const std::string& ansi) {
    int wlen = MultiByteToWideChar(CP_ACP, 0, ansi.c_str(), (int)ansi.size(), nullptr, 0);
    std::wstring wstr(wlen, 0);
    MultiByteToWideChar(CP_ACP, 0, ansi.c_str(), (int)ansi.size(), &wstr[0], wlen);

    int utf8_len = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.size(), nullptr, 0, nullptr, nullptr);
    std::string utf8(utf8_len, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.size(), &utf8[0], utf8_len, nullptr, nullptr);
    return utf8;
}
#endif

// Функция преобразования u8string в string
std::string u8_to_string(const fs::path::string_type& u8str) {
#ifdef _WIN32
    if constexpr (std::is_same_v<fs::path::value_type, wchar_t>) {
        int utf8_len = WideCharToMultiByte(CP_UTF8, 0, u8str.c_str(), (int)u8str.size(), nullptr, 0, nullptr, nullptr);
        std::string result(utf8_len, 0);
        WideCharToMultiByte(CP_UTF8, 0, u8str.c_str(), (int)u8str.size(), &result[0], utf8_len, nullptr, nullptr);
        return result;
    }
#endif
    return std::string(u8str.begin(), u8str.end());
}

int main() {
    // Настройка локали и кодовой страницы
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif
    std::locale::global(std::locale(""));
    std::cout.imbue(std::locale());
    std::cerr.imbue(std::locale());
    std::cin.imbue(std::locale());

    std::string path;
    std::cout << "Введите путь к директории: ";
    std::getline(std::cin, path);

    // Преобразование введенного пути в UTF-8 (для Windows)
#ifdef _WIN32
    std::string utf8_path = ansi_to_utf8(path);
#else
    std::string utf8_path = path;
#endif

    // Настройка вывода в файл
    std::ofstream outFile("output.txt");
    if (!outFile.is_open()) {
        std::cerr << "Ошибка открытия файла для записи." << std::endl;
        return 1;
    }

    // Сохраняем оригинальные буферы
    std::streambuf* oldCoutBuffer = std::cout.rdbuf();
    std::streambuf* oldCerrBuffer = std::cerr.rdbuf();

    // Перенаправляем вывод в файл
    std::cout.rdbuf(outFile.rdbuf());
    std::cerr.rdbuf(outFile.rdbuf());

    try {
        if (!fs::exists(path)) {
            std::cerr << "Ошибка: путь не существует." << std::endl;
            return 1;
        }

        if (!fs::is_directory(path)) {
            std::cerr << "Ошибка: указанный путь не является директорией." << std::endl;
            return 1;
        }

        std::cout << "\nСодержимое директории '" << utf8_path << "' (рекурсивно):" << std::endl;

        auto dirOptions = fs::directory_options::skip_permission_denied;
        for (auto it = fs::recursive_directory_iterator(path, dirOptions);
            it != fs::recursive_directory_iterator();
            ++it)
        {
            int depth_level = it.depth();
            const auto& entry = *it;

            // Используем новую функцию преобразования
            std::string name = u8_to_string(entry.path().filename().native());
            std::cout << std::string(depth_level * 2, ' ') << name;

            if (fs::is_directory(entry.status())) {
                std::cout << " [Директория]";
            }
            else if (fs::is_regular_file(entry.status())) {
                std::cout << " [Файл]";

                // Получаем расширение через новую функцию
                std::string extension = u8_to_string(entry.path().extension().native());
                std::transform(extension.begin(), extension.end(), extension.begin(),
                    [](unsigned char c) { return std::tolower(c); });

                if (extension == ".cs") {
                    std::ifstream file(entry.path(), std::ios::binary);
                    if (file) {
                        std::cout << std::endl;

                        // Проверка на UTF-8 BOM
                        char bom[3] = { 0 };
                        file.read(bom, 3);
                        bool hasBOM = (file.gcount() == 3) &&
                            (static_cast<unsigned char>(bom[0]) == 0xEF &&
                                static_cast<unsigned char>(bom[1]) == 0xBB &&
                                static_cast<unsigned char>(bom[2]) == 0xBF);
                        if (!hasBOM) {
                            file.seekg(0); // Если нет BOM, возвращаемся к началу
                        }

                        std::string line;
                        while (std::getline(file, line)) {
                            // Удаляем \r в конце строки (для Windows)
                            if (!line.empty() && line.back() == '\r') {
                                line.pop_back();
                            }
                            std::cout << std::string((depth_level + 1) * 2, ' ') << line << std::endl;
                        }
                    }
                }
            }
            else {
                std::cout << " [Специальный тип]";
            }
            std::cout << std::endl;
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Ошибка: " << e.what() << std::endl;
        return 1;
    }

    // Восстанавливаем оригинальные буферы и закрываем файл
    std::cout.rdbuf(oldCoutBuffer);
    std::cerr.rdbuf(oldCerrBuffer);
    outFile.close();

    return 0;
}