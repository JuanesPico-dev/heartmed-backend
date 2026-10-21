#pragma once

#include <string>
#include <vector>

#include "Models/Lote.h"

// Representa una fila de la tabla medicamentos, que guarda tanto medicamentos
// como insumos (lo distingue tipo_insumo), junto con sus lotes.
// cantidadTotalDisponible es un dato derivado: lo mantienen los triggers a
// partir de los lotes, así que aquí solo se lee.
class Medicamento {
public:
    Medicamento();

    void setIdMedicamento(long long idMedicamento);
    long long getIdMedicamento() const;

    void setNombre(const std::string& nombre);
    std::string getNombre() const;

    void setTipoInsumo(const std::string& tipoInsumo);
    std::string getTipoInsumo() const;

    void setUnidadMedida(const std::string& unidadMedida);
    std::string getUnidadMedida() const;

    void setDescripcion(const std::string& descripcion);
    std::string getDescripcion() const;

    void setCantidadTotalDisponible(long long cantidadTotalDisponible);
    long long getCantidadTotalDisponible() const;

    void setUmbralMinimo(long long umbralMinimo);
    long long getUmbralMinimo() const;

    void agregarLote(const Lote& lote);
    const std::vector<Lote>& getLotes() const;

    // Regla de negocio: hay que reponer cuando el stock queda por debajo del
    // umbral configurado para ese insumo.
    bool estaBajoMinimo() const;

private:
    long long idMedicamento_;
    std::string nombre_;
    std::string tipoInsumo_;
    std::string unidadMedida_;
    std::string descripcion_;
    long long cantidadTotalDisponible_;
    long long umbralMinimo_;
    std::vector<Lote> lotes_;
};
