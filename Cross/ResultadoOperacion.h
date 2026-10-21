#pragma once

// Resultado común de la capa de Services. Permite que las rutas elijan el
// código HTTP sin interpretar el texto del mensaje de error.
// Vive aparte de ApiHttp.h para que los Services no dependan de Crow.
enum class ResultadoOperacion {
    Exito,
    DatosInvalidos,
    Duplicado,
    ErrorInterno,
};
