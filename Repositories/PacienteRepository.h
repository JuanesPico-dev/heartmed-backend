#pragma once

#include <string>
#include <vector>

#include "Models/Paciente.h"

class PacienteRepository {
public:
    bool listar(std::vector<Paciente>& pacientes, std::string& error) const;
    bool buscarPorId(long long idPaciente, Paciente& paciente) const;
    bool existeDocumento(const std::string& documentoIdentidad) const;
    // Inserta el paciente y su historia clínica vacía en una sola transacción.
    bool crear(const Paciente& paciente, long long& idGenerado, std::string& error) const;
};
