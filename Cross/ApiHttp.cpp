#include "Cross/ApiHttp.h"

#include <nlohmann/json.hpp>
#include <string>

#include "Cross/JwtToken.h"

namespace ApiHttp {

bool peticionAutorizada(const crow::request& peticion, std::string& correo, std::string& rol) {
    const std::string cabecera = peticion.get_header_value("Authorization");
    const std::string prefijo = "Bearer ";

    if (cabecera.size() <= prefijo.size() || cabecera.compare(0, prefijo.size(), prefijo) != 0) {
        return false;
    }

    const std::string token = cabecera.substr(prefijo.size());
    return TokenJwt::validar(token, correo, rol);
}

int codigoHttpDe(ResultadoOperacion resultado) {
    switch (resultado) {
        case ResultadoOperacion::Exito:
            return 200;
        case ResultadoOperacion::DatosInvalidos:
            return 400;
        case ResultadoOperacion::Duplicado:
            return 409;
        case ResultadoOperacion::ErrorInterno:
        default:
            return 500;
    }
}

crow::response respuestaError(int codigo, const std::string& mensaje) {
    nlohmann::json cuerpo;
    cuerpo["success"] = false;
    cuerpo["message"] = mensaje;
    return crow::response(codigo, cuerpo.dump());
}

}  // namespace ApiHttp
