#include "Repositories/InventarioRepository.h"

#include <map>
#include <string>
#include <vector>

#include "Database/SQLite.h"
#include "Models/Lote.h"

namespace {

std::string leerTexto(sqlite3_stmt* sentencia, int columna) {
    const unsigned char* valor = sqlite3_column_text(sentencia, columna);
    if (valor == nullptr) {
        return "";
    }
    return reinterpret_cast<const char*>(valor);
}

}  // namespace

bool InventarioRepository::listar(std::vector<Medicamento>& medicamentos, std::string& error) const {
    sqlite3* conexion = BaseDatosSqlite::instancia().obtenerConexion();
    if (conexion == nullptr || !BaseDatosSqlite::instancia().estaAbierta()) {
        error = "La base de datos no está disponible";
        return false;
    }

    const std::string sqlMedicamentos =
        "SELECT id_medicamento, nombre, tipo_insumo, unidad_medida, descripcion, "
        "       cantidad_total_disponible, umbral_minimo "
        "FROM medicamentos ORDER BY nombre;";

    sqlite3_stmt* sentencia = nullptr;
    if (sqlite3_prepare_v2(conexion, sqlMedicamentos.c_str(), -1, &sentencia, nullptr) != SQLITE_OK) {
        error = sqlite3_errmsg(conexion);
        return false;
    }

    medicamentos.clear();

    // Guarda en qué posición del vector quedó cada medicamento para colgarle
    // después sus lotes sin tener que recorrerlo entero por cada uno.
    std::map<long long, std::size_t> posicionPorId;

    while (sqlite3_step(sentencia) == SQLITE_ROW) {
        Medicamento medicamento;
        medicamento.setIdMedicamento(sqlite3_column_int64(sentencia, 0));
        medicamento.setNombre(leerTexto(sentencia, 1));
        medicamento.setTipoInsumo(leerTexto(sentencia, 2));
        medicamento.setUnidadMedida(leerTexto(sentencia, 3));
        medicamento.setDescripcion(leerTexto(sentencia, 4));
        medicamento.setCantidadTotalDisponible(sqlite3_column_int64(sentencia, 5));
        medicamento.setUmbralMinimo(sqlite3_column_int64(sentencia, 6));

        posicionPorId[medicamento.getIdMedicamento()] = medicamentos.size();
        medicamentos.push_back(medicamento);
    }

    sqlite3_finalize(sentencia);

    // Segunda consulta para todos los lotes de golpe: con una consulta por
    // medicamento tendríamos el problema N+1.
    const std::string sqlLotes =
        "SELECT id_lote, id_medicamento, num_lote, ubicacion, fecha_vencimiento, cantidad "
        "FROM lotes ORDER BY fecha_vencimiento, id_lote;";

    sqlite3_stmt* sentenciaLotes = nullptr;
    if (sqlite3_prepare_v2(conexion, sqlLotes.c_str(), -1, &sentenciaLotes, nullptr) != SQLITE_OK) {
        error = sqlite3_errmsg(conexion);
        return false;
    }

    while (sqlite3_step(sentenciaLotes) == SQLITE_ROW) {
        Lote lote;
        lote.setIdLote(sqlite3_column_int64(sentenciaLotes, 0));
        lote.setIdMedicamento(sqlite3_column_int64(sentenciaLotes, 1));
        lote.setNumLote(leerTexto(sentenciaLotes, 2));
        lote.setUbicacion(leerTexto(sentenciaLotes, 3));
        lote.setFechaVencimiento(leerTexto(sentenciaLotes, 4));
        lote.setCantidad(sqlite3_column_int64(sentenciaLotes, 5));

        const auto posicion = posicionPorId.find(lote.getIdMedicamento());
        if (posicion != posicionPorId.end()) {
            medicamentos[posicion->second].agregarLote(lote);
        }
    }

    sqlite3_finalize(sentenciaLotes);
    error.clear();
    return true;
}
