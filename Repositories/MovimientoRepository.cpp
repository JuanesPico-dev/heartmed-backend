#include "Repositories/MovimientoRepository.h"

#include <string>

#include "Database/SQLite.h"

namespace {

std::string leerTexto(sqlite3_stmt* sentencia, int columna) {
    const unsigned char* valor = sqlite3_column_text(sentencia, columna);
    if (valor == nullptr) {
        return "";
    }
    return reinterpret_cast<const char*>(valor);
}

bool existeEn(const char* sql, long long id) {
    sqlite3* conexion = BaseDatosSqlite::instancia().obtenerConexion();
    if (conexion == nullptr || !BaseDatosSqlite::instancia().estaAbierta()) {
        return false;
    }

    sqlite3_stmt* sentencia = nullptr;
    if (sqlite3_prepare_v2(conexion, sql, -1, &sentencia, nullptr) != SQLITE_OK) {
        return false;
    }

    sqlite3_bind_int64(sentencia, 1, id);
    const int resultado = sqlite3_step(sentencia);
    sqlite3_finalize(sentencia);
    return resultado == SQLITE_ROW;
}

}  // namespace

StockResultante::StockResultante()
    : idLote(0),
      numLote(""),
      cantidadLote(0),
      idMedicamento(0),
      nombreMedicamento(""),
      cantidadTotalDisponible(0),
      umbralMinimo(0) {
}

bool MovimientoRepository::existeLote(long long idLote) const {
    return existeEn("SELECT 1 FROM lotes WHERE id_lote = ? LIMIT 1;", idLote);
}

bool MovimientoRepository::existeTratamiento(long long idTratamiento) const {
    return existeEn("SELECT 1 FROM tratamientos WHERE id_tratamiento = ? LIMIT 1;", idTratamiento);
}

bool MovimientoRepository::crear(const MovimientoInventario& movimiento,
                                 long long& idGenerado,
                                 std::string& error) const {
    sqlite3* conexion = BaseDatosSqlite::instancia().obtenerConexion();
    if (conexion == nullptr || !BaseDatosSqlite::instancia().estaAbierta()) {
        error = "La base de datos no está disponible";
        return false;
    }

    // fecha_movimiento se deja a la base, que usa datetime('now', '-5 hours').
    const std::string sql =
        "INSERT INTO movimientos_inventario "
        "(id_lote, id_usuario, id_tratamiento, tipo_movimiento, cantidad, motivo) "
        "VALUES (?, ?, ?, ?, ?, ?);";

    sqlite3_stmt* sentencia = nullptr;
    if (sqlite3_prepare_v2(conexion, sql.c_str(), -1, &sentencia, nullptr) != SQLITE_OK) {
        error = sqlite3_errmsg(conexion);
        return false;
    }

    sqlite3_bind_int64(sentencia, 1, movimiento.getIdLote());
    sqlite3_bind_int64(sentencia, 2, movimiento.getIdUsuario());

    if (movimiento.tieneIdTratamiento()) {
        sqlite3_bind_int64(sentencia, 3, movimiento.getIdTratamiento());
    } else {
        sqlite3_bind_null(sentencia, 3);
    }

    const std::string tipo = movimiento.getTipoMovimiento();
    const std::string motivo = movimiento.getMotivo();

    sqlite3_bind_text(sentencia, 4, tipo.c_str(), static_cast<int>(tipo.size()), SQLITE_TRANSIENT);
    sqlite3_bind_int64(sentencia, 5, movimiento.getCantidad());

    if (motivo.empty()) {
        sqlite3_bind_null(sentencia, 6);
    } else {
        sqlite3_bind_text(sentencia, 6, motivo.c_str(), static_cast<int>(motivo.size()), SQLITE_TRANSIENT);
    }

    if (sqlite3_step(sentencia) != SQLITE_DONE) {
        // Aquí caen los triggers de stock insuficiente y de lote que no
        // corresponde al medicamento del tratamiento: su mensaje se propaga.
        error = sqlite3_errmsg(conexion);
        sqlite3_finalize(sentencia);
        return false;
    }

    sqlite3_finalize(sentencia);
    idGenerado = sqlite3_last_insert_rowid(conexion);
    error.clear();
    return true;
}

bool MovimientoRepository::buscarPorId(long long idMovimiento, MovimientoInventario& movimiento) const {
    sqlite3* conexion = BaseDatosSqlite::instancia().obtenerConexion();
    if (conexion == nullptr || !BaseDatosSqlite::instancia().estaAbierta()) {
        return false;
    }

    const std::string sql =
        "SELECT id_movimiento, id_lote, id_usuario, id_tratamiento, tipo_movimiento, "
        "       cantidad, fecha_movimiento, motivo "
        "FROM movimientos_inventario WHERE id_movimiento = ? LIMIT 1;";

    sqlite3_stmt* sentencia = nullptr;
    if (sqlite3_prepare_v2(conexion, sql.c_str(), -1, &sentencia, nullptr) != SQLITE_OK) {
        return false;
    }

    sqlite3_bind_int64(sentencia, 1, idMovimiento);

    bool encontrado = false;
    if (sqlite3_step(sentencia) == SQLITE_ROW) {
        movimiento.setIdMovimiento(sqlite3_column_int64(sentencia, 0));
        movimiento.setIdLote(sqlite3_column_int64(sentencia, 1));
        movimiento.setIdUsuario(sqlite3_column_int64(sentencia, 2));

        if (sqlite3_column_type(sentencia, 3) == SQLITE_NULL) {
            movimiento.quitarIdTratamiento();
        } else {
            movimiento.setIdTratamiento(sqlite3_column_int64(sentencia, 3));
        }

        movimiento.setTipoMovimiento(leerTexto(sentencia, 4));
        movimiento.setCantidad(sqlite3_column_int64(sentencia, 5));
        movimiento.setFechaMovimiento(leerTexto(sentencia, 6));
        movimiento.setMotivo(leerTexto(sentencia, 7));
        encontrado = true;
    }

    sqlite3_finalize(sentencia);
    return encontrado;
}

bool MovimientoRepository::obtenerStock(long long idLote, StockResultante& stock) const {
    sqlite3* conexion = BaseDatosSqlite::instancia().obtenerConexion();
    if (conexion == nullptr || !BaseDatosSqlite::instancia().estaAbierta()) {
        return false;
    }

    const std::string sql =
        "SELECT l.id_lote, l.num_lote, l.cantidad, m.id_medicamento, m.nombre, "
        "       m.cantidad_total_disponible, m.umbral_minimo "
        "FROM lotes l JOIN medicamentos m ON m.id_medicamento = l.id_medicamento "
        "WHERE l.id_lote = ? LIMIT 1;";

    sqlite3_stmt* sentencia = nullptr;
    if (sqlite3_prepare_v2(conexion, sql.c_str(), -1, &sentencia, nullptr) != SQLITE_OK) {
        return false;
    }

    sqlite3_bind_int64(sentencia, 1, idLote);

    bool encontrado = false;
    if (sqlite3_step(sentencia) == SQLITE_ROW) {
        stock.idLote = sqlite3_column_int64(sentencia, 0);
        stock.numLote = leerTexto(sentencia, 1);
        stock.cantidadLote = sqlite3_column_int64(sentencia, 2);
        stock.idMedicamento = sqlite3_column_int64(sentencia, 3);
        stock.nombreMedicamento = leerTexto(sentencia, 4);
        stock.cantidadTotalDisponible = sqlite3_column_int64(sentencia, 5);
        stock.umbralMinimo = sqlite3_column_int64(sentencia, 6);
        encontrado = true;
    }

    sqlite3_finalize(sentencia);
    return encontrado;
}

MovimientoDetallado::MovimientoDetallado()
    : numLote(""),
      idMedicamento(0),
      nombreMedicamento(""),
      unidadMedida(""),
      nombreUsuario("") {
}

bool MovimientoRepository::listar(long long limite,
                                  std::vector<MovimientoDetallado>& movimientos,
                                  std::string& error) const {
    sqlite3* conexion = BaseDatosSqlite::instancia().obtenerConexion();
    if (conexion == nullptr || !BaseDatosSqlite::instancia().estaAbierta()) {
        error = "La base de datos no está disponible";
        return false;
    }

    // Del más reciente al más antiguo: en un libro de movimientos lo último es
    // lo que interesa. El id desempata los que comparten fecha.
    const std::string sql =
        "SELECT m.id_movimiento, m.id_lote, m.id_usuario, m.id_tratamiento, m.tipo_movimiento, "
        "       m.cantidad, m.fecha_movimiento, m.motivo, "
        "       l.num_lote, med.id_medicamento, med.nombre, med.unidad_medida, u.nombre "
        "FROM movimientos_inventario m "
        "JOIN lotes l ON l.id_lote = m.id_lote "
        "JOIN medicamentos med ON med.id_medicamento = l.id_medicamento "
        "JOIN usuarios u ON u.id_usuario = m.id_usuario "
        "ORDER BY m.fecha_movimiento DESC, m.id_movimiento DESC LIMIT ?;";

    sqlite3_stmt* sentencia = nullptr;
    if (sqlite3_prepare_v2(conexion, sql.c_str(), -1, &sentencia, nullptr) != SQLITE_OK) {
        error = sqlite3_errmsg(conexion);
        return false;
    }

    sqlite3_bind_int64(sentencia, 1, limite);

    movimientos.clear();
    while (sqlite3_step(sentencia) == SQLITE_ROW) {
        MovimientoDetallado detalle;
        detalle.movimiento.setIdMovimiento(sqlite3_column_int64(sentencia, 0));
        detalle.movimiento.setIdLote(sqlite3_column_int64(sentencia, 1));
        detalle.movimiento.setIdUsuario(sqlite3_column_int64(sentencia, 2));

        if (sqlite3_column_type(sentencia, 3) == SQLITE_NULL) {
            detalle.movimiento.quitarIdTratamiento();
        } else {
            detalle.movimiento.setIdTratamiento(sqlite3_column_int64(sentencia, 3));
        }

        detalle.movimiento.setTipoMovimiento(leerTexto(sentencia, 4));
        detalle.movimiento.setCantidad(sqlite3_column_int64(sentencia, 5));
        detalle.movimiento.setFechaMovimiento(leerTexto(sentencia, 6));
        detalle.movimiento.setMotivo(leerTexto(sentencia, 7));

        detalle.numLote = leerTexto(sentencia, 8);
        detalle.idMedicamento = sqlite3_column_int64(sentencia, 9);
        detalle.nombreMedicamento = leerTexto(sentencia, 10);
        detalle.unidadMedida = leerTexto(sentencia, 11);
        detalle.nombreUsuario = leerTexto(sentencia, 12);

        movimientos.push_back(detalle);
    }

    sqlite3_finalize(sentencia);
    error.clear();
    return true;
}
