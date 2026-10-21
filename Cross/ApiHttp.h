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

// Toda respuesta de la API se construye aquí para que lleve su Content-Type.
// Sin esa cabecera el cliente tiene que adivinar formato y codificación: axios
// no parsea el cuerpo y las herramientas que respetan el estándar rompen los
// acentos al no saber que el texto viene en UTF-8.
crow::response respuestaJson(int codigo, const std::string& cuerpo);

crow::response respuestaError(int codigo, const std::string& mensaje);

}  // namespace ApiHttp
