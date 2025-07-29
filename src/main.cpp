#include <iostream>
#include <string>
#include <fstream>
#include <atomic>
#include <thread>
#include <chrono>
#include <csignal>
#include <fstream>

#include <cpr/cpr.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

static std::atomic_bool g_running{true};
void on_signal(int) { g_running = false; }

std::string BOT_TOKEN = "...";
std::string BASE_URL = "https://api.telegram.org/bot" + BOT_TOKEN;

cpr::Response send_message(long long chat_id,
                           const std::string& text,
                           const std::string& parse_mode = std::string()) {
    if (parse_mode.empty()) {
        return cpr::Get(
            cpr::Url{BASE_URL + "/sendMessage"},
            cpr::Parameters{
                {"chat_id", std::to_string(chat_id)},
                {"text", text}
            }
        );
    } else {
        return cpr::Get(
            cpr::Url{BASE_URL + "/sendMessage"},
            cpr::Parameters{
                {"chat_id", std::to_string(chat_id)},
                {"text", text},
                {"parse_mode", parse_mode}
            }
        );
    }
}

std::int64_t load_offset(const std::string& path) {
    std::ifstream f(path);
    std::int64_t off = 0;
    if (f) f >> off;
    return off;
}
void save_offset(const std::string& path, std::int64_t off) {
    std::ofstream f(path, std::ios::trunc);
    if (f) f << off;
}

int main() {
    std::signal(SIGINT,  on_signal);
    std::signal(SIGTERM, on_signal);

    std::cout << "MemoLang Bot запущен (Ctrl+C для выхода)\n";

    const std::string offset_file = "data/last_offset.txt";
    std::int64_t offset = load_offset(offset_file);

    while (g_running) {
        cpr::Response r = cpr::Get(
            cpr::Url{BASE_URL + "/getUpdates"},
            cpr::Parameters{
                {"timeout", "25"},
                {"offset", std::to_string(offset)}
            },
            cpr::Timeout{30000}
        );

        if (r.error) {
            std::cerr << "HTTP error: " << r.error.message << "\n";
            std::this_thread::sleep_for(std::chrono::seconds(2));
            continue;
        }

        json j = json::parse(r.text, nullptr, false);
        if (j.is_discarded() || !j.value("ok", false)) {
            std::cerr << "Ошибка ответа: " << r.text << "\n";
            std::this_thread::sleep_for(std::chrono::seconds(2));
            continue;
        }

        for (const auto& upd : j["result"]) {
            std::int64_t update_id = upd.value("update_id", 0LL);
            if (update_id >= offset) offset = update_id + 1;

            if (!upd.contains("message")) continue;

            const auto& msg = upd["message"];
            long long chat_id = msg["chat"]["id"].get<long long>();
            std::string text  = msg.value("text", "");
            std::string uname = msg["from"].value("username", "unknown");

            std::cout << "@" << uname << ": " << text << "\n";

            if (text == "/start") {
                send_message(chat_id,
                    "Привет! Я MemoLang. Здесь ты можешь сохранять незнакомые иностранные слова, а потом повторять их!\nОтправь `/add dog - собака` чтобы добавить слово.");
                continue;
            }

            if (text.rfind("/add ", 0) == 0) {
                std::string entry = text.substr(5);
                std::string file_path = "data/DataBase.json";

                json db;

                // Чтение файла
                std::ifstream inFile(file_path);
                if (inFile) {
                    std::stringstream buffer;
                    buffer << inFile.rdbuf();
                    std::string content = buffer.str();
                    if (!content.empty()) {
                        try {
                            db = json::parse(content);
                        } catch (const json::parse_error& e) {
                            send_message(chat_id, "Ошибка чтения JSON: " + std::string(e.what()));
                            continue;
                        }
                    }
                }
                inFile.close();

                // Убедимся, что есть массив
                if (!db.contains("entries") || !db["entries"].is_array()) {
                    db["entries"] = json::array();
                }

                // Парсим ключ-значение
                size_t pos = entry.find("-");
                if (pos == std::string::npos) {
                    send_message(chat_id, "Неверный формат. Используй: /add ключ - значение");
                    continue;
                }

                std::string key = entry.substr(0, pos);
                std::string value = entry.substr(pos + 1);

                // trim
                key.erase(0, key.find_first_not_of(" \t"));
                key.erase(key.find_last_not_of(" \t") + 1);
                value.erase(0, value.find_first_not_of(" \t"));
                value.erase(value.find_last_not_of(" \t") + 1);

                // Добавляем
                json record;
                record[key] = value;
                db["entries"].push_back(record);

                // Сохраняем файл
                std::ofstream outFile(file_path);
                outFile << db.dump(4);
                outFile.close();

                send_message(chat_id, "Добавил: " + key + " → " + value);
                continue;
            }

            send_message(chat_id, "Ты написал: " + text);
        }

        save_offset(offset_file, offset);
    }

    std::cout << "Остановка...\n";
    return 0;
}
