#pragma once

#include <string>

#include "crow.h"

#include "Cross/ResultadoOperacion.h"

// Piezas que repiten todas las rutas de la API: comprobar el token, traducir el
// resultado del servicio a un código HTTP y responder un error con el mismo
// formato en todas partes.
namespace ApiHttp {

// Lee el "Authorization: Bearer <token>" y lo valida.
bool peticionAutorizada(const crow::request& peticion, std::string& correo, std::string& rol);

int codigoHttpDe(ResultadoOperacion resultado);

crow::response respuestaError(int codigo, const std::string& mensaje);

}  // namespace ApiHttp
