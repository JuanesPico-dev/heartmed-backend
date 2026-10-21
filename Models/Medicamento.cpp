#include "Models/Medicamento.h"

#include <string>
#include <vector>

Medicamento::Medicamento()
    : idMedicamento_(0),
      nombre_(""),
      tipoInsumo_(""),
      unidadMedida_(""),
      descripcion_(""),
      cantidadTotalDisponible_(0),
      umbralMinimo_(0) {
}

void Medicamento::setIdMedicamento(long long idMedicamento) {
    idMedicamento_ = idMedicamento;
}

long long Medicamento::getIdMedicamento() const {
    return idMedicamento_;
}

void Medicamento::setNombre(const std::string& nombre) {
    nombre_ = nombre;
}

std::string Medicamento::getNombre() const {
    return nombre_;
}

void Medicamento::setTipoInsumo(const std::string& tipoInsumo) {
    tipoInsumo_ = tipoInsumo;
}

std::string Medicamento::getTipoInsumo() const {
    return tipoInsumo_;
}

void Medicamento::setUnidadMedida(const std::string& unidadMedida) {
    unidadMedida_ = unidadMedida;
}

std::string Medicamento::getUnidadMedida() const {
    return unidadMedida_;
}

void Medicamento::setDescripcion(const std::string& descripcion) {
    descripcion_ = descripcion;
}

std::string Medicamento::getDescripcion() const {
    return descripcion_;
}

void Medicamento::setCantidadTotalDisponible(long long cantidadTotalDisponible) {
    cantidadTotalDisponible_ = cantidadTotalDisponible;
}

long long Medicamento::getCantidadTotalDisponible() const {
    return cantidadTotalDisponible_;
}

void Medicamento::setUmbralMinimo(long long umbralMinimo) {
    umbralMinimo_ = umbralMinimo;
}

long long Medicamento::getUmbralMinimo() const {
    return umbralMinimo_;
}

void Medicamento::agregarLote(const Lote& lote) {
    lotes_.push_back(lote);
}

const std::vector<Lote>& Medicamento::getLotes() const {
    return lotes_;
}

bool Medicamento::estaBajoMinimo() const {
    return cantidadTotalDisponible_ < umbralMinimo_;
}
