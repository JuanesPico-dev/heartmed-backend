#include "Models/MovimientoInventario.h"

#include <string>

MovimientoInventario::MovimientoInventario()
    : idMovimiento_(0),
      idLote_(0),
      idUsuario_(0),
      idTratamiento_(0),
      tieneIdTratamiento_(false),
      tipoMovimiento_(""),
      cantidad_(0),
      fechaMovimiento_(""),
      motivo_("") {
}

void MovimientoInventario::setIdMovimiento(long long idMovimiento) {
    idMovimiento_ = idMovimiento;
}

long long MovimientoInventario::getIdMovimiento() const {
    return idMovimiento_;
}

void MovimientoInventario::setIdLote(long long idLote) {
    idLote_ = idLote;
}

long long MovimientoInventario::getIdLote() const {
    return idLote_;
}

void MovimientoInventario::setIdUsuario(long long idUsuario) {
    idUsuario_ = idUsuario;
}

long long MovimientoInventario::getIdUsuario() const {
    return idUsuario_;
}

void MovimientoInventario::setIdTratamiento(long long idTratamiento) {
    idTratamiento_ = idTratamiento;
    tieneIdTratamiento_ = true;
}

void MovimientoInventario::quitarIdTratamiento() {
    idTratamiento_ = 0;
    tieneIdTratamiento_ = false;
}

long long MovimientoInventario::getIdTratamiento() const {
    return idTratamiento_;
}

bool MovimientoInventario::tieneIdTratamiento() const {
    return tieneIdTratamiento_;
}

void MovimientoInventario::setTipoMovimiento(const std::string& tipoMovimiento) {
    tipoMovimiento_ = tipoMovimiento;
}

std::string MovimientoInventario::getTipoMovimiento() const {
    return tipoMovimiento_;
}

void MovimientoInventario::setCantidad(long long cantidad) {
    cantidad_ = cantidad;
}

long long MovimientoInventario::getCantidad() const {
    return cantidad_;
}

void MovimientoInventario::setFechaMovimiento(const std::string& fechaMovimiento) {
    fechaMovimiento_ = fechaMovimiento;
}

std::string MovimientoInventario::getFechaMovimiento() const {
    return fechaMovimiento_;
}

void MovimientoInventario::setMotivo(const std::string& motivo) {
    motivo_ = motivo;
}

std::string MovimientoInventario::getMotivo() const {
    return motivo_;
}
