#pragma once

#include "crow.h"
#include "crow/middlewares/cors.h"
#include "Services/PacienteService.h"

class PacienteRoutes {
public:
    explicit PacienteRoutes(PacienteService& pacienteService);
    void registrarRutas(crow::App<crow::CORSHandler>& app);

private:
    PacienteService& pacienteService_;
};
