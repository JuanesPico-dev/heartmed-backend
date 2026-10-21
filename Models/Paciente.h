#pragma once

#include <string>

// Representa una fila de la tabla pacientes.
// Los pacientes sin cuenta de acceso tienen id_usuario NULL en la base, por eso
// el identificador de usuario se acompaña de una bandera en lugar de un cero.
class Paciente {
public:
    Paciente();

    void setIdPaciente(long long idPaciente);
    long long getIdPaciente() const;

    void setIdUsuario(long long idUsuario);
    void quitarIdUsuario();
    long long getIdUsuario() const;
    bool tieneIdUsuario() const;

    void setDocumentoIdentidad(const std::string& documentoIdentidad);
    std::string getDocumentoIdentidad() const;

    void setNombreCompleto(const std::string& nombreCompleto);
    std::string getNombreCompleto() const;

    void setFotoPerfil(const std::string& fotoPerfil);
    std::string getFotoPerfil() const;

    void setFechaNacimiento(const std::string& fechaNacimiento);
    std::string getFechaNacimiento() const;

    void setTipoSangre(const std::string& tipoSangre);
    std::string getTipoSangre() const;

    void setRangoCargo(const std::string& rangoCargo);
    std::string getRangoCargo() const;

    void setRecompensa(long long recompensa);
    long long getRecompensa() const;

    void setEstadoSalud(const std::string& estadoSalud);
    std::string getEstadoSalud() const;

    // No es una columna de pacientes: son las observaciones de su historia
    // clínica, que es 1 a 1 con el paciente. Viaja aquí para que la API pueda
    // exponer y recibir el paciente como un solo objeto.
    void setObservaciones(const std::string& observaciones);
    std::string getObservaciones() const;

private:
    long long idPaciente_;
    long long idUsuario_;
    bool tieneIdUsuario_;
    std::string documentoIdentidad_;
    std::string nombreCompleto_;
    std::string fotoPerfil_;
    std::string fechaNacimiento_;
    std::string tipoSangre_;
    std::string rangoCargo_;
    long long recompensa_;
    std::string estadoSalud_;
    std::string observaciones_;
};
