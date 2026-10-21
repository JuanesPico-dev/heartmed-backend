#include "Repositories/PacienteRepository.h"

#include <string>
#include <vector>

#include "Database/SQLite.h"

namespace {

// Columnas en el mismo orden en todas las consultas, para que leerFila() sirva
// tanto en listar() como en buscarPorId(). El LEFT JOIN trae las observaciones
// de la historia clínica, que es 1 a 1 con el paciente.
const char* const COLUMNAS_PACIENTE =
    "p.id_paciente, p.id_usuario, p.documento_identidad, p.nombre_completo, p.foto_perfil, "
    "p.fecha_nacimiento, p.tipo_sangre, p.rango_cargo, p.recompensa, p.estado_salud, "
    "h.observaciones";

const char* const ORIGEN_PACIENTE =
    " FROM pacientes p LEFT JOIN historias_clinicas h ON h.id_paciente = p.id_paciente";

std::string leerTexto(sqlite3_stmt* sentencia, int columna) {
    const unsigned char* valor = sqlite3_column_text(sentencia, columna);
    if (valor == nullptr) {
        return "";
    }
    return reinterpret_cast<const char*>(valor);
}

Paciente leerFila(sqlite3_stmt* sentencia) {
    Paciente paciente;
    paciente.setIdPaciente(sqlite3_column_int64(sentencia, 0));

    if (sqlite3_column_type(sentencia, 1) == SQLITE_NULL) {
        paciente.quitarIdUsuario();
    } else {
        paciente.setIdUsuario(sqlite3_column_int64(sentencia, 1));
    }

    paciente.setDocumentoIdentidad(leerTexto(sentencia, 2));
    paciente.setNombreCompleto(leerTexto(sentencia, 3));
    paciente.setFotoPerfil(leerTexto(sentencia, 4));
    paciente.setFechaNacimiento(leerTexto(sentencia, 5));
    paciente.setTipoSangre(leerTexto(sentencia, 6));
    paciente.setRangoCargo(leerTexto(sentencia, 7));
    paciente.setRecompensa(sqlite3_column_int64(sentencia, 8));
    paciente.setEstadoSalud(leerTexto(sentencia, 9));
    paciente.setObservaciones(leerTexto(sentencia, 10));
    return paciente;
}

bool ejecutar(sqlite3* conexion, const char* sql) {
    return sqlite3_exec(conexion, sql, nullptr, nullptr, nullptr) == SQLITE_OK;
}

}  // namespace

bool PacienteRepository::listar(std::vector<Paciente>& pacientes, std::string& error) const {
    sqlite3* conexion = BaseDatosSqlite::instancia().obtenerConexion();
    if (conexion == nullptr || !BaseDatosSqlite::instancia().estaAbierta()) {
        error = "La base de datos no está disponible";
        return false;
    }

    const std::string sql =
        std::string("SELECT ") + COLUMNAS_PACIENTE + ORIGEN_PACIENTE + " ORDER BY p.id_paciente;";

    sqlite3_stmt* sentencia = nullptr;
    if (sqlite3_prepare_v2(conexion, sql.c_str(), -1, &sentencia, nullptr) != SQLITE_OK) {
        error = sqlite3_errmsg(conexion);
        return false;
    }

    pacientes.clear();
    while (sqlite3_step(sentencia) == SQLITE_ROW) {
        pacientes.push_back(leerFila(sentencia));
    }

    sqlite3_finalize(sentencia);
    error.clear();
    return true;
}

bool PacienteRepository::buscarPorId(long long idPaciente, Paciente& paciente) const {
    sqlite3* conexion = BaseDatosSqlite::instancia().obtenerConexion();
    if (conexion == nullptr || !BaseDatosSqlite::instancia().estaAbierta()) {
        return false;
    }

    const std::string sql =
        std::string("SELECT ") + COLUMNAS_PACIENTE + ORIGEN_PACIENTE + " WHERE p.id_paciente = ? LIMIT 1;";

    sqlite3_stmt* sentencia = nullptr;
    if (sqlite3_prepare_v2(conexion, sql.c_str(), -1, &sentencia, nullptr) != SQLITE_OK) {
        return false;
    }

    sqlite3_bind_int64(sentencia, 1, idPaciente);

    bool encontrado = false;
    if (sqlite3_step(sentencia) == SQLITE_ROW) {
        paciente = leerFila(sentencia);
        encontrado = true;
    }

    sqlite3_finalize(sentencia);
    return encontrado;
}

bool PacienteRepository::existeId(long long idPaciente) const {
    sqlite3* conexion = BaseDatosSqlite::instancia().obtenerConexion();
    if (conexion == nullptr || !BaseDatosSqlite::instancia().estaAbierta()) {
        return false;
    }

    const std::string sql = "SELECT 1 FROM pacientes WHERE id_paciente = ? LIMIT 1;";

    sqlite3_stmt* sentencia = nullptr;
    if (sqlite3_prepare_v2(conexion, sql.c_str(), -1, &sentencia, nullptr) != SQLITE_OK) {
        return false;
    }

    sqlite3_bind_int64(sentencia, 1, idPaciente);
    const int resultado = sqlite3_step(sentencia);
    sqlite3_finalize(sentencia);
    return resultado == SQLITE_ROW;
}

bool PacienteRepository::existeDocumento(const std::string& documentoIdentidad,
                                         long long exceptoIdPaciente) const {
    if (documentoIdentidad.empty()) {
        return false;
    }

    sqlite3* conexion = BaseDatosSqlite::instancia().obtenerConexion();
    if (conexion == nullptr || !BaseDatosSqlite::instancia().estaAbierta()) {
        return false;
    }

    // La columna es COLLATE NOCASE, así que la comparación ya ignora mayúsculas.
    const std::string sql =
        "SELECT 1 FROM pacientes WHERE documento_identidad = ? AND id_paciente <> ? LIMIT 1;";

    sqlite3_stmt* sentencia = nullptr;
    if (sqlite3_prepare_v2(conexion, sql.c_str(), -1, &sentencia, nullptr) != SQLITE_OK) {
        return false;
    }

    sqlite3_bind_text(sentencia, 1, documentoIdentidad.c_str(),
                      static_cast<int>(documentoIdentidad.size()), SQLITE_TRANSIENT);
    sqlite3_bind_int64(sentencia, 2, exceptoIdPaciente);

    const int resultado = sqlite3_step(sentencia);
    sqlite3_finalize(sentencia);
    return resultado == SQLITE_ROW;
}

bool PacienteRepository::crear(const Paciente& paciente, long long& idGenerado, std::string& error) const {
    sqlite3* conexion = BaseDatosSqlite::instancia().obtenerConexion();
    if (conexion == nullptr || !BaseDatosSqlite::instancia().estaAbierta()) {
        error = "La base de datos no está disponible";
        return false;
    }

    if (!ejecutar(conexion, "BEGIN IMMEDIATE;")) {
        error = sqlite3_errmsg(conexion);
        return false;
    }

    const std::string sql =
        "INSERT INTO pacientes "
        "(id_usuario, documento_identidad, nombre_completo, foto_perfil, fecha_nacimiento, "
        " tipo_sangre, rango_cargo, recompensa, estado_salud) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?);";

    sqlite3_stmt* sentencia = nullptr;
    if (sqlite3_prepare_v2(conexion, sql.c_str(), -1, &sentencia, nullptr) != SQLITE_OK) {
        error = sqlite3_errmsg(conexion);
        ejecutar(conexion, "ROLLBACK;");
        return false;
    }

    if (paciente.tieneIdUsuario()) {
        sqlite3_bind_int64(sentencia, 1, paciente.getIdUsuario());
    } else {
        sqlite3_bind_null(sentencia, 1);
    }

    const std::string documento = paciente.getDocumentoIdentidad();
    const std::string nombre = paciente.getNombreCompleto();
    const std::string foto = paciente.getFotoPerfil();
    const std::string fecha = paciente.getFechaNacimiento();
    const std::string sangre = paciente.getTipoSangre();
    const std::string rango = paciente.getRangoCargo();
    const std::string estado = paciente.getEstadoSalud();

    sqlite3_bind_text(sentencia, 2, documento.c_str(), static_cast<int>(documento.size()), SQLITE_TRANSIENT);
    sqlite3_bind_text(sentencia, 3, nombre.c_str(), static_cast<int>(nombre.size()), SQLITE_TRANSIENT);

    if (foto.empty()) {
        sqlite3_bind_null(sentencia, 4);
    } else {
        sqlite3_bind_text(sentencia, 4, foto.c_str(), static_cast<int>(foto.size()), SQLITE_TRANSIENT);
    }

    sqlite3_bind_text(sentencia, 5, fecha.c_str(), static_cast<int>(fecha.size()), SQLITE_TRANSIENT);
    sqlite3_bind_text(sentencia, 6, sangre.c_str(), static_cast<int>(sangre.size()), SQLITE_TRANSIENT);
    sqlite3_bind_text(sentencia, 7, rango.c_str(), static_cast<int>(rango.size()), SQLITE_TRANSIENT);
    sqlite3_bind_int64(sentencia, 8, paciente.getRecompensa());
    sqlite3_bind_text(sentencia, 9, estado.c_str(), static_cast<int>(estado.size()), SQLITE_TRANSIENT);

    if (sqlite3_step(sentencia) != SQLITE_DONE) {
        error = sqlite3_errmsg(conexion);
        sqlite3_finalize(sentencia);
        ejecutar(conexion, "ROLLBACK;");
        return false;
    }

    sqlite3_finalize(sentencia);
    const long long idNuevo = sqlite3_last_insert_rowid(conexion);

    // Regla de negocio del esquema: todo paciente tiene exactamente una historia
    // clínica, y crearla vacía le toca al backend (la base solo garantiza que no
    // haya más de una). Va dentro de la misma transacción para que no quede un
    // paciente sin historia si algo falla aquí.
    const std::string sqlHistoria =
        "INSERT INTO historias_clinicas (id_paciente, observaciones) VALUES (?, ?);";

    sqlite3_stmt* sentenciaHistoria = nullptr;
    if (sqlite3_prepare_v2(conexion, sqlHistoria.c_str(), -1, &sentenciaHistoria, nullptr) != SQLITE_OK) {
        error = sqlite3_errmsg(conexion);
        ejecutar(conexion, "ROLLBACK;");
        return false;
    }

    sqlite3_bind_int64(sentenciaHistoria, 1, idNuevo);

    const std::string observaciones = paciente.getObservaciones();
    if (observaciones.empty()) {
        sqlite3_bind_null(sentenciaHistoria, 2);
    } else {
        sqlite3_bind_text(sentenciaHistoria, 2, observaciones.c_str(),
                          static_cast<int>(observaciones.size()), SQLITE_TRANSIENT);
    }

    if (sqlite3_step(sentenciaHistoria) != SQLITE_DONE) {
        error = sqlite3_errmsg(conexion);
        sqlite3_finalize(sentenciaHistoria);
        ejecutar(conexion, "ROLLBACK;");
        return false;
    }

    sqlite3_finalize(sentenciaHistoria);

    if (!ejecutar(conexion, "COMMIT;")) {
        error = sqlite3_errmsg(conexion);
        ejecutar(conexion, "ROLLBACK;");
        return false;
    }

    idGenerado = idNuevo;
    error.clear();
    return true;
}

bool PacienteRepository::actualizar(const Paciente& paciente, std::string& error) const {
    sqlite3* conexion = BaseDatosSqlite::instancia().obtenerConexion();
    if (conexion == nullptr || !BaseDatosSqlite::instancia().estaAbierta()) {
        error = "La base de datos no está disponible";
        return false;
    }

    if (!ejecutar(conexion, "BEGIN IMMEDIATE;")) {
        error = sqlite3_errmsg(conexion);
        return false;
    }

    // id_usuario no se toca aquí: vincular un paciente con una cuenta es otra
    // operación, con sus propias reglas.
    const std::string sql =
        "UPDATE pacientes SET "
        "documento_identidad = ?, nombre_completo = ?, foto_perfil = ?, fecha_nacimiento = ?, "
        "tipo_sangre = ?, rango_cargo = ?, recompensa = ?, estado_salud = ? "
        "WHERE id_paciente = ?;";

    sqlite3_stmt* sentencia = nullptr;
    if (sqlite3_prepare_v2(conexion, sql.c_str(), -1, &sentencia, nullptr) != SQLITE_OK) {
        error = sqlite3_errmsg(conexion);
        ejecutar(conexion, "ROLLBACK;");
        return false;
    }

    const std::string documento = paciente.getDocumentoIdentidad();
    const std::string nombre = paciente.getNombreCompleto();
    const std::string foto = paciente.getFotoPerfil();
    const std::string fecha = paciente.getFechaNacimiento();
    const std::string sangre = paciente.getTipoSangre();
    const std::string rango = paciente.getRangoCargo();
    const std::string estado = paciente.getEstadoSalud();

    sqlite3_bind_text(sentencia, 1, documento.c_str(), static_cast<int>(documento.size()), SQLITE_TRANSIENT);
    sqlite3_bind_text(sentencia, 2, nombre.c_str(), static_cast<int>(nombre.size()), SQLITE_TRANSIENT);

    if (foto.empty()) {
        sqlite3_bind_null(sentencia, 3);
    } else {
        sqlite3_bind_text(sentencia, 3, foto.c_str(), static_cast<int>(foto.size()), SQLITE_TRANSIENT);
    }

    sqlite3_bind_text(sentencia, 4, fecha.c_str(), static_cast<int>(fecha.size()), SQLITE_TRANSIENT);
    sqlite3_bind_text(sentencia, 5, sangre.c_str(), static_cast<int>(sangre.size()), SQLITE_TRANSIENT);
    sqlite3_bind_text(sentencia, 6, rango.c_str(), static_cast<int>(rango.size()), SQLITE_TRANSIENT);
    sqlite3_bind_int64(sentencia, 7, paciente.getRecompensa());
    sqlite3_bind_text(sentencia, 8, estado.c_str(), static_cast<int>(estado.size()), SQLITE_TRANSIENT);
    sqlite3_bind_int64(sentencia, 9, paciente.getIdPaciente());

    if (sqlite3_step(sentencia) != SQLITE_DONE) {
        // Aquí cae el trigger trg_pacientes_bu_estado_salud cuando el paciente
        // ya tiene controles de evolución: su mensaje se propaga tal cual.
        error = sqlite3_errmsg(conexion);
        sqlite3_finalize(sentencia);
        ejecutar(conexion, "ROLLBACK;");
        return false;
    }

    sqlite3_finalize(sentencia);

    const std::string sqlHistoria =
        "UPDATE historias_clinicas SET observaciones = ? WHERE id_paciente = ?;";

    sqlite3_stmt* sentenciaHistoria = nullptr;
    if (sqlite3_prepare_v2(conexion, sqlHistoria.c_str(), -1, &sentenciaHistoria, nullptr) != SQLITE_OK) {
        error = sqlite3_errmsg(conexion);
        ejecutar(conexion, "ROLLBACK;");
        return false;
    }

    const std::string observaciones = paciente.getObservaciones();
    if (observaciones.empty()) {
        sqlite3_bind_null(sentenciaHistoria, 1);
    } else {
        sqlite3_bind_text(sentenciaHistoria, 1, observaciones.c_str(),
                          static_cast<int>(observaciones.size()), SQLITE_TRANSIENT);
    }

    sqlite3_bind_int64(sentenciaHistoria, 2, paciente.getIdPaciente());

    if (sqlite3_step(sentenciaHistoria) != SQLITE_DONE) {
        error = sqlite3_errmsg(conexion);
        sqlite3_finalize(sentenciaHistoria);
        ejecutar(conexion, "ROLLBACK;");
        return false;
    }

    sqlite3_finalize(sentenciaHistoria);

    if (!ejecutar(conexion, "COMMIT;")) {
        error = sqlite3_errmsg(conexion);
        ejecutar(conexion, "ROLLBACK;");
        return false;
    }

    error.clear();
    return true;
}
