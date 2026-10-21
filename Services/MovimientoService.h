#pragma once

#include <string>
#include <vector>

#include "Cross/ResultadoOperacion.h"
#include "Models/MovimientoInventario.h"
#include "Repositories/MovimientoRepository.h"

class MovimientoService {
public:
    // `limite` acota cuántos movimientos devuelve el historial; 0 usa el valor
    // por defecto.
    ResultadoOperacion listar(long long limite,
                              std::vector<MovimientoDetallado>& movimientos,
                              std::string& mensaje) const;

    // `correoUsuario` sale del token, no del cuerpo de la petición: quién
    // movió el inventario es un dato de auditoría y no lo decide el cliente.
    ResultadoOperacion registrar(const MovimientoInventario& datos,
                                 const std::string& correoUsuario,
                                 MovimientoInventario& registrado,
                                 StockResultante& stock,
                                 std::string& mensaje) const;
};
