#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "Cross/ResultadoOperacion.h"
#include "Models/Medicamento.h"

// Cifras agregadas del inventario, para que el dashboard no tenga que
// calcularlas a partir del listado.
struct ResumenInventario {
    ResumenInventario();

    std::size_t total;              // medicamentos e insumos registrados
    std::size_t bajoMinimo;         // los que están por debajo de su umbral
    int porcentajeDisponibilidad;   // % de insumos con stock suficiente
    bool hayDatos;                  // false si no hay nada registrado
};

class InventarioService {
public:
    ResultadoOperacion listar(std::vector<Medicamento>& medicamentos,
                              ResumenInventario& resumen,
                              std::string& mensaje) const;
};
