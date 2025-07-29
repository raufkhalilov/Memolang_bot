cmake_minimum_required(VERSION 3.14)
project(MemoLangBot LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# -------------------------------------------------------
# CPR из локальной папки external/cpr, НО с системным curl
# -------------------------------------------------------
# Важные флаги должны быть установлены ДО add_subdirectory.
set(CPR_USE_SYSTEM_CURL ON CACHE BOOL "Use system installed libcurl")
set(CPR_ENABLE_SSL ON CACHE BOOL "Enable SSL support")
# На Linux принудительно используем OpenSSL.
set(CPR_FORCE_OPENSSL_BACKEND ON CACHE BOOL "Force OpenSSL backend")
# Ускоряем сборку: не собираем тесты и лишние проверки.
set(CPR_BUILD_TESTS OFF CACHE BOOL "Do not build CPR tests")
set(CPR_ENABLE_LINTING OFF CACHE BOOL "Disable linting")

add_subdirectory(external/cpr)

# -------------------------------------------------------
# nlohmann/json (через FetchContent даёт целевой target
# nlohmann_json::nlohmann_json)
# -------------------------------------------------------
include(FetchContent)
FetchContent_Declare(
  json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG v3.11.2
)
FetchContent_MakeAvailable(json)  # даёт target nlohmann_json::nlohmann_json
# Документация подтверждает имя таргета.  :contentReference[oaicite:1]{index=1}

# -------------------------------------------------------
# Исполняемый файл
# -------------------------------------------------------
add_executable(memolang_bot
  src/main.cpp
)

# -------------------------------------------------------
# Линковка
# -------------------------------------------------------
target_link_libraries(memolang_bot
  PRIVATE
    cpr::cpr
    nlohmann_json::nlohmann_json
)

# (Не обязательно, но полезно)
target_compile_features(memolang_bot PRIVATE cxx_std_17)
