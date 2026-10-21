#pragma once

#include <string>

// Representa una fila de la tabla lotes: una remesa concreta de un medicamento
// o insumo, con su ubicación y su fecha de vencimiento.
// La cantidad es de solo lectura: la calcula la base a partir de los
// movimientos de inventario, y un trigger rechaza escribirla a mano.
class Lote {
public:
    Lote();

    void setIdLote(long long idLote);
    long long getIdLote() const;

    void setIdMedicamento(long long idMedicamento);
    long long getIdMedicamento() const;

    void setNumLote(const std::string& numLote);
    std::string getNumLote() const;

    void setUbicacion(const std::string& ubicacion);
    std::string getUbicacion() const;

    void setFechaVencimiento(const std::string& fechaVencimiento);
    std::string getFechaVencimiento() const;

    void setCantidad(long long cantidad);
    long long getCantidad() const;

private:
    long long idLote_;
    long long idMedicamento_;
    std::string numLote_;
    std::string ubicacion_;
    std::string fechaVencimiento_;
    long long cantidad_;
};
