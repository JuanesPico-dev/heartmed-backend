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

// El token viaja en la cabecera Authorization, que no es UTF-8: el navegador
// manda una "á" como un byte Latin-1 mientras el servidor la firmó con los dos
// bytes de UTF-8, y la firma deja de coincidir. Pasó en cuanto el login empezó
// a meter el nombre real del rol en el token ("Capitán/Médico Principal").
//
// Codificando correo y rol en base64url el token es ASCII pase lo que pase con
// esos textos. De paso ninguno de los dos puede ya contener un punto, así que
// el token tiene siempre cinco trozos exactos.
const char ALFABETO_B64URL[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";

std::string aBase64Url(const std::string& texto) {
    std::string salida;
    const std::size_t total = texto.size();

    for (std::size_t i = 0; i < total; i += 3) {
        const std::size_t restantes = total - i;
        const unsigned int octeto1 = static_cast<unsigned char>(texto[i]);
        const unsigned int octeto2 = restantes > 1 ? static_cast<unsigned char>(texto[i + 1]) : 0u;
        const unsigned int octeto3 = restantes > 2 ? static_cast<unsigned char>(texto[i + 2]) : 0u;
        const unsigned int bloque = (octeto1 << 16) | (octeto2 << 8) | octeto3;

        salida += ALFABETO_B64URL[(bloque >> 18) & 63];
        salida += ALFABETO_B64URL[(bloque >> 12) & 63];
        if (restantes > 1) {
            salida += ALFABETO_B64URL[(bloque >> 6) & 63];
        }
        if (restantes > 2) {
            salida += ALFABETO_B64URL[bloque & 63];
        }
    }

    return salida;
}

int valorBase64Url(char caracter) {
    if (caracter >= 'A' && caracter <= 'Z') return caracter - 'A';
    if (caracter >= 'a' && caracter <= 'z') return caracter - 'a' + 26;
    if (caracter >= '0' && caracter <= '9') return caracter - '0' + 52;
    if (caracter == '-') return 62;
    if (caracter == '_') return 63;
    return -1;
}

// false si el texto no es base64url válido: un token manipulado no debe pasar
// de aquí.
bool deBase64Url(const std::string& codificado, std::string& texto) {
    texto.clear();
    const std::size_t total = codificado.size();

    for (std::size_t i = 0; i < total; i += 4) {
        const std::size_t restantes = total - i;
        if (restantes == 1) {
            return false;
        }

        const int valor1 = valorBase64Url(codificado[i]);
        const int valor2 = valorBase64Url(codificado[i + 1]);
        const int valor3 = restantes > 2 ? valorBase64Url(codificado[i + 2]) : 0;
        const int valor4 = restantes > 3 ? valorBase64Url(codificado[i + 3]) : 0;

        if (valor1 < 0 || valor2 < 0 || valor3 < 0 || valor4 < 0) {
            return false;
        }

        const unsigned int bloque = (static_cast<unsigned int>(valor1) << 18) |
                                    (static_cast<unsigned int>(valor2) << 12) |
                                    (static_cast<unsigned int>(valor3) << 6) |
                                    static_cast<unsigned int>(valor4);

        texto += static_cast<char>((bloque >> 16) & 0xFF);
        if (restantes > 2) {
            texto += static_cast<char>((bloque >> 8) & 0xFF);
        }
        if (restantes > 3) {
            texto += static_cast<char>(bloque & 0xFF);
        }
    }

    return true;
}
}

std::string TokenJwt::generar(const std::string& correo, const std::string& rol) {
    const std::string secreto = Configuracion::instancia().obtenerCadena("JWT_SECRET", "secret_de_prueba");
    const int minutosExpiracion = Configuracion::instancia().obtenerEntero("JWT_EXPIRATION_MINUTES", 60);

    const auto ahora = std::chrono::system_clock::now();
    const auto expiracionUnix = std::chrono::duration_cast<std::chrono::seconds>(
        ahora.time_since_epoch() + std::chrono::minutes(minutosExpiracion)
    ).count();

    // La firma se calcula sobre los valores de verdad, no sobre su versión
    // codificada: lo que cambia es cómo viajan, no lo que se firma.
    const std::string firma = firmaTokenPrueba(secreto, correo, rol, expiracionUnix);
    return "heartmed." + aBase64Url(correo) + "." + aBase64Url(rol) + "." +
           std::to_string(expiracionUnix) + "." + firma;
}

bool TokenJwt::validar(const std::string& token, std::string& correo, std::string& rol) {
    if (token.empty()) {
        return false;
    }

    // Correo y rol van en base64url, que no admite puntos, así que el token
    // tiene exactamente cinco trozos.
    const std::vector<std::string> partes = dividir(token, '.');
    if (partes.size() != 5) {
        return false;
    }

    if (partes[0] != "heartmed") {
        return false;
    }

    std::string correoToken;
    std::string rolToken;
    if (!deBase64Url(partes[1], correoToken) || !deBase64Url(partes[2], rolToken)) {
        return false;
    }

    const std::string secreto = Configuracion::instancia().obtenerCadena("JWT_SECRET", "secret_de_prueba");

    long long expiracionUnix = 0;
    try {
        expiracionUnix = std::stoll(partes[3]);
    } catch (const std::exception&) {
        return false;
    }

    const auto ahora = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();

    if (expiracionUnix < ahora) {
        return false;
    }

    const std::string firmaEsperada = firmaTokenPrueba(secreto, correoToken, rolToken, expiracionUnix);
    if (firmaEsperada != partes[4]) {
        return false;
    }

    correo = correoToken;
    rol = rolToken;
    return true;
}
