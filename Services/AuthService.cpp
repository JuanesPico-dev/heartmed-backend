#include "Services/AuthService.h"

#include <cctype>
#include <string>

#include "Cross/JwtToken.h"
#include "Repositories/UsuarioRepository.h"

namespace {

std::string limpiarTexto(const std::string& valor) {
    std::string resultado = valor;
    const auto inicio = resultado.find_first_not_of(" \t\r\n");
    if (inicio == std::string::npos) {
        return "";
    }

    const auto fin = resultado.find_last_not_of(" \t\r\n");
    resultado = resultado.substr(inicio, fin - inicio + 1);
    return resultado;
}

bool tieneFormatoEmailValido(const std::string& email) {
    if (email.empty()) {
        return false;
    }

    const auto posicionArroba = email.find('@');
    if (posicionArroba == std::string::npos || posicionArroba == 0 || posicionArroba + 1 >= email.size()) {
        return false;
    }

    const auto puntoDespuesDeArroba = email.find('.', posicionArroba + 1);
    return puntoDespuesDeArroba != std::string::npos &&
           puntoDespuesDeArroba > posicionArroba + 1 &&
           puntoDespuesDeArroba + 1 < email.size();
}

}  // namespace

bool AuthService::login(const std::string& email, const std::string& password) {
    std::string token;
    std::string rol;
    return login(email, password, token, rol);
}

bool AuthService::login(const std::string& email, const std::string& password, std::string& token, std::string& rol) {
    const std::string correo = limpiarTexto(email);
    const std::string clave = limpiarTexto(password);

    if (!tieneFormatoEmailValido(correo) || clave.empty() || clave.size() < 6) {
        return false;
    }

    UsuarioRepository repository;
    if (!repository.existeUsuario(correo)) {
        return false;
    }

    if (!repository.validarCredenciales(correo, clave)) {
        return false;
    }

    rol = repository.ObtenerRolUsuario(correo);
    token = TokenJwt::generar(correo, rol);
    return true;
}
