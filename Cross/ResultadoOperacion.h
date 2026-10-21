#pragma once

// Resultado estándar de los services para decidir el código HTTP.
enum class ResultadoOperacion {
    Exito,
    DatosInvalidos,
    NoEncontrado,
    Duplicado,
    // Operación válida pero no permitida por el estado actual de los datos.
    Conflicto,
    ErrorInterno,
};
