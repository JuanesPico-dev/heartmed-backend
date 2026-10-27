#include "Repositories/HistoriaRepository.h"

#include <string>
#include <vector>

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

bool ejecutar(sqlite3* conexion, const char* sql) {
    return sqlite3_exec(conexion, sql, nullptr, nullptr, nullptr) == SQLITE_OK;
}

// Enlaza un texto, o NULL si viene vacío: en la base "sin dato" es NULL, no
// una cadena en blanco.
void enlazarTextoONulo(sqlite3_stmt* sentencia, int posicion, const std::string& valor) {
    if (valor.empty()) {
        sqlite3_bind_null(sentencia, posicion);
    } else {
        sqlite3_bind_text(sentencia, posicion, valor.c_str(), static_cast<int>(valor.size()), SQLITE_TRANSIENT);
    }
}

bool cargarAlergias(sqlite3* conexion, HistoriaClinica& historia) {
    const std::string sql =
        "SELECT a.id_alergia, a.sustancia, a.id_medicamento, m.nombre "
        "FROM alergias a LEFT JOIN medicamentos m ON m.id_medicamento = a.id_medicamento "
        "WHERE a.id_historia = ? ORDER BY a.sustancia;";

    sqlite3_stmt* sentencia = nullptr;
    if (sqlite3_prepare_v2(conexion, sql.c_str(), -1, &sentencia, nullptr) != SQLITE_OK) {
        return false;
    }

    sqlite3_bind_int64(sentencia, 1, historia.getIdHistoria());

    while (sqlite3_step(sentencia) == SQLITE_ROW) {
        Alergia alergia;
        alergia.setIdAlergia(sqlite3_column_int64(sentencia, 0));
        alergia.setSustancia(leerTexto(sentencia, 1));

        if (sqlite3_column_type(sentencia, 2) == SQLITE_NULL) {
            alergia.quitarIdMedicamento();
        } else {
            alergia.setIdMedicamento(sqlite3_column_int64(sentencia, 2));
        }

        alergia.setNombreMedicamento(leerTexto(sentencia, 3));
        historia.agregarAlergia(alergia);
    }

    sqlite3_finalize(sentencia);
    return true;
}

}  // namespace

bool HistoriaRepository::existePaciente(long long idPaciente) const {
    return existeEn("SELECT 1 FROM pacientes WHERE id_paciente = ? LIMIT 1;", idPaciente);
}

bool HistoriaRepository::existeMedicamento(long long idMedicamento) const {
    return existeEn("SELECT 1 FROM medicamentos WHERE id_medicamento = ? LIMIT 1;", idMedicamento);
}

bool HistoriaRepository::buscarPorPaciente(long long idPaciente,
                                           HistoriaClinica& historia,
                                           std::string& error) const {
    sqlite3* conexion = BaseDatosSqlite::instancia().obtenerConexion();
    if (conexion == nullptr || !BaseDatosSqlite::instancia().estaAbierta()) {
        error = "La base de datos no está disponible";
        return false;
    }

    const std::string sql =
        "SELECT h.id_historia, h.id_paciente, p.nombre_completo, h.alergias_revisadas, "
        "       h.antecedentes, h.cirugias_previas, h.observaciones, h.fruta_diablo, h.fecha_creacion "
        "FROM historias_clinicas h JOIN pacientes p ON p.id_paciente = h.id_paciente "
        "WHERE h.id_paciente = ? LIMIT 1;";

    sqlite3_stmt* sentencia = nullptr;
    if (sqlite3_prepare_v2(conexion, sql.c_str(), -1, &sentencia, nullptr) != SQLITE_OK) {
        error = sqlite3_errmsg(conexion);
        return false;
    }

    sqlite3_bind_int64(sentencia, 1, idPaciente);

    bool encontrada = false;
    if (sqlite3_step(sentencia) == SQLITE_ROW) {
        historia.setIdHistoria(sqlite3_column_int64(sentencia, 0));
        historia.setIdPaciente(sqlite3_column_int64(sentencia, 1));
        historia.setNombrePaciente(leerTexto(sentencia, 2));
        historia.setAlergiasRevisadas(sqlite3_column_int(sentencia, 3) != 0);
        historia.setAntecedentes(leerTexto(sentencia, 4));
        historia.setCirugiasPrevias(leerTexto(sentencia, 5));
        historia.setObservaciones(leerTexto(sentencia, 6));
        historia.setFrutaDiablo(sqlite3_column_int(sentencia, 7) != 0);
        historia.setFechaCreacion(leerTexto(sentencia, 8));
        encontrada = true;
    }

    sqlite3_finalize(sentencia);

    if (!encontrada) {
        error.clear();
        return false;
    }

    cargarAlergias(conexion, historia);
    error.clear();
    return true;
}

bool HistoriaRepository::actualizar(long long idHistoria,
                                    const CambiosHistoria& cambios,
                                    std::string& error) const {
    sqlite3* conexion = BaseDatosSqlite::instancia().obtenerConexion();
    if (conexion == nullptr || !BaseDatosSqlite::instancia().estaAbierta()) {
        error = "La base de datos no está disponible";
        return false;
    }

    if (!ejecutar(conexion, "BEGIN IMMEDIATE;")) {
        error = sqlite3_errmsg(conexion);
        return false;
    }

    // Se arma el UPDATE solo con los campos que llegaron, para no pisar con
    // NULL los que el cliente no envió.
    std::string asignaciones;
    std::vector<const std::string*> textos;
    std::vector<int> enteros;

    if (cambios.antecedentes.has_value()) {
        asignaciones += (asignaciones.empty() ? "" : ", ");
        asignaciones += "antecedentes = ?";
        textos.push_back(&cambios.antecedentes.value());
    }

    if (cambios.cirugiasPrevias.has_value()) {
        asignaciones += (asignaciones.empty() ? "" : ", ");
        asignaciones += "cirugias_previas = ?";
        textos.push_back(&cambios.cirugiasPrevias.value());
    }

    if (cambios.observaciones.has_value()) {
        asignaciones += (asignaciones.empty() ? "" : ", ");
        asignaciones += "observaciones = ?";
        textos.push_back(&cambios.observaciones.value());
    }

    if (cambios.frutaDiablo.has_value()) {
        asignaciones += (asignaciones.empty() ? "" : ", ");
        asignaciones += "fruta_diablo = ?";
        enteros.push_back(cambios.frutaDiablo.value() ? 1 : 0);
    }

    if (!asignaciones.empty()) {
        const std::string sql =
            "UPDATE historias_clinicas SET " + asignaciones + " WHERE id_historia = ?;";

        sqlite3_stmt* sentencia = nullptr;
        if (sqlite3_prepare_v2(conexion, sql.c_str(), -1, &sentencia, nullptr) != SQLITE_OK) {
            error = sqlite3_errmsg(conexion);
            ejecutar(conexion, "ROLLBACK;");
            return false;
        }

        int posicion = 1;
        for (const std::string* texto : textos) {
            enlazarTextoONulo(sentencia, posicion++, *texto);
        }
        for (int entero : enteros) {
            sqlite3_bind_int(sentencia, posicion++, entero);
        }
        sqlite3_bind_int64(sentencia, posicion, idHistoria);

        if (sqlite3_step(sentencia) != SQLITE_DONE) {
            error = sqlite3_errmsg(conexion);
            sqlite3_finalize(sentencia);
            ejecutar(conexion, "ROLLBACK;");
            return false;
        }

        sqlite3_finalize(sentencia);
    }

    // Las alergias se reemplazan en bloque: es la forma de que el formulario
    // pueda quitar una que se registró por error.
    if (cambios.alergias.has_value()) {
        sqlite3_stmt* borrado = nullptr;
        if (sqlite3_prepare_v2(conexion, "DELETE FROM alergias WHERE id_historia = ?;", -1, &borrado,
                               nullptr) != SQLITE_OK) {
            error = sqlite3_errmsg(conexion);
            ejecutar(conexion, "ROLLBACK;");
            return false;
        }

        sqlite3_bind_int64(borrado, 1, idHistoria);
        if (sqlite3_step(borrado) != SQLITE_DONE) {
            error = sqlite3_errmsg(conexion);
            sqlite3_finalize(borrado);
            ejecutar(conexion, "ROLLBACK;");
            return false;
        }
        sqlite3_finalize(borrado);

        for (const Alergia& alergia : cambios.alergias.value()) {
            sqlite3_stmt* insercion = nullptr;
            if (sqlite3_prepare_v2(conexion,
                                   "INSERT INTO alergias (id_historia, sustancia, id_medicamento) "
                                   "VALUES (?, ?, ?);",
                                   -1, &insercion, nullptr) != SQLITE_OK) {
                error = sqlite3_errmsg(conexion);
                ejecutar(conexion, "ROLLBACK;");
                return false;
            }

            const std::string sustancia = alergia.getSustancia();
            sqlite3_bind_int64(insercion, 1, idHistoria);
            sqlite3_bind_text(insercion, 2, sustancia.c_str(), static_cast<int>(sustancia.size()),
                              SQLITE_TRANSIENT);

            if (alergia.tieneIdMedicamento()) {
                sqlite3_bind_int64(insercion, 3, alergia.getIdMedicamento());
            } else {
                sqlite3_bind_null(insercion, 3);
            }

            if (sqlite3_step(insercion) != SQLITE_DONE) {
                error = sqlite3_errmsg(conexion);
                sqlite3_finalize(insercion);
                ejecutar(conexion, "ROLLBACK;");
                return false;
            }

            sqlite3_finalize(insercion);
        }
    }

    // Va al final a propósito: el trigger trg_alergias_ai_historia_revisada
    // pone la marca a 1 al insertar la primera alergia, y aquí manda lo que
    // diga el servicio (puede ser "revisadas y ninguna").
    if (cambios.alergiasRevisadas.has_value()) {
        sqlite3_stmt* sentencia = nullptr;
        if (sqlite3_prepare_v2(conexion,
                               "UPDATE historias_clinicas SET alergias_revisadas = ? WHERE id_historia = ?;",
                               -1, &sentencia, nullptr) != SQLITE_OK) {
            error = sqlite3_errmsg(conexion);
            ejecutar(conexion, "ROLLBACK;");
            return false;
        }

        sqlite3_bind_int(sentencia, 1, cambios.alergiasRevisadas.value() ? 1 : 0);
        sqlite3_bind_int64(sentencia, 2, idHistoria);

        if (sqlite3_step(sentencia) != SQLITE_DONE) {
            error = sqlite3_errmsg(conexion);
            sqlite3_finalize(sentencia);
            ejecutar(conexion, "ROLLBACK;");
            return false;
        }

        sqlite3_finalize(sentencia);
    }

    if (!ejecutar(conexion, "COMMIT;")) {
        error = sqlite3_errmsg(conexion);
        ejecutar(conexion, "ROLLBACK;");
        return false;
    }

    error.clear();
    return true;
}
