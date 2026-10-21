#pragma once

#include <string>
#include <vector>

#include "Models/Paciente.h"

// Permite que la capa de rutas elija el código HTTP sin interpretar el mensaje.
enum class ResultadoPaciente {
    Exito,
    DatosInvalidos,
    Duplicado,
    ErrorInterno,
};

class PacienteService {
public:
    ResultadoPaciente listar(std::vector<Paciente>& pacientes, std::string& mensaje) const;
    ResultadoPaciente crear(const Paciente& datos, Paciente& creado, std::string& mensaje) const;
};
