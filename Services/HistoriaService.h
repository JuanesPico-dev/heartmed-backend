#pragma once

#include <string>

#include "Cross/ResultadoOperacion.h"
#include "Models/HistoriaClinica.h"
#include "Repositories/HistoriaRepository.h"

class HistoriaService {
public:
    ResultadoOperacion consultar(long long idPaciente,
                                 HistoriaClinica& historia,
                                 std::string& mensaje) const;

    ResultadoOperacion actualizar(long long idPaciente,
                                  const CambiosHistoria& cambios,
                                  HistoriaClinica& actualizada,
                                  std::string& mensaje) const;
};
