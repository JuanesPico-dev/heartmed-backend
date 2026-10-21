#include "Services/MovimientoService.h"

#include <array>
#include <cctype>
#include <string>

#include "Repositories/UsuarioRepository.h"

namespace {

// Mismos valores que el CHECK de la tabla.
const std::array<const char*, 3> TIPOS_MOVIMIENTO = {"ENTRADA", "CONSUMO", "BAJA"};

std::string limpiarTexto(const std::string& valor) {
    std::string resultado = valor;
    const auto inicio = resultado.find_first_not_of(" \t\r\n");
    if (inicio == std::string::npos) {
        return "";
    }

    const auto fin = resultado.find_last_not_of(" \t\r\n");
    resultado = resultado.substr(inicio, fin - inicio + 1);
    return resultado;
}

std::string aMayusculas(const std::string& valor) {
    std::string resultado = valor;
    for (char& caracter : resultado) {
        caracter = static_cast<char>(std::toupper(static_cast<unsigned char>(caracter)));
    }
    return resultado;
}

bool esTipoValido(const std::string& tipo) {
    for (const char* permitido : TIPOS_MOVIMIENTO) {
        if (tipo == permitido) {
            return true;
        }
    }
    return false;
}

}  // namespace

ResultadoOperacion MovimientoService::registrar(const MovimientoInventario& datos,
                                                const std::string& correoUsuario,
                                                MovimientoInventario& registrado,
                                                StockResultante& stock,
                                                std::string& mensaje) const {
    const std::string tipo = aMayusculas(limpiarTexto(datos.getTipoMovimiento()));
    const std::string motivo = limpiarTexto(datos.getMotivo());

    if (!esTipoValido(tipo)) {
        mensaje = "El tipo de movimiento debe ser ENTRADA, CONSUMO o BAJA";
        return ResultadoOperacion::DatosInvalidos;
    }

    if (datos.getCantidad() <= 0) {
        mensaje = "La cantidad debe ser un entero mayor que cero";
        return ResultadoOperacion::DatosInvalidos;
    }

    if (motivo.size() > 300) {
        mensaje = "El motivo admite máximo 300 caracteres";
        return ResultadoOperacion::DatosInvalidos;
    }

    // Un tratamiento solo justifica un consumo: las entradas y las bajas no
    // salen de la receta de un paciente.
    if (datos.tieneIdTratamiento() && tipo != "CONSUMO") {
        mensaje = "Solo los movimientos de tipo CONSUMO pueden ir ligados a un tratamiento";
        return ResultadoOperacion::DatosInvalidos;
    }

    // Toda salida que no venga de un tratamiento tiene que explicarse.
    if (tipo != "ENTRADA" && !datos.tieneIdTratamiento() && motivo.empty()) {
        mensaje = "Un movimiento de " + tipo + " sin tratamiento asociado necesita un motivo";
        return ResultadoOperacion::DatosInvalidos;
    }

    MovimientoRepository repository;

    if (datos.getIdLote() <= 0 || !repository.existeLote(datos.getIdLote())) {
        mensaje = "No existe el lote indicado";
        return ResultadoOperacion::NoEncontrado;
    }

    if (datos.tieneIdTratamiento() &&
        (datos.getIdTratamiento() <= 0 || !repository.existeTratamiento(datos.getIdTratamiento()))) {
        mensaje = "No existe el tratamiento indicado";
        return ResultadoOperacion::NoEncontrado;
    }

    UsuarioRepository usuarios;
    const long long idUsuario = usuarios.obtenerIdPorCorreo(correoUsuario);
    if (idUsuario <= 0) {
        mensaje = "La cuenta que firma el movimiento ya no está activa";
        return ResultadoOperacion::Conflicto;
    }

    MovimientoInventario movimiento;
    movimiento.setIdLote(datos.getIdLote());
    movimiento.setIdUsuario(idUsuario);
    movimiento.setTipoMovimiento(tipo);
    movimiento.setCantidad(datos.getCantidad());
    movimiento.setMotivo(motivo);

    if (datos.tieneIdTratamiento()) {
        movimiento.setIdTratamiento(datos.getIdTratamiento());
    }

    long long idGenerado = 0;
    std::string error;
    if (!repository.crear(movimiento, idGenerado, error)) {
        // Los dos triggers que pueden abortar el INSERT describen situaciones
        // que el usuario puede corregir, así que son conflicto y no error
        // interno: no hay stock suficiente, o el lote no es del medicamento
        // que el tratamiento receta.
        if (error.find("stock") != std::string::npos ||
            error.find("tratamiento") != std::string::npos) {
            mensaje = error;
            return ResultadoOperacion::Conflicto;
        }

        mensaje = error.empty() ? "No se pudo registrar el movimiento" : error;
        return ResultadoOperacion::ErrorInterno;
    }

    if (!repository.buscarPorId(idGenerado, registrado)) {
        movimiento.setIdMovimiento(idGenerado);
        registrado = movimiento;
    }

    // El stock lo recalculan los triggers, así que se vuelve a leer.
    repository.obtenerStock(movimiento.getIdLote(), stock);

    mensaje = "Movimiento registrado";
    return ResultadoOperacion::Exito;
}

ResultadoOperacion MovimientoService::listar(long long limite,
                                             std::vector<MovimientoDetallado>& movimientos,
                                             std::string& mensaje) const {
    // Sin tope la consulta crecería sin control según se use el sistema, y el
    // historial completo no cabe en una pantalla.
    const long long LIMITE_POR_DEFECTO = 50;
    const long long LIMITE_MAXIMO = 200;

    long long limiteEfectivo = limite <= 0 ? LIMITE_POR_DEFECTO : limite;
    if (limiteEfectivo > LIMITE_MAXIMO) {
        limiteEfectivo = LIMITE_MAXIMO;
    }

    MovimientoRepository repository;
    std::string error;
    if (!repository.listar(limiteEfectivo, movimientos, error)) {
        mensaje = error.empty() ? "No se pudo consultar el historial de movimientos" : error;
        return ResultadoOperacion::ErrorInterno;
    }

    mensaje = "Historial de movimientos";
    return ResultadoOperacion::Exito;
}
