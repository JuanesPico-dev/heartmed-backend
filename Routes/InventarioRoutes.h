#pragma once

#include "crow.h"
#include "crow/middlewares/cors.h"
#include "Services/InventarioService.h"

class InventarioRoutes {
public:
    explicit InventarioRoutes(InventarioService& inventarioService);
    void registrarRutas(crow::App<crow::CORSHandler>& app);

private:
    InventarioService& inventarioService_;
};
