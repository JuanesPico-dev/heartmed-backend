#pragma once

#include "crow.h"
#include "crow/middlewares/cors.h"
#include "Services/HistoriaService.h"

// Rutas de la historia clínica, colgadas del paciente porque es 1 a 1 con él:
// /api/pacientes/<id>/historia
class HistoriaRoutes {
public:
    explicit HistoriaRoutes(HistoriaService& historiaService);
    void registrarRutas(crow::App<crow::CORSHandler>& app);

private:
    HistoriaService& historiaService_;
};
