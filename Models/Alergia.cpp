#include "Models/Alergia.h"

#include <string>

Alergia::Alergia()
    : idAlergia_(0),
      sustancia_(""),
      idMedicamento_(0),
      tieneIdMedicamento_(false),
      nombreMedicamento_("") {
}

void Alergia::setIdAlergia(long long idAlergia) {
    idAlergia_ = idAlergia;
}

long long Alergia::getIdAlergia() const {
    return idAlergia_;
}

void Alergia::setSustancia(const std::string& sustancia) {
    sustancia_ = sustancia;
}

std::string Alergia::getSustancia() const {
    return sustancia_;
}

void Alergia::setIdMedicamento(long long idMedicamento) {
    idMedicamento_ = idMedicamento;
    tieneIdMedicamento_ = true;
}

void Alergia::quitarIdMedicamento() {
    idMedicamento_ = 0;
    tieneIdMedicamento_ = false;
}

long long Alergia::getIdMedicamento() const {
    return idMedicamento_;
}

bool Alergia::tieneIdMedicamento() const {
    return tieneIdMedicamento_;
}

void Alergia::setNombreMedicamento(const std::string& nombreMedicamento) {
    nombreMedicamento_ = nombreMedicamento;
}

std::string Alergia::getNombreMedicamento() const {
    return nombreMedicamento_;
}
