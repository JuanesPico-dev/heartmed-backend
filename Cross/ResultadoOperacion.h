#pragma once

// Resultado común de la capa de Services. Permite que las rutas elijan el
// código HTTP sin interpretar el texto del mensaje de error.
// Vive aparte de ApiHttp.h para que los Services no dependan de Crow.
enum class ResultadoOperacion {
    Exito,
    DatosInvalidos,
    NoEncontrado,
    Duplicado,
    // La operación es válida pero el estado actual de los datos no la permite,
    // por ejemplo cambiar el estado de salud de un paciente que ya tiene
    // controles de evolución registrados.
    Conflicto,
    ErrorInterno,
};
