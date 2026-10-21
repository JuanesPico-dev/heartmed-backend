#pragma once

#include <string>
#include <vector>

#include "Models/Medicamento.h"

class InventarioRepository {
public:
    // Devuelve los medicamentos e insumos con sus lotes ya asociados.
    bool listar(std::vector<Medicamento>& medicamentos, std::string& error) const;
};
