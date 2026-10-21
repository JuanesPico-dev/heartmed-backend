#include "Repositories/UsuarioRepository.h"

#include <string>

#include "Cross/PasswordHash.h"
#include "Database/SQLite.h"

namespace {

std::string limpiarTexto(const std::string& valor) {
    std::string resultado = valor;
    const auto inicio = resultado.find_first_not_of(" \t\r\n");
    if (inicio == std::string::npos) {
        return "";
    }

    const auto fin = resultado.find_last_not_of(" \t\r\n");
    resultado = resultado.substr(inicio, fin - inicio + 1);
    return resultado;
}

}  // namespace

bool UsuarioRepository::buscarPorCorreo(const std::string& email, std::string& nombre) const {
    const std::string correo = limpiarTexto(email);
    if (correo.empty()) {
        return false;
    }

    sqlite3* conexion = BaseDatosSqlite::instancia().obtenerConexion();
    if (conexion == nullptr || !BaseDatosSqlite::instancia().estaAbierta()) {
        return false;
    }

    const std::string sql =
        "SELECT nombre FROM usuarios WHERE LOWER(correo) = LOWER(?) AND activo = 1 LIMIT 1;";

    sqlite3_stmt* sentencia = nullptr;
    if (sqlite3_prepare_v2(conexion, sql.c_str(), -1, &sentencia, nullptr) != SQLITE_OK) {
        return false;
    }

    sqlite3_bind_text(sentencia, 1, correo.c_str(), static_cast<int>(correo.size()), SQLITE_TRANSIENT);

    const int resultado = sqlite3_step(sentencia);
    if (resultado == SQLITE_ROW) {
        const unsigned char* valor = sqlite3_column_text(sentencia, 0);
        if (valor != nullptr) {
            nombre = reinterpret_cast<const char*>(valor);
        }
        sqlite3_finalize(sentencia);
        return true;
    }

    sqlite3_finalize(sentencia);
    return false;
}

std::string UsuarioRepository::obtenerHashContrasena(const std::string& email) const {
    const std::string correo = limpiarTexto(email);
    if (correo.empty()) {
        return "";
    }

    sqlite3* conexion = BaseDatosSqlite::instancia().obtenerConexion();
    if (conexion == nullptr || !BaseDatosSqlite::instancia().estaAbierta()) {
        return "";
    }

    const std::string sql =
        "SELECT contrasena_hash FROM usuarios WHERE LOWER(correo) = LOWER(?) AND activo = 1 LIMIT 1;";

    sqlite3_stmt* sentencia = nullptr;
    if (sqlite3_prepare_v2(conexion, sql.c_str(), -1, &sentencia, nullptr) != SQLITE_OK) {
        return "";
    }

    sqlite3_bind_text(sentencia, 1, correo.c_str(), static_cast<int>(correo.size()), SQLITE_TRANSIENT);

    std::string hashAlmacenado;
    if (sqlite3_step(sentencia) == SQLITE_ROW) {
        const unsigned char* valor = sqlite3_column_text(sentencia, 0);
        if (valor != nullptr) {
            hashAlmacenado = reinterpret_cast<const char*>(valor);
        }
    }

    sqlite3_finalize(sentencia);
    return hashAlmacenado;
}

bool UsuarioRepository::usuarioActivo(const std::string& email) const {
    const std::string correo = limpiarTexto(email);
    if (correo.empty()) {
        return false;
    }

    sqlite3* conexion = BaseDatosSqlite::instancia().obtenerConexion();
    if (conexion == nullptr || !BaseDatosSqlite::instancia().estaAbierta()) {
        return false;
    }

    const std::string sql =
        "SELECT 1 FROM usuarios WHERE LOWER(correo) = LOWER(?) AND activo = 1 LIMIT 1;";

    sqlite3_stmt* sentencia = nullptr;
    if (sqlite3_prepare_v2(conexion, sql.c_str(), -1, &sentencia, nullptr) != SQLITE_OK) {
        return false;
    }

    sqlite3_bind_text(sentencia, 1, correo.c_str(), static_cast<int>(correo.size()), SQLITE_TRANSIENT);
    const int resultado = sqlite3_step(sentencia);
    sqlite3_finalize(sentencia);
    return resultado == SQLITE_ROW;
}

bool UsuarioRepository::existeUsuario(const std::string& email) const {
    const std::string correo = limpiarTexto(email);
    if (correo.empty()) {
        return false;
    }

    sqlite3* conexion = BaseDatosSqlite::instancia().obtenerConexion();
    if (conexion == nullptr || !BaseDatosSqlite::instancia().estaAbierta()) {
        return false;
    }

    const std::string sql =
        "SELECT 1 FROM usuarios WHERE LOWER(correo) = LOWER(?) AND activo = 1 LIMIT 1;";

    sqlite3_stmt* sentencia = nullptr;
    if (sqlite3_prepare_v2(conexion, sql.c_str(), -1, &sentencia, nullptr) != SQLITE_OK) {
        return false;
    }

    sqlite3_bind_text(sentencia, 1, correo.c_str(), static_cast<int>(correo.size()), SQLITE_TRANSIENT);
    const int resultado = sqlite3_step(sentencia);
    sqlite3_finalize(sentencia);
    return resultado == SQLITE_ROW;
}

bool UsuarioRepository::validarCredenciales(const std::string& email, const std::string& password) const {
    const std::string correo = limpiarTexto(email);
    const std::string clave = limpiarTexto(password);
    if (correo.empty() || clave.empty()) {
        return false;
    }

    const std::string hashAlmacenado = obtenerHashContrasena(correo);
    if (hashAlmacenado.empty()) {
        return false;
    }

    return HashContrasena::verificar(clave, hashAlmacenado);
}

bool UsuarioRepository::buscarPorCorreoYPassword(const std::string& email, const std::string& password) const {
    return existeUsuario(email) && validarCredenciales(email, password);
}

long long UsuarioRepository::obtenerIdPorCorreo(const std::string& email) const {
    const std::string correo = limpiarTexto(email);
    if (correo.empty()) {
        return 0;
    }

    sqlite3* conexion = BaseDatosSqlite::instancia().obtenerConexion();
    if (conexion == nullptr || !BaseDatosSqlite::instancia().estaAbierta()) {
        return 0;
    }

    const std::string sql =
        "SELECT id_usuario FROM usuarios WHERE LOWER(correo) = LOWER(?) AND activo = 1 LIMIT 1;";

    sqlite3_stmt* sentencia = nullptr;
    if (sqlite3_prepare_v2(conexion, sql.c_str(), -1, &sentencia, nullptr) != SQLITE_OK) {
        return 0;
    }

    sqlite3_bind_text(sentencia, 1, correo.c_str(), static_cast<int>(correo.size()), SQLITE_TRANSIENT);

    long long idUsuario = 0;
    if (sqlite3_step(sentencia) == SQLITE_ROW) {
        idUsuario = sqlite3_column_int64(sentencia, 0);
    }

    sqlite3_finalize(sentencia);
    return idUsuario;
}
