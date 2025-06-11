#include <iostream>
#include <filesystem>
#include <string>
#include <fstream>
#include <locale>
#include <algorithm>
#include <cctype>

#ifdef _WIN32
#include <Windows.h>
#endif

namespace fs = std::filesystem;

int main() {
    // ��������� ������ � ������� ��������
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif
    std::locale::global(std::locale(""));
    std::cout.imbue(std::locale());
    std::cerr.imbue(std::locale());
    std::cin.imbue(std::locale());

    std::string path;
    std::cout << "������� ���� � ����������: ";
    std::getline(std::cin, path);

    // ��������� ������ � ����
    std::ofstream outFile("output.txt");
    if (!outFile.is_open()) {
        std::cerr << "������ �������� ����� ��� ������." << std::endl;
        return 1;
    }

    // ��������� ������������ ������
    std::streambuf* oldCoutBuffer = std::cout.rdbuf();
    std::streambuf* oldCerrBuffer = std::cerr.rdbuf();

    // �������������� ����� � ����
    std::cout.rdbuf(outFile.rdbuf());
    std::cerr.rdbuf(outFile.rdbuf());

    try {
        if (!fs::exists(path)) {
            std::cerr << "������: ���� �� ����������." << std::endl;
            return 1;
        }

        if (!fs::is_directory(path)) {
            std::cerr << "������: ��������� ���� �� �������� �����������." << std::endl;
            return 1;
        }

        std::cout << "\n���������� ���������� '" << path << "' (����������):" << std::endl;

        auto dirOptions = fs::directory_options::skip_permission_denied;
        for (auto it = fs::recursive_directory_iterator(path, dirOptions);
            it != fs::recursive_directory_iterator();
            ++it)
        {
            int depth_level = it.depth();
            const auto& entry = *it;

            std::cout << std::string(depth_level * 2, ' ')
                << entry.path().filename().string();

            if (fs::is_directory(entry.status())) {
                std::cout << " [����������]";
            }
            else if (fs::is_regular_file(entry.status())) {
                std::cout << " [����]";

                // ��������� ���������� �����
                auto extension = entry.path().extension().string();
                std::transform(extension.begin(), extension.end(), extension.begin(),
                    [](unsigned char c) { return std::tolower(c); });

                // ���������� ������� �������� ����������
                if (extension == ".cs" && extension != ".axaml.cs") {
                    std::ifstream file(entry.path());
                    if (file) {
                        std::cout << std::endl;
                        std::string line;
                        while (std::getline(file, line)) {
                            std::cout << std::string((depth_level + 1) * 2, ' ') << line << std::endl;
                        }
                    }
                }
            }
            else {
                std::cout << " [����������� ���]";
            }
            std::cout << std::endl;
        }
    }
    catch (const std::exception& e) {  // ������������� �������� ����������
        std::cerr << "������: " << e.what() << std::endl;
        return 1;
    }

    // ��������������� ������������ ������ � ��������� ����
    std::cout.rdbuf(oldCoutBuffer);
    std::cerr.rdbuf(oldCerrBuffer);
    outFile.close();

    return 0;
}