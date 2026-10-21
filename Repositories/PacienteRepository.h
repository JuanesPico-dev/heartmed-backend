#pragma once

#include <string>
#include <vector>

#include "Models/Paciente.h"

class PacienteRepository {
public:
    bool listar(std::vector<Paciente>& pacientes, std::string& error) const;
    bool buscarPorId(long long idPaciente, Paciente& paciente) const;
    bool existeId(long long idPaciente) const;
    // El segundo parámetro excluye al propio paciente, para poder guardar una
    // edición que no cambia el documento.
    bool existeDocumento(const std::string& documentoIdentidad, long long exceptoIdPaciente = 0) const;
    // Inserta el paciente y su historia clínica vacía en una sola transacción.
    bool crear(const Paciente& paciente, long long& idGenerado, std::string& error) const;
    // Actualiza el paciente y las observaciones de su historia clínica.
    bool actualizar(const Paciente& paciente, std::string& error) const;
};
