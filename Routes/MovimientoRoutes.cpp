#include "Routes/MovimientoRoutes.h"

#include <nlohmann/json.hpp>
#include <string>

#include "Cross/ApiHttp.h"
#include "Models/MovimientoInventario.h"

namespace {

nlohmann::json aJson(const MovimientoInventario& movimiento) {
    nlohmann::json json;
    json["id_movimiento"] = movimiento.getIdMovimiento();
    json["id_lote"] = movimiento.getIdLote();
    json["id_usuario"] = movimiento.getIdUsuario();

    if (movimiento.tieneIdTratamiento()) {
        json["id_tratamiento"] = movimiento.getIdTratamiento();
    } else {
        json["id_tratamiento"] = nullptr;
    }

    json["tipo_movimiento"] = movimiento.getTipoMovimiento();
    json["cantidad"] = movimiento.getCantidad();
    json["fecha_movimiento"] = movimiento.getFechaMovimiento();

    if (movimiento.getMotivo().empty()) {
        json["motivo"] = nullptr;
    } else {
        json["motivo"] = movimiento.getMotivo();
    }

    return json;
}

// Cómo queda el inventario después del movimiento. Se devuelve para que el
// cliente no tenga que volver a pedir /api/inventario entero.
nlohmann::json aJson(const StockResultante& stock) {
    nlohmann::json json;
    json["id_lote"] = stock.idLote;
    json["num_lote"] = stock.numLote;
    json["cantidad_lote"] = stock.cantidadLote;
    json["id_medicamento"] = stock.idMedicamento;
    json["nombre_medicamento"] = stock.nombreMedicamento;
    json["cantidad_total_disponible"] = stock.cantidadTotalDisponible;
    json["umbral_minimo"] = stock.umbralMinimo;
    json["bajo_minimo"] = stock.cantidadTotalDisponible < stock.umbralMinimo;
    return json;
}

}  // namespace

MovimientoRoutes::MovimientoRoutes(MovimientoService& movimientoService)
    : movimientoService_(movimientoService) {
}

void MovimientoRoutes::registrarRutas(crow::App<crow::CORSHandler>& app) {
    CROW_ROUTE(app, "/api/movimientos")
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

            MovimientoInventario datos;
            try {
                datos.setIdLote(cuerpoPeticion.value("id_lote", static_cast<long long>(0)));
                datos.setTipoMovimiento(cuerpoPeticion.value("tipo_movimiento", ""));
                datos.setCantidad(cuerpoPeticion.value("cantidad", static_cast<long long>(0)));
                datos.setMotivo(cuerpoPeticion.value("motivo", ""));

                if (cuerpoPeticion.contains("id_tratamiento") &&
                    !cuerpoPeticion["id_tratamiento"].is_null()) {
                    datos.setIdTratamiento(cuerpoPeticion["id_tratamiento"].get<long long>());
                }
            } catch (const std::exception&) {
                return ApiHttp::respuestaError(400, "Alguno de los campos tiene un tipo de dato incorrecto");
            }

            // id_usuario se ignora si viene en el cuerpo: lo pone el servicio a
            // partir del token.
            MovimientoInventario registrado;
            StockResultante stock;
            std::string mensaje;
            const ResultadoOperacion resultado =
                movimientoService_.registrar(datos, correo, registrado, stock, mensaje);

            if (resultado != ResultadoOperacion::Exito) {
                return ApiHttp::respuestaError(ApiHttp::codigoHttpDe(resultado), mensaje);
            }

            nlohmann::json cuerpo;
            cuerpo["success"] = true;
            cuerpo["message"] = mensaje;
            cuerpo["movimiento"] = aJson(registrado);
            cuerpo["stock"] = aJson(stock);
            return crow::response(201, cuerpo.dump());
        });
}
