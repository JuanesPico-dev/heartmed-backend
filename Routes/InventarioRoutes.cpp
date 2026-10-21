#include "Routes/InventarioRoutes.h"

#include <nlohmann/json.hpp>
#include <string>
#include <vector>

#include "Cross/ApiHttp.h"
#include "Models/Lote.h"
#include "Models/Medicamento.h"

namespace {

nlohmann::json aJson(const Lote& lote) {
    nlohmann::json json;
    json["id_lote"] = lote.getIdLote();
    json["num_lote"] = lote.getNumLote();
    json["ubicacion"] = lote.getUbicacion();
    json["fecha_vencimiento"] = lote.getFechaVencimiento();
    json["cantidad"] = lote.getCantidad();
    return json;
}

nlohmann::json aJson(const Medicamento& medicamento) {
    nlohmann::json json;
    json["id_medicamento"] = medicamento.getIdMedicamento();
    json["nombre"] = medicamento.getNombre();
    json["tipo_insumo"] = medicamento.getTipoInsumo();
    json["unidad_medida"] = medicamento.getUnidadMedida();

    if (medicamento.getDescripcion().empty()) {
        json["descripcion"] = nullptr;
    } else {
        json["descripcion"] = medicamento.getDescripcion();
    }

    json["cantidad_total_disponible"] = medicamento.getCantidadTotalDisponible();
    json["umbral_minimo"] = medicamento.getUmbralMinimo();
    json["bajo_minimo"] = medicamento.estaBajoMinimo();

    // Alias para compatibilidad con el frontend.
    json["cantidad"] = medicamento.getCantidadTotalDisponible();
    json["stock_minimo"] = medicamento.getUmbralMinimo();

    nlohmann::json lotes = nlohmann::json::array();
    for (const Lote& lote : medicamento.getLotes()) {
        lotes.push_back(aJson(lote));
    }
    json["lotes"] = lotes;

    return json;
}

}  // namespace

InventarioRoutes::InventarioRoutes(InventarioService& inventarioService)
    : inventarioService_(inventarioService) {
}

void InventarioRoutes::registrarRutas(crow::App<crow::CORSHandler>& app) {
    CROW_ROUTE(app, "/api/inventario")
        .methods(crow::HTTPMethod::Get)([this](const crow::request& peticion) {
            std::string correo;
            std::string rol;
            if (!ApiHttp::peticionAutorizada(peticion, correo, rol)) {
                return ApiHttp::respuestaError(401, "Token ausente o inválido");
            }

            std::vector<Medicamento> medicamentos;
            ResumenInventario resumen;
            std::string mensaje;
            const ResultadoOperacion resultado = inventarioService_.listar(medicamentos, resumen, mensaje);

            if (resultado != ResultadoOperacion::Exito) {
                return ApiHttp::respuestaError(ApiHttp::codigoHttpDe(resultado), mensaje);
            }

            nlohmann::json items = nlohmann::json::array();
            for (const Medicamento& medicamento : medicamentos) {
                items.push_back(aJson(medicamento));
            }

            nlohmann::json cuerpo;
            cuerpo["success"] = true;
            cuerpo["total"] = resumen.total;
            cuerpo["bajo_minimo"] = resumen.bajoMinimo;

            // Si no hay datos, devuelve null para diferenciarlo de 0%.
            if (resumen.hayDatos) {
                cuerpo["porcentaje"] = resumen.porcentajeDisponibilidad;
            } else {
                cuerpo["porcentaje"] = nullptr;
            }

            cuerpo["items"] = items;
            return ApiHttp::respuestaJson(200, cuerpo.dump());
        });
}
