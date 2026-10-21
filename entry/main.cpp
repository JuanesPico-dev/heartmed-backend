#include "crow.h"
#include "crow/middlewares/cors.h"

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>

#include <nlohmann/json.hpp>

#include "Cross/ApiHttp.h"
#include "Cross/Config.h"
#include "Database/SQLite.h"
#include "Routes/AuthRoutes.h"
#include "Routes/InventarioRoutes.h"
#include "Routes/MovimientoRoutes.h"
#include "Routes/PacienteRoutes.h"
#include "Services/AuthService.h"
#include "Services/InventarioService.h"
#include "Services/MovimientoService.h"
#include "Services/PacienteService.h"

int main() {
    const std::filesystem::path rutaArchivoEnv = ".env";
    Configuracion::instancia().cargarDesdeArchivo(rutaArchivoEnv.string());

    // Usa la DB completa del proyecto si no viene en .env.
    const std::string rutaBaseDatos =
        Configuracion::instancia().obtenerCadena("DB_PATH", "./Database/heartmed_db.sqlite3");
    BaseDatosSqlite::instancia().abrir(rutaBaseDatos);

    const char* puertoEntorno = std::getenv("PORT");
    int puerto = Configuracion::instancia().obtenerEntero("PORT", 18080);

    if (puertoEntorno != nullptr && puertoEntorno[0] != '\0') {
        try {
            puerto = std::stoi(puertoEntorno);
        } catch (const std::exception&) {
            std::cerr << "La variable PORT no es válida. Se usará el puerto " << puerto << std::endl;
        }
    }

    crow::App<crow::CORSHandler> aplicacion;
    auto& cors = aplicacion.get_middleware<crow::CORSHandler>();
    cors.global()
        .origin("http://localhost:5173")
        .methods("GET"_method, "POST"_method, "PUT"_method, "PATCH"_method, "DELETE"_method, "OPTIONS"_method)
        .headers("Origin", "Content-Type", "Authorization", "Accept")
        .max_age(86400);

    AuthService servicioAutenticacion;
    AuthRoutes rutasAutenticacion(servicioAutenticacion);
    rutasAutenticacion.registrarRutas(aplicacion);

    PacienteService servicioPacientes;
    PacienteRoutes rutasPacientes(servicioPacientes);
    rutasPacientes.registrarRutas(aplicacion);

    InventarioService servicioInventario;
    InventarioRoutes rutasInventario(servicioInventario);
    rutasInventario.registrarRutas(aplicacion);

    MovimientoService servicioMovimientos;
    MovimientoRoutes rutasMovimientos(servicioMovimientos);
    rutasMovimientos.registrarRutas(aplicacion);

    CROW_ROUTE(aplicacion, "/health")([]() {
        nlohmann::json respuesta;
        respuesta["status"] = "ok";
        respuesta["service"] = "heartmed-backend";
        return ApiHttp::respuestaJson(200, respuesta.dump());
    });

    CROW_ROUTE(aplicacion, "/api/health")([]() {
        nlohmann::json respuesta;
        respuesta["status"] = "ok";
        respuesta["message"] = "Backend de HeartMed funcionando";
        respuesta["database"] = Configuracion::instancia().obtenerCadena("DB_NAME", "heartmed_db");
        return ApiHttp::respuestaJson(200, respuesta.dump());
    });

    std::cout << "Configuración cargada desde .env" << std::endl;
    std::cout << "Base de datos: " << rutaBaseDatos << std::endl;
    std::cout << "Iniciando servidor Crow en puerto " << puerto << std::endl;
    aplicacion.port(puerto).multithreaded().run();

    return 0;
}
