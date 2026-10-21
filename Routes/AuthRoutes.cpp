#include "Routes/AuthRoutes.h"

#include <nlohmann/json.hpp>
#include <string>

#include "Cross/ApiHttp.h"
#include "Cross/JwtToken.h"

AuthRoutes::AuthRoutes(AuthService& authService)
    : authService_(authService) {
}

void AuthRoutes::registrarRutas(crow::App<crow::CORSHandler>& app) {
    CROW_ROUTE(app, "/login")
        .methods(crow::HTTPMethod::Post)([this](const crow::request& req) {
            try {
                const auto body = nlohmann::json::parse(req.body);
                const std::string email = body.value("email", "");
                const std::string password = body.value("password", "");

                std::string token;
                std::string rol;
                const bool ok = authService_.login(email, password, token, rol);

                nlohmann::json response;
                response["success"] = ok;
                response["message"] = ok ? "Login correcto" : "Credenciales inválidas";

                if (ok) {
                    response["token"] = token;
                    response["rol"] = rol;
                    response["email"] = email;
                }

                return ApiHttp::respuestaJson(ok ? 200 : 401, response.dump());
            } catch (const std::exception&) {
                nlohmann::json response;
                response["success"] = false;
                response["message"] = "JSON inválido";
                return ApiHttp::respuestaJson(400, response.dump());
            }
        });
}
