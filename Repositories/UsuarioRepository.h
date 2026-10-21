#pragma once

#include <string>

class UsuarioRepository {
public:
    bool existeUsuario(const std::string& email) const;
    bool validarCredenciales(const std::string& email, const std::string& password) const;
    bool buscarPorCorreo(const std::string& email, std::string& nombre) const;
    // Devuelve 0 si no hay un usuario activo con ese correo.
    long long obtenerIdPorCorreo(const std::string& email) const;
    bool buscarPorCorreoYPassword(const std::string& email, const std::string& password) const;
    std::string obtenerHashContrasena(const std::string& email) const;
    bool usuarioActivo(const std::string& email) const;
    std::string ObtenerRolUsuario(const std::string& email) const;
};
