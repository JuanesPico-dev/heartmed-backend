#include "Routes/PacienteRoutes.h"

#include <nlohmann/json.hpp>
#include <string>
#include <vector>

#include "Cross/ApiHttp.h"
#include "Models/Paciente.h"

namespace {

nlohmann::json aJson(const Paciente& paciente) {
    nlohmann::json json;
    json["id_paciente"] = paciente.getIdPaciente();

    if (paciente.tieneIdUsuario()) {
        json["id_usuario"] = paciente.getIdUsuario();
    } else {
        json["id_usuario"] = nullptr;
    }

    json["documento_identidad"] = paciente.getDocumentoIdentidad();
    json["nombre_completo"] = paciente.getNombreCompleto();

    if (paciente.getFotoPerfil().empty()) {
        json["foto_perfil"] = nullptr;
    } else {
        json["foto_perfil"] = paciente.getFotoPerfil();
    }

    json["fecha_nacimiento"] = paciente.getFechaNacimiento();
    json["tipo_sangre"] = paciente.getTipoSangre();
    json["rango_cargo"] = paciente.getRangoCargo();
    json["recompensa"] = paciente.getRecompensa();
    json["estado_salud"] = paciente.getEstadoSalud();

    // Viene de la historia clínica del paciente, que es 1 a 1 con él.
    if (paciente.getObservaciones().empty()) {
        json["observaciones"] = nullptr;
    } else {
        json["observaciones"] = paciente.getObservaciones();
    }

    // Alias de estado_salud: el dashboard del front lee "estado" para contar
    // pacientes activos y críticos. Se envía duplicado para no obligar al
    // front a cambiar mientras se unifica el nombre del campo.
    json["estado"] = paciente.getEstadoSalud();
    return json;
}

// Pasa el JSON recibido a un Paciente. Devuelve false y deja el motivo en
// "error" si el cuerpo no es un objeto o algún campo tiene el tipo cambiado.
// Lo comparten el alta y la edición, que reciben la misma forma.
bool leerCuerpo(const std::string& textoJson, Paciente& datos, std::string& error) {
    nlohmann::json cuerpo;
    try {
        cuerpo = nlohmann::json::parse(textoJson);
    } catch (const std::exception&) {
        error = "JSON inválido";
        return false;
    }

    if (!cuerpo.is_object()) {
        error = "Se esperaba un objeto JSON";
        return false;
    }

    try {
        datos.setDocumentoIdentidad(cuerpo.value("documento_identidad", ""));
        datos.setNombreCompleto(cuerpo.value("nombre_completo", ""));
        datos.setFotoPerfil(cuerpo.value("foto_perfil", ""));
        datos.setFechaNacimiento(cuerpo.value("fecha_nacimiento", ""));
        datos.setTipoSangre(cuerpo.value("tipo_sangre", ""));
        datos.setRangoCargo(cuerpo.value("rango_cargo", ""));
        datos.setRecompensa(cuerpo.value("recompensa", static_cast<long long>(0)));
        datos.setObservaciones(cuerpo.value("observaciones", ""));

        // Acepta "estado_salud" y, por comodidad del front, "estado".
        std::string estado = cuerpo.value("estado_salud", "");
        if (estado.empty()) {
            estado = cuerpo.value("estado", "");
        }
        datos.setEstadoSalud(estado);

        if (cuerpo.contains("id_usuario") && !cuerpo["id_usuario"].is_null()) {
            datos.setIdUsuario(cuerpo["id_usuario"].get<long long>());
        }
    } catch (const std::exception&) {
        error = "Alguno de los campos tiene un tipo de dato incorrecto";
        return false;
    }

    error.clear();
    return true;
}

}  // namespace

PacienteRoutes::PacienteRoutes(PacienteService& pacienteService)
    : pacienteService_(pacienteService) {
}

void PacienteRoutes::registrarRutas(crow::App<crow::CORSHandler>& app) {
    CROW_ROUTE(app, "/api/pacientes")
        .methods(crow::HTTPMethod::Get)([this](const crow::request& peticion) {
            std::string correo;
            std::string rol;
            if (!ApiHttp::peticionAutorizada(peticion, correo, rol)) {
                return ApiHttp::respuestaError(401, "Token ausente o inválido");
            }

            std::vector<Paciente> pacientes;
            std::string mensaje;
            const ResultadoOperacion resultado = pacienteService_.listar(pacientes, mensaje);

            if (resultado != ResultadoOperacion::Exito) {
                return ApiHttp::respuestaError(ApiHttp::codigoHttpDe(resultado), mensaje);
            }

            nlohmann::json lista = nlohmann::json::array();
            for (const Paciente& paciente : pacientes) {
                lista.push_back(aJson(paciente));
            }

            nlohmann::json cuerpo;
            cuerpo["success"] = true;
            cuerpo["total"] = lista.size();
            cuerpo["pacientes"] = lista;
            return crow::response(200, cuerpo.dump());
        });

    CROW_ROUTE(app, "/api/pacientes")
        .methods(crow::HTTPMethod::Post)([this](const crow::request& peticion) {
            std::string correo;
            std::string rol;
            if (!ApiHttp::peticionAutorizada(peticion, correo, rol)) {
                return ApiHttp::respuestaError(401, "Token ausente o inválido");
            }

            Paciente datos;
            std::string errorCuerpo;
            if (!leerCuerpo(peticion.body, datos, errorCuerpo)) {
                return ApiHttp::respuestaError(400, errorCuerpo);
            }

            Paciente creado;
            std::string mensaje;
            const ResultadoOperacion resultado = pacienteService_.crear(datos, creado, mensaje);

            if (resultado != ResultadoOperacion::Exito) {
                return ApiHttp::respuestaError(ApiHttp::codigoHttpDe(resultado), mensaje);
            }

            nlohmann::json cuerpo;
            cuerpo["success"] = true;
            cuerpo["message"] = mensaje;
            cuerpo["paciente"] = aJson(creado);
            return crow::response(201, cuerpo.dump());
        });

    CROW_ROUTE(app, "/api/pacientes/<int>")
        .methods(crow::HTTPMethod::Put)([this](const crow::request& peticion, int idPaciente) {
            std::string correo;
            std::string rol;
            if (!ApiHttp::peticionAutorizada(peticion, correo, rol)) {
                return ApiHttp::respuestaError(401, "Token ausente o inválido");
            }

            Paciente datos;
            std::string errorCuerpo;
            if (!leerCuerpo(peticion.body, datos, errorCuerpo)) {
                return ApiHttp::respuestaError(400, errorCuerpo);
            }

            Paciente actualizado;
            std::string mensaje;
            const ResultadoOperacion resultado =
                pacienteService_.actualizar(idPaciente, datos, actualizado, mensaje);

            if (resultado != ResultadoOperacion::Exito) {
                return ApiHttp::respuestaError(ApiHttp::codigoHttpDe(resultado), mensaje);
            }

            nlohmann::json cuerpo;
            cuerpo["success"] = true;
            cuerpo["message"] = mensaje;
            cuerpo["paciente"] = aJson(actualizado);
            return crow::response(200, cuerpo.dump());
        });
}
