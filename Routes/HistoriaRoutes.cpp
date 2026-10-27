#include "Routes/HistoriaRoutes.h"

#include <nlohmann/json.hpp>
#include <string>
#include <vector>

#include "Cross/ApiHttp.h"
#include "Models/Alergia.h"
#include "Models/HistoriaClinica.h"

namespace {

nlohmann::json aJson(const Alergia& alergia) {
    nlohmann::json json;
    json["id_alergia"] = alergia.getIdAlergia();
    json["sustancia"] = alergia.getSustancia();

    if (alergia.tieneIdMedicamento()) {
        json["id_medicamento"] = alergia.getIdMedicamento();
        json["nombre_medicamento"] = alergia.getNombreMedicamento();
    } else {
        json["id_medicamento"] = nullptr;
        json["nombre_medicamento"] = nullptr;
    }

    return json;
}

// En la base "sin dato" es NULL, y así viaja al cliente: null y "" significan
// cosas distintas en una historia clínica.
nlohmann::json textoONulo(const std::string& valor) {
    if (valor.empty()) {
        return nullptr;
    }
    return valor;
}

nlohmann::json aJson(const HistoriaClinica& historia) {
    nlohmann::json json;
    json["id_historia"] = historia.getIdHistoria();
    json["id_paciente"] = historia.getIdPaciente();
    json["nombre_paciente"] = historia.getNombrePaciente();
    json["alergias_revisadas"] = historia.getAlergiasRevisadas();
    json["antecedentes"] = textoONulo(historia.getAntecedentes());
    json["cirugias_previas"] = textoONulo(historia.getCirugiasPrevias());
    json["observaciones"] = textoONulo(historia.getObservaciones());
    json["fruta_diablo"] = historia.getFrutaDiablo();
    json["fecha_creacion"] = historia.getFechaCreacion();

    nlohmann::json alergias = nlohmann::json::array();
    for (const Alergia& alergia : historia.getAlergias()) {
        alergias.push_back(aJson(alergia));
    }
    json["alergias"] = alergias;

    return json;
}

// Un campo de texto ausente se deja sin tocar; uno a null o "" borra el dato.
void leerTextoOpcional(const nlohmann::json& cuerpo,
                       const char* clave,
                       std::optional<std::string>& destino) {
    if (!cuerpo.contains(clave)) {
        return;
    }

    if (cuerpo[clave].is_null()) {
        destino = std::string();
        return;
    }

    destino = cuerpo[clave].get<std::string>();
}

bool leerCuerpo(const std::string& textoJson, CambiosHistoria& cambios, std::string& error) {
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
        leerTextoOpcional(cuerpo, "antecedentes", cambios.antecedentes);
        leerTextoOpcional(cuerpo, "cirugias_previas", cambios.cirugiasPrevias);
        leerTextoOpcional(cuerpo, "observaciones", cambios.observaciones);

        if (cuerpo.contains("fruta_diablo") && !cuerpo["fruta_diablo"].is_null()) {
            cambios.frutaDiablo = cuerpo["fruta_diablo"].get<bool>();
        }

        if (cuerpo.contains("alergias_revisadas") && !cuerpo["alergias_revisadas"].is_null()) {
            cambios.alergiasRevisadas = cuerpo["alergias_revisadas"].get<bool>();
        }

        if (cuerpo.contains("alergias") && !cuerpo["alergias"].is_null()) {
            if (!cuerpo["alergias"].is_array()) {
                error = "El campo alergias debe ser una lista";
                return false;
            }

            std::vector<Alergia> alergias;
            for (const auto& entrada : cuerpo["alergias"]) {
                Alergia alergia;

                // Se acepta tanto {"sustancia": "..."} como el texto suelto,
                // que es como lo escribiría un formulario sencillo.
                if (entrada.is_string()) {
                    alergia.setSustancia(entrada.get<std::string>());
                } else if (entrada.is_object()) {
                    alergia.setSustancia(entrada.value("sustancia", ""));
                    if (entrada.contains("id_medicamento") && !entrada["id_medicamento"].is_null()) {
                        alergia.setIdMedicamento(entrada["id_medicamento"].get<long long>());
                    }
                } else {
                    error = "Cada alergia debe ser un texto o un objeto con sustancia";
                    return false;
                }

                alergias.push_back(alergia);
            }

            cambios.alergias = alergias;
        }
    } catch (const std::exception&) {
        error = "Alguno de los campos tiene un tipo de dato incorrecto";
        return false;
    }

    error.clear();
    return true;
}

}  // namespace

HistoriaRoutes::HistoriaRoutes(HistoriaService& historiaService)
    : historiaService_(historiaService) {
}

void HistoriaRoutes::registrarRutas(crow::App<crow::CORSHandler>& app) {
    CROW_ROUTE(app, "/api/pacientes/<int>/historia")
        .methods(crow::HTTPMethod::Get)([this](const crow::request& peticion, int idPaciente) {
            std::string correo;
            std::string rol;
            if (!ApiHttp::peticionAutorizada(peticion, correo, rol)) {
                return ApiHttp::respuestaError(401, "Token ausente o inválido");
            }

            HistoriaClinica historia;
            std::string mensaje;
            const ResultadoOperacion resultado = historiaService_.consultar(idPaciente, historia, mensaje);

            if (resultado != ResultadoOperacion::Exito) {
                return ApiHttp::respuestaError(ApiHttp::codigoHttpDe(resultado), mensaje);
            }

            nlohmann::json cuerpo;
            cuerpo["success"] = true;
            cuerpo["historia"] = aJson(historia);
            return ApiHttp::respuestaJson(200, cuerpo.dump());
        });

    CROW_ROUTE(app, "/api/pacientes/<int>/historia")
        .methods(crow::HTTPMethod::Put)([this](const crow::request& peticion, int idPaciente) {
            std::string correo;
            std::string rol;
            if (!ApiHttp::peticionAutorizada(peticion, correo, rol)) {
                return ApiHttp::respuestaError(401, "Token ausente o inválido");
            }

            CambiosHistoria cambios;
            std::string errorCuerpo;
            if (!leerCuerpo(peticion.body, cambios, errorCuerpo)) {
                return ApiHttp::respuestaError(400, errorCuerpo);
            }

            HistoriaClinica actualizada;
            std::string mensaje;
            const ResultadoOperacion resultado =
                historiaService_.actualizar(idPaciente, cambios, actualizada, mensaje);

            if (resultado != ResultadoOperacion::Exito) {
                return ApiHttp::respuestaError(ApiHttp::codigoHttpDe(resultado), mensaje);
            }

            nlohmann::json cuerpo;
            cuerpo["success"] = true;
            cuerpo["message"] = mensaje;
            cuerpo["historia"] = aJson(actualizada);
            return ApiHttp::respuestaJson(200, cuerpo.dump());
        });
}
