#pragma once

#include <string>

// Representa una fila de movimientos_inventario: una entrada, un consumo o una
// baja sobre un lote concreto.
//
// Los movimientos son el libro mayor del inventario: la base recalcula sola
// lotes.cantidad y medicamentos.cantidad_total_disponible a partir de ellos, y
// unos triggers impiden editarlos o borrarlos. Para corregir un error se
// registra el movimiento contrario, nunca se toca el original.
class MovimientoInventario {
public:
    MovimientoInventario();

    void setIdMovimiento(long long idMovimiento);
    long long getIdMovimiento() const;

    void setIdLote(long long idLote);
    long long getIdLote() const;

    void setIdUsuario(long long idUsuario);
    long long getIdUsuario() const;

    // El tratamiento es opcional y solo vale para los consumos.
    void setIdTratamiento(long long idTratamiento);
    void quitarIdTratamiento();
    long long getIdTratamiento() const;
    bool tieneIdTratamiento() const;

    void setTipoMovimiento(const std::string& tipoMovimiento);
    std::string getTipoMovimiento() const;

    void setCantidad(long long cantidad);
    long long getCantidad() const;

    void setFechaMovimiento(const std::string& fechaMovimiento);
    std::string getFechaMovimiento() const;

    void setMotivo(const std::string& motivo);
    std::string getMotivo() const;

private:
    long long idMovimiento_;
    long long idLote_;
    long long idUsuario_;
    long long idTratamiento_;
    bool tieneIdTratamiento_;
    std::string tipoMovimiento_;
    long long cantidad_;
    std::string fechaMovimiento_;
    std::string motivo_;
};
