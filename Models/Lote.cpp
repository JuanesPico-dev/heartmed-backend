#include "Models/Lote.h"

#include <string>

Lote::Lote()
    : idLote_(0),
      idMedicamento_(0),
      numLote_(""),
      ubicacion_(""),
      fechaVencimiento_(""),
      cantidad_(0) {
}

void Lote::setIdLote(long long idLote) {
    idLote_ = idLote;
}

long long Lote::getIdLote() const {
    return idLote_;
}

void Lote::setIdMedicamento(long long idMedicamento) {
    idMedicamento_ = idMedicamento;
}

long long Lote::getIdMedicamento() const {
    return idMedicamento_;
}

void Lote::setNumLote(const std::string& numLote) {
    numLote_ = numLote;
}

std::string Lote::getNumLote() const {
    return numLote_;
}

void Lote::setUbicacion(const std::string& ubicacion) {
    ubicacion_ = ubicacion;
}

std::string Lote::getUbicacion() const {
    return ubicacion_;
}

void Lote::setFechaVencimiento(const std::string& fechaVencimiento) {
    fechaVencimiento_ = fechaVencimiento;
}

std::string Lote::getFechaVencimiento() const {
    return fechaVencimiento_;
}

void Lote::setCantidad(long long cantidad) {
    cantidad_ = cantidad;
}

long long Lote::getCantidad() const {
    return cantidad_;
}
