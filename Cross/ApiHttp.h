#pragma once

#include <string>

#include "crow.h"

#include "Cross/ResultadoOperacion.h"

// Helpers comunes para rutas: autenticación, códigos HTTP y respuestas JSON.
namespace ApiHttp {

// Valida el token Bearer y devuelve correo/rol.
bool peticionAutorizada(const crow::request& peticion, std::string& correo, std::string& rol);

int codigoHttpDe(ResultadoOperacion resultado);

// Respuesta JSON con cabecera correcta para el cliente.
crow::response respuestaJson(int codigo, const std::string& cuerpo);

crow::response respuestaError(int codigo, const std::string& mensaje);

}  // namespace ApiHttp
