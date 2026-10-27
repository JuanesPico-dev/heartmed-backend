#pragma once

#include <optional>
#include <string>
#include <vector>

#include "Models/Alergia.h"
#include "Models/HistoriaClinica.h"

// Lo que trae un PUT de historia clínica. Cada campo es opcional a propósito:
// vacío significa "no venía en la petición, no lo toques". Así un formulario
// que solo rellena una parte no borra en silencio lo que no envía, que en
// datos clínicos sería grave.
struct CambiosHistoria {
    std::optional<std::string> antecedentes;
    std::optional<std::string> cirugiasPrevias;
    std::optional<std::string> observaciones;
    std::optional<bool> frutaDiablo;
    std::optional<bool> alergiasRevisadas;
    std::optional<std::vector<Alergia>> alergias;
};

class HistoriaRepository {
public:
    bool existePaciente(long long idPaciente) const;
    bool existeMedicamento(long long idMedicamento) const;
    // false si el paciente no tiene historia.
    bool buscarPorPaciente(long long idPaciente, HistoriaClinica& historia, std::string& error) const;
    // Aplica solo los campos presentes. Si vienen alergias, reemplaza la lista
    // entera por la recibida.
    bool actualizar(long long idHistoria, const CambiosHistoria& cambios, std::string& error) const;
};
