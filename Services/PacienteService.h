#pragma once

#include <string>
#include <vector>

#include "Cross/ResultadoOperacion.h"
#include "Models/Paciente.h"

class PacienteService {
public:
    ResultadoOperacion listar(std::vector<Paciente>& pacientes, std::string& mensaje) const;
    ResultadoOperacion crear(const Paciente& datos, Paciente& creado, std::string& mensaje) const;
    ResultadoOperacion actualizar(long long idPaciente,
                                  const Paciente& datos,
                                  Paciente& actualizado,
                                  std::string& mensaje) const;
};
