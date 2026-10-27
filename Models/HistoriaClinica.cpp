#include "Models/HistoriaClinica.h"

#include <string>
#include <vector>

HistoriaClinica::HistoriaClinica()
    : idHistoria_(0),
      idPaciente_(0),
      nombrePaciente_(""),
      alergiasRevisadas_(false),
      antecedentes_(""),
      cirugiasPrevias_(""),
      observaciones_(""),
      frutaDiablo_(false),
      fechaCreacion_("") {
}

void HistoriaClinica::setIdHistoria(long long idHistoria) {
    idHistoria_ = idHistoria;
}

long long HistoriaClinica::getIdHistoria() const {
    return idHistoria_;
}

void HistoriaClinica::setIdPaciente(long long idPaciente) {
    idPaciente_ = idPaciente;
}

long long HistoriaClinica::getIdPaciente() const {
    return idPaciente_;
}

void HistoriaClinica::setNombrePaciente(const std::string& nombrePaciente) {
    nombrePaciente_ = nombrePaciente;
}

std::string HistoriaClinica::getNombrePaciente() const {
    return nombrePaciente_;
}

void HistoriaClinica::setAlergiasRevisadas(bool alergiasRevisadas) {
    alergiasRevisadas_ = alergiasRevisadas;
}

bool HistoriaClinica::getAlergiasRevisadas() const {
    return alergiasRevisadas_;
}

void HistoriaClinica::setAntecedentes(const std::string& antecedentes) {
    antecedentes_ = antecedentes;
}

std::string HistoriaClinica::getAntecedentes() const {
    return antecedentes_;
}

void HistoriaClinica::setCirugiasPrevias(const std::string& cirugiasPrevias) {
    cirugiasPrevias_ = cirugiasPrevias;
}

std::string HistoriaClinica::getCirugiasPrevias() const {
    return cirugiasPrevias_;
}

void HistoriaClinica::setObservaciones(const std::string& observaciones) {
    observaciones_ = observaciones;
}

std::string HistoriaClinica::getObservaciones() const {
    return observaciones_;
}

void HistoriaClinica::setFrutaDiablo(bool frutaDiablo) {
    frutaDiablo_ = frutaDiablo;
}

bool HistoriaClinica::getFrutaDiablo() const {
    return frutaDiablo_;
}

void HistoriaClinica::setFechaCreacion(const std::string& fechaCreacion) {
    fechaCreacion_ = fechaCreacion;
}

std::string HistoriaClinica::getFechaCreacion() const {
    return fechaCreacion_;
}

void HistoriaClinica::agregarAlergia(const Alergia& alergia) {
    alergias_.push_back(alergia);
}

const std::vector<Alergia>& HistoriaClinica::getAlergias() const {
    return alergias_;
}
