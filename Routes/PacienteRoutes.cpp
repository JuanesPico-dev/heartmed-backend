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

    // Alias de estado_salud: el dashboard del front lee "estado" para contar
    // pacientes activos y críticos. Se envía duplicado para no obligar al
    // front a cambiar mientras se unifica el nombre del campo.
    json["estado"] = paciente.getEstadoSalud();
    return json;
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

            nlohmann::json cuerpoPeticion;
            try {
                cuerpoPeticion = nlohmann::json::parse(peticion.body);
            } catch (const std::exception&) {
                return ApiHttp::respuestaError(400, "JSON inválido");
            }

            if (!cuerpoPeticion.is_object()) {
                return ApiHttp::respuestaError(400, "Se esperaba un objeto JSON");
            }

            Paciente datos;
            try {
                datos.setDocumentoIdentidad(cuerpoPeticion.value("documento_identidad", ""));
                datos.setNombreCompleto(cuerpoPeticion.value("nombre_completo", ""));
                datos.setFotoPerfil(cuerpoPeticion.value("foto_perfil", ""));
                datos.setFechaNacimiento(cuerpoPeticion.value("fecha_nacimiento", ""));
                datos.setTipoSangre(cuerpoPeticion.value("tipo_sangre", ""));
                datos.setRangoCargo(cuerpoPeticion.value("rango_cargo", ""));
                datos.setRecompensa(cuerpoPeticion.value("recompensa", static_cast<long long>(0)));

                // Acepta "estado_salud" y, por comodidad del front, "estado".
                std::string estado = cuerpoPeticion.value("estado_salud", "");
                if (estado.empty()) {
                    estado = cuerpoPeticion.value("estado", "");
                }
                datos.setEstadoSalud(estado);

                if (cuerpoPeticion.contains("id_usuario") && !cuerpoPeticion["id_usuario"].is_null()) {
                    datos.setIdUsuario(cuerpoPeticion["id_usuario"].get<long long>());
                }
            } catch (const std::exception&) {
                return ApiHttp::respuestaError(400, "Alguno de los campos tiene un tipo de dato incorrecto");
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
}
