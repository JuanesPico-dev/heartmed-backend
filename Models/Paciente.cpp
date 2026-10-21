#include "Models/Paciente.h"

#include <string>

Paciente::Paciente()
    : idPaciente_(0),
      idUsuario_(0),
      tieneIdUsuario_(false),
      documentoIdentidad_(""),
      nombreCompleto_(""),
      fotoPerfil_(""),
      fechaNacimiento_(""),
      tipoSangre_(""),
      rangoCargo_(""),
      recompensa_(0),
      estadoSalud_("") {
}

void Paciente::setIdPaciente(long long idPaciente) {
    idPaciente_ = idPaciente;
}

long long Paciente::getIdPaciente() const {
    return idPaciente_;
}

void Paciente::setIdUsuario(long long idUsuario) {
    idUsuario_ = idUsuario;
    tieneIdUsuario_ = true;
}

void Paciente::quitarIdUsuario() {
    idUsuario_ = 0;
    tieneIdUsuario_ = false;
}

long long Paciente::getIdUsuario() const {
    return idUsuario_;
}

bool Paciente::tieneIdUsuario() const {
    return tieneIdUsuario_;
}

void Paciente::setDocumentoIdentidad(const std::string& documentoIdentidad) {
    documentoIdentidad_ = documentoIdentidad;
}

std::string Paciente::getDocumentoIdentidad() const {
    return documentoIdentidad_;
}

void Paciente::setNombreCompleto(const std::string& nombreCompleto) {
    nombreCompleto_ = nombreCompleto;
}

std::string Paciente::getNombreCompleto() const {
    return nombreCompleto_;
}

void Paciente::setFotoPerfil(const std::string& fotoPerfil) {
    fotoPerfil_ = fotoPerfil;
}

std::string Paciente::getFotoPerfil() const {
    return fotoPerfil_;
}

void Paciente::setFechaNacimiento(const std::string& fechaNacimiento) {
    fechaNacimiento_ = fechaNacimiento;
}

std::string Paciente::getFechaNacimiento() const {
    return fechaNacimiento_;
}

void Paciente::setTipoSangre(const std::string& tipoSangre) {
    tipoSangre_ = tipoSangre;
}

std::string Paciente::getTipoSangre() const {
    return tipoSangre_;
}

void Paciente::setRangoCargo(const std::string& rangoCargo) {
    rangoCargo_ = rangoCargo;
}

std::string Paciente::getRangoCargo() const {
    return rangoCargo_;
}

void Paciente::setRecompensa(long long recompensa) {
    recompensa_ = recompensa;
}

long long Paciente::getRecompensa() const {
    return recompensa_;
}

void Paciente::setEstadoSalud(const std::string& estadoSalud) {
    estadoSalud_ = estadoSalud;
}

std::string Paciente::getEstadoSalud() const {
    return estadoSalud_;
}
