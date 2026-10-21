#include "Cross/JwtToken.h"

#include <openssl/sha.h>

#include <chrono>
#include <cstddef>
#include <exception>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

#include "Cross/Config.h"

namespace {
std::string aHex(const unsigned char* datos, std::size_t longitud) {
    std::ostringstream flujo;
    flujo << std::hex << std::setfill('0');
    for (std::size_t i = 0; i < longitud; ++i) {
        flujo << std::setw(2) << static_cast<int>(datos[i]);
    }
    return flujo.str();
}

std::string firmaTokenPrueba(const std::string& secreto, const std::string& correo, const std::string& rol, long long expiracionUnix) {
    std::string entrada = secreto + "." + correo + "." + rol + "." + std::to_string(expiracionUnix);

    unsigned char resumen[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(entrada.c_str()), entrada.size(), resumen);
    return aHex(resumen, SHA256_DIGEST_LENGTH);
}

std::vector<std::string> dividir(const std::string& texto, char delimitador) {
    std::vector<std::string> partes;
    std::stringstream flujo(texto);
    std::string parte;
    while (std::getline(flujo, parte, delimitador)) {
        partes.push_back(parte);
    }
    return partes;
}
}

std::string TokenJwt::generar(const std::string& correo, const std::string& rol) {
    const std::string secreto = Configuracion::instancia().obtenerCadena("JWT_SECRET", "secret_de_prueba");
    const int minutosExpiracion = Configuracion::instancia().obtenerEntero("JWT_EXPIRATION_MINUTES", 60);

    const auto ahora = std::chrono::system_clock::now();
    const auto expiracionUnix = std::chrono::duration_cast<std::chrono::seconds>(
        ahora.time_since_epoch() + std::chrono::minutes(minutosExpiracion)
    ).count();

    const std::string firma = firmaTokenPrueba(secreto, correo, rol, expiracionUnix);
    return "heartmed." + correo + "." + rol + "." + std::to_string(expiracionUnix) + "." + firma;
}

bool TokenJwt::validar(const std::string& token, std::string& correo, std::string& rol) {
    if (token.empty()) {
        return false;
    }

    // El correo lleva puntos ("law@heartpirates.com"), así que el token se parte
    // en más de cinco trozos. Las piezas fijas se cuentan desde el final
    // (rol, expiración y firma) y lo que queda en medio es el correo.
    const std::vector<std::string> partes = dividir(token, '.');
    if (partes.size() < 5) {
        return false;
    }

    if (partes[0] != "heartmed") {
        return false;
    }

    const std::size_t indiceFirma = partes.size() - 1;
    const std::size_t indiceExpiracion = partes.size() - 2;
    const std::size_t indiceRol = partes.size() - 3;

    std::string correoToken = partes[1];
    for (std::size_t i = 2; i < indiceRol; ++i) {
        correoToken += "." + partes[i];
    }

    const std::string secreto = Configuracion::instancia().obtenerCadena("JWT_SECRET", "secret_de_prueba");

    long long expiracionUnix = 0;
    try {
        expiracionUnix = std::stoll(partes[indiceExpiracion]);
    } catch (const std::exception&) {
        return false;
    }

    const auto ahora = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();

    if (expiracionUnix < ahora) {
        return false;
    }

    const std::string firmaEsperada = firmaTokenPrueba(secreto, correoToken, partes[indiceRol], expiracionUnix);
    if (firmaEsperada != partes[indiceFirma]) {
        return false;
    }

    correo = correoToken;
    rol = partes[indiceRol];
    return true;
}
