#pragma once

#include <string>

#include "Cross/ResultadoOperacion.h"
#include "Models/MovimientoInventario.h"
#include "Repositories/MovimientoRepository.h"

class MovimientoService {
public:
    // `correoUsuario` sale del token, no del cuerpo de la petición: quién
    // movió el inventario es un dato de auditoría y no lo decide el cliente.
    ResultadoOperacion registrar(const MovimientoInventario& datos,
                                 const std::string& correoUsuario,
                                 MovimientoInventario& registrado,
                                 StockResultante& stock,
                                 std::string& mensaje) const;
};
