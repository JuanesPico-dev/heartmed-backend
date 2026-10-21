#pragma once

#include "crow.h"
#include "crow/middlewares/cors.h"
#include "Services/MovimientoService.h"

class MovimientoRoutes {
public:
    explicit MovimientoRoutes(MovimientoService& movimientoService);
    void registrarRutas(crow::App<crow::CORSHandler>& app);

private:
    MovimientoService& movimientoService_;
};
