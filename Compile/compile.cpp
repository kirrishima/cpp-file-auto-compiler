#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <filesystem>
#include <fstream>
#include <string>
#include <sstream>
#include <iomanip>
#include <nlohmann/json.hpp> 
#include <openssl/evp.h>     
#include <Windows.h>

namespace fs = std::filesystem;
using json = nlohmann::json;

// Функция для вычисления SHA-256 хэша файла
std::string computeFileHash(const std::string& filePath) {
	std::ifstream file(filePath, std::ios::binary);
	if (!file.is_open()) {
		throw std::runtime_error("Не удалось открыть файл для хэширования: " + filePath);
	}

	EVP_MD_CTX* ctx = EVP_MD_CTX_new();
	if (!ctx) {
		throw std::runtime_error("Не удалось создать контекст хэширования");
	}

	unsigned char hash[EVP_MAX_MD_SIZE];
	unsigned int hashLen = 0;

	if (EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr) != 1) {
		EVP_MD_CTX_free(ctx);
		throw std::runtime_error("Ошибка инициализации хэширования");
	}

	char buffer[8192];
	while (file.read(buffer, sizeof(buffer))) {
		if (EVP_DigestUpdate(ctx, buffer, file.gcount()) != 1) {
			EVP_MD_CTX_free(ctx);
			throw std::runtime_error("Ошибка обновления хэша");
		}
	}
	if (EVP_DigestUpdate(ctx, buffer, file.gcount()) != 1) {
		EVP_MD_CTX_free(ctx);
		throw std::runtime_error("Ошибка обновления хэша");
	}

	if (EVP_DigestFinal_ex(ctx, hash, &hashLen) != 1) {
		EVP_MD_CTX_free(ctx);
		throw std::runtime_error("Ошибка финализации хэша");
	}

	EVP_MD_CTX_free(ctx);

	std::ostringstream result;
	for (unsigned int i = 0; i < hashLen; ++i) {
		result << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
	}
	return result.str();
}

// Чтение конфигурации из JSON файла
json loadJsonFile(const std::string& filePath) {
	std::ifstream file(filePath);
	if (!file.is_open()) {
		throw std::runtime_error("Не удалось открыть JSON файл: " + filePath);
	}
	json config;
	file >> config;
	return config;
}

// Чтение стандарта C++ из файла
int readCppStandard(const json& config, const std::string& filePath) {
	if (config.contains("cpp_standard")) {
		return config["cpp_standard"].get<int>();
	}

	try {
		std::ifstream file(filePath);
		if (!file.is_open()) {
			throw std::runtime_error("Не удалось открыть файл: " + filePath);
		}
		std::string line;
		while (std::getline(file, line)) {
			if (line.find("//std:c++")  != std::string::npos) {
				auto pos = line.find_last_of('+');
				if (pos != std::string::npos) {
					return std::stoi(line.substr(pos + 1));
				}
			}
		}
	}
	catch (const std::exception&) {
		return 14;
	}
	return 14;
}



int main() {
	setlocale(LC_ALL, "ru");
	SetConsoleOutputCP(1251);
	try {
		const std::string configPath = "config.json";
		json globalConfig = loadJsonFile(configPath);

		std::string inputDir = fs::absolute(fs::path(globalConfig["input_directory"].get<std::string>())).string();
		std::string defaultOutputDir = fs::absolute(fs::path(globalConfig["output_directory"].get<std::string>())).string();
		std::string cacheDir = fs::absolute(fs::path(globalConfig["cache_directory"].get<std::string>())).string();
		std::string defaultFlags = globalConfig["default_compiler_flags"].get<std::string>();
		std::string devBatPath = globalConfig["devcmd_path"].get<std::string>();

		if (!cacheDir.empty() && !fs::exists(cacheDir)) {
			if (!fs::create_directories(cacheDir)) {
				std::cerr << "Не удалось создать директории: " << cacheDir << "\n";
				return 1;
			}
		}

		for (const auto& entry : fs::directory_iterator(inputDir)) {
			if (!entry.is_regular_file() || entry.path().extension() != ".cpp")
				continue;

			std::string filePath = entry.path().string();
			std::string fileName = entry.path().filename().string();

			// Загрузка индивидуальных настроек файла
			std::string fileConfigPath = (entry.path().parent_path() / (entry.path().stem().string() + ".json")).string();
			json fileConfig;
			if (fs::exists(fileConfigPath)) {
				try {
					fileConfig = loadJsonFile(fileConfigPath);
				}
				catch (const std::exception& e) {
					std::cerr << "Ошибка чтения конфигурации для файла " << fileName << ": " << e.what() << "\n";
					continue;
				}
			}

			// Выходная директория и имя файла
			std::string outputDir = fileConfig.contains("output_directory")
				? fs::absolute(fileConfig["output_directory"].get<std::string>()).string()
				: defaultOutputDir;
			std::string outputFileName = fileConfig.contains("output_file")
				? fileConfig["output_file"].get<std::string>()
				: fileName.substr(0, fileName.find_last_of('.'));

			std::string outputPath = (fs::path(outputDir) / outputFileName).string();

			// Вычисление хэша текущего файла
			std::string currentHash = computeFileHash(filePath);
			std::string cacheFilePath = (fs::path(cacheDir) / (fileName + ".hash")).string();

			bool needsRecompile = true;

			if (fs::exists(cacheFilePath)) {
				std::ifstream cacheFile(cacheFilePath);
				std::string cachedHash;
				cacheFile >> cachedHash;
				cacheFile.close();
				if (cachedHash == currentHash && !fs::exists(fs::path(inputDir) / (entry.path().stem().string() + ".exe"))) {
					needsRecompile = false;
				}
			}

			if (needsRecompile) {
				std::cout << "Обнаружены изменения в файле: '" << fileName << "'. Он будет скомпилирован\n";

				int cppStandard = readCppStandard(fileConfig, filePath);

				// Параметры компиляции
				std::string additionalFlags = fileConfig.contains("compile_flags")
					? fileConfig["compile_flags"].get<std::string>()
					: defaultFlags;

				// Формирование команды компиляции
				std::string command = std::format(
					R"("{}" && cl /std:c++{} {} /Fe:"{}" "{}" > "compile_logs/compile_{}.log" 2>&1)",
					devBatPath,
					cppStandard,
					additionalFlags,
					outputPath,
					filePath,
					entry.path().filename().string());

				if (!fs::exists("compile_logs") || (!fs::is_directory("compile_logs") && fs::exists("compile_logs")))
				{
					fs::create_directory("compile_logs");
				}

				std::ofstream batFile("compile.bat");
				if (!batFile.is_open()) {
					std::cout << "Не удалось создать файл compile.bat\n";
					continue;
				}
				batFile << command;
				batFile.close();

#ifdef _WIN32
				const char* nullDevice = "NUL";
#else
				const char* nullDevice = "/dev/null";
#endif

				FILE* nullOut = freopen(nullDevice, "w", stdout);
				FILE* nullErr = freopen(nullDevice, "w", stderr);

				int result = std::system("compile.bat");

				if (nullOut) freopen("CON", "w", stdout);
				if (nullErr) freopen("CON", "w", stderr);

				if (result != 0) {
					std::cout << "Ошибка компиляции файла: " << fileName << "\n";
				}
				else
				{
					std::ofstream cacheFile(cacheFilePath);
					cacheFile << currentHash;
					cacheFile.close();
				}

				std::string fileObjPath = entry.path().stem().string() + ".obj";
				if (fs::exists(fileObjPath))
				{
					fs::remove(fileObjPath);
				}
			}
			else {
				std::cout << "Файл не изменился: " << fileName << "\n";
			}
		}

		std::cout << "Обработка завершена." << std::endl;
	}
	catch (const std::exception& e) {
		std::cerr << "Ошибка: " << e.what() << std::endl;
		return 1;
	}

	if (fs::exists("compile.bat"))
	{
		fs::remove("compile.bat");
	}

	return 0;
}
