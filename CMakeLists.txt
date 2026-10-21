cmake_minimum_required(VERSION 3.16)

project(HeartMedBackend LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if (WIN32 AND EXISTS "C:/msys64/mingw64")
    set(OPENSSL_ROOT_DIR "C:/msys64/mingw64" CACHE PATH "OpenSSL root directory" FORCE)
    list(APPEND CMAKE_PREFIX_PATH "C:/msys64/mingw64")
endif()

include(FetchContent)
find_package(SQLite3 REQUIRED)
find_package(OpenSSL REQUIRED COMPONENTS Crypto)

FetchContent_Declare(
    crow
    GIT_REPOSITORY https://github.com/CrowCpp/Crow.git
    GIT_TAG master
)
FetchContent_MakeAvailable(crow)

FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
)
FetchContent_MakeAvailable(nlohmann_json)

if (MSVC)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
endif()

add_executable(heartmed-backend
    entry/main.cpp
    Models/Usuarios.cpp
    Models/Paciente.cpp
    Models/Lote.cpp
    Models/Medicamento.cpp
    Repositories/UsuarioRepository.cpp
    Repositories/PacienteRepository.cpp
    Repositories/InventarioRepository.cpp
    Services/AuthService.cpp
    Services/PacienteService.cpp
    Services/InventarioService.cpp
    Routes/AuthRoutes.cpp
    Routes/PacienteRoutes.cpp
    Routes/InventarioRoutes.cpp
    Database/SQLite.cpp
    Cross/Config.cpp
    Cross/PasswordHash.cpp
    Cross/JwtToken.cpp
    Cross/ApiHttp.cpp
    Models/Paciente.h
    Models/Lote.h
    Models/Medicamento.h
    Repositories/UsuarioRepository.h
    Repositories/PacienteRepository.h
    Repositories/InventarioRepository.h
    Routes/AuthRoutes.h
    Routes/PacienteRoutes.h
    Routes/InventarioRoutes.h
    Services/PacienteService.h
    Services/InventarioService.h
    Database/SQLite.h
    Cross/Config.h
    Cross/PasswordHash.h
    Cross/JwtToken.h
    Cross/ApiHttp.h
    Cross/ResultadoOperacion.h
)

target_include_directories(heartmed-backend
    PRIVATE
        ${CMAKE_SOURCE_DIR}
)

target_link_libraries(heartmed-backend
    PRIVATE
        Crow::Crow
        nlohmann_json::nlohmann_json
        SQLite3::SQLite3
        OpenSSL::Crypto
        ws2_32
        wsock32
)
