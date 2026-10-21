#include "Database/SQLite.h"

#include <string>

BaseDatosSqlite::BaseDatosSqlite() : conexion_(nullptr) {
}

BaseDatosSqlite::~BaseDatosSqlite() {
    cerrar();
}

BaseDatosSqlite& BaseDatosSqlite::instancia() {
    static BaseDatosSqlite baseDatos;
    return baseDatos;
}

bool BaseDatosSqlite::abrir(const std::string& ruta) {
    if (conexion_ != nullptr) {
        rutaBaseDatos_ = ruta;
        return true;
    }

    const int resultado = sqlite3_open(ruta.c_str(), &conexion_);
    if (resultado != SQLITE_OK) {
        ultimoError_ = sqlite3_errmsg(conexion_);
        if (conexion_ != nullptr) {
            sqlite3_close(conexion_);
            conexion_ = nullptr;
        }
        return false;
    }

    // SQLite trae las claves foráneas desactivadas por defecto y hay que pedirlo
    // en cada conexión. El esquema las usa (p. ej. historias_clinicas apunta a
    // pacientes), así que sin esto las relaciones no se verifican al escribir.
    sqlite3_exec(conexion_, "PRAGMA foreign_keys = ON;", nullptr, nullptr, nullptr);

    rutaBaseDatos_ = ruta;
    ultimoError_.clear();
    return true;
}

void BaseDatosSqlite::cerrar() {
    if (conexion_ != nullptr) {
        sqlite3_close(conexion_);
        conexion_ = nullptr;
    }

    rutaBaseDatos_.clear();
    ultimoError_.clear();
}

sqlite3* BaseDatosSqlite::obtenerConexion() const {
    return conexion_;
}

bool BaseDatosSqlite::estaAbierta() const {
    return conexion_ != nullptr;
}

std::string BaseDatosSqlite::obtenerUltimoError() const {
    return ultimoError_;
}
