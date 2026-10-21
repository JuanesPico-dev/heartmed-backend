#pragma once

#include <string>
#include <vector>

#include "Models/MovimientoInventario.h"

// Stock resultante después de registrar un movimiento. Lo calcula la base con
// sus triggers, así que se vuelve a leer en lugar de deducirlo aquí.
struct StockResultante {
    StockResultante();

    long long idLote;
    std::string numLote;
    long long cantidadLote;
    long long idMedicamento;
    std::string nombreMedicamento;
    long long cantidadTotalDisponible;
    long long umbralMinimo;
};

// Un movimiento con los nombres ya resueltos, para listarlo sin obligar al
// cliente a cruzar lotes, medicamentos y usuarios por su cuenta.
struct MovimientoDetallado {
    MovimientoDetallado();

    MovimientoInventario movimiento;
    std::string numLote;
    long long idMedicamento;
    std::string nombreMedicamento;
    std::string unidadMedida;
    std::string nombreUsuario;
};

class MovimientoRepository {
public:
    bool listar(long long limite, std::vector<MovimientoDetallado>& movimientos, std::string& error) const;
    bool existeLote(long long idLote) const;
    bool existeTratamiento(long long idTratamiento) const;
    // Inserta el movimiento. No actualiza el stock: de eso se encargan los
    // triggers trg_movimientos_inventario_ai_stock y trg_lotes_au_total_medicamento.
    bool crear(const MovimientoInventario& movimiento, long long& idGenerado, std::string& error) const;
    bool buscarPorId(long long idMovimiento, MovimientoInventario& movimiento) const;
    bool obtenerStock(long long idLote, StockResultante& stock) const;
};
