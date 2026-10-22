#include "Services/PacienteService.h"

#include <array>
#include <cctype>
#include <string>
#include <vector>

#include "Repositories/PacienteRepository.h"

namespace {

// Mismos valores que los CHECK de la tabla pacientes. Si cambian en el esquema,
// hay que cambiarlos aquí: validamos antes para devolver un 400 con mensaje
// claro en vez de dejar que reviente la base con un error genérico.
const std::array<const char*, 4> TIPOS_SANGRE = {"F", "S", "X", "XF"};
const std::array<const char*, 4> ESTADOS_SALUD = {"SANO", "EN_TRATAMIENTO", "EN_RECUPERACION", "CRITICO"};

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

bool estaEnLista(const std::string& valor, const std::array<const char*, 4>& permitidos) {
    for (const char* permitido : permitidos) {
        if (valor == permitido) {
            return true;
        }
    }
    return false;
}

std::string listaComoTexto(const std::array<const char*, 4>& permitidos) {
    std::string resultado;
    for (std::size_t i = 0; i < permitidos.size(); ++i) {
        if (i > 0) {
            resultado += ", ";
        }
        resultado += permitidos[i];
    }
    return resultado;
}

// La base exige date(fecha_nacimiento) IS fecha_nacimiento, o sea un
// YYYY-MM-DD que además exista en el calendario.
bool esFechaValida(const std::string& fecha) {
    if (fecha.size() != 10 || fecha[4] != '-' || fecha[7] != '-') {
        return false;
    }

    for (std::size_t i = 0; i < fecha.size(); ++i) {
        if (i == 4 || i == 7) {
            continue;
        }
        if (std::isdigit(static_cast<unsigned char>(fecha[i])) == 0) {
            return false;
        }
    }

    const int anio = std::stoi(fecha.substr(0, 4));
    const int mes = std::stoi(fecha.substr(5, 2));
    const int dia = std::stoi(fecha.substr(8, 2));

    if (mes < 1 || mes > 12 || dia < 1) {
        return false;
    }

    std::array<int, 12> diasPorMes = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    const bool bisiesto = (anio % 4 == 0 && anio % 100 != 0) || (anio % 400 == 0);
    if (mes == 2 && bisiesto) {
        diasPorMes[1] = 29;
    }

    return dia <= diasPorMes[mes - 1];
}

}  // namespace

ResultadoOperacion PacienteService::listar(std::vector<Paciente>& pacientes, std::string& mensaje) const {
    PacienteRepository repository;
    std::string error;

    if (!repository.listar(pacientes, error)) {
        mensaje = error.empty() ? "No se pudo consultar la lista de pacientes" : error;
        return ResultadoOperacion::ErrorInterno;
    }

    mensaje = "Listado de pacientes";
    return ResultadoOperacion::Exito;
}

ResultadoOperacion PacienteService::obtenerPorId(long long idPaciente,
                                                 Paciente& paciente,
                                                 std::string& mensaje) const {
    if (idPaciente <= 0) {
        mensaje = "El identificador del paciente no es válido";
        return ResultadoOperacion::DatosInvalidos;
    }

    PacienteRepository repository;
    if (!repository.buscarPorId(idPaciente, paciente)) {
        mensaje = "No existe un paciente con ese identificador";
        return ResultadoOperacion::NoEncontrado;
    }

    mensaje = "Paciente encontrado";
    return ResultadoOperacion::Exito;
}

namespace {

// Limpia, normaliza y valida los datos que llegan de la API. Devuelve el
// paciente listo para guardar. Lo comparten el alta y la edición.
ResultadoOperacion prepararPaciente(const Paciente& datos, Paciente& limpio, std::string& mensaje) {
    const std::string documento = limpiarTexto(datos.getDocumentoIdentidad());
    const std::string nombre = limpiarTexto(datos.getNombreCompleto());
    const std::string foto = limpiarTexto(datos.getFotoPerfil());
    const std::string fecha = limpiarTexto(datos.getFechaNacimiento());
    const std::string sangre = aMayusculas(limpiarTexto(datos.getTipoSangre()));
    const std::string rango = limpiarTexto(datos.getRangoCargo());
    const std::string estado = aMayusculas(limpiarTexto(datos.getEstadoSalud()));
    const std::string observaciones = limpiarTexto(datos.getObservaciones());

    if (documento.empty() || documento.size() > 50) {
        mensaje = "El documento de identidad es obligatorio y admite máximo 50 caracteres";
        return ResultadoOperacion::DatosInvalidos;
    }

    if (nombre.size() < 2 || nombre.size() > 150) {
        mensaje = "El nombre completo debe tener entre 2 y 150 caracteres";
        return ResultadoOperacion::DatosInvalidos;
    }

    if (foto.size() > 255) {
        mensaje = "La foto de perfil admite máximo 255 caracteres";
        return ResultadoOperacion::DatosInvalidos;
    }

    if (!esFechaValida(fecha)) {
        mensaje = "La fecha de nacimiento debe tener el formato YYYY-MM-DD y ser una fecha real";
        return ResultadoOperacion::DatosInvalidos;
    }

    if (!estaEnLista(sangre, TIPOS_SANGRE)) {
        mensaje = "El tipo de sangre debe ser uno de: " + listaComoTexto(TIPOS_SANGRE);
        return ResultadoOperacion::DatosInvalidos;
    }

    if (rango.empty() || rango.size() > 50) {
        mensaje = "El rango o cargo es obligatorio y admite máximo 50 caracteres";
        return ResultadoOperacion::DatosInvalidos;
    }

    if (datos.getRecompensa() < 0) {
        mensaje = "La recompensa no puede ser negativa";
        return ResultadoOperacion::DatosInvalidos;
    }

    if (!estaEnLista(estado, ESTADOS_SALUD)) {
        mensaje = "El estado de salud debe ser uno de: " + listaComoTexto(ESTADOS_SALUD);
        return ResultadoOperacion::DatosInvalidos;
    }

    if (observaciones.size() > 500) {
        mensaje = "Las observaciones admiten máximo 500 caracteres";
        return ResultadoOperacion::DatosInvalidos;
    }

    if (datos.tieneIdUsuario()) {
        limpio.setIdUsuario(datos.getIdUsuario());
    }
    limpio.setIdPaciente(datos.getIdPaciente());
    limpio.setDocumentoIdentidad(documento);
    limpio.setNombreCompleto(nombre);
    limpio.setFotoPerfil(foto);
    limpio.setFechaNacimiento(fecha);
    limpio.setTipoSangre(sangre);
    limpio.setRangoCargo(rango);
    limpio.setRecompensa(datos.getRecompensa());
    limpio.setEstadoSalud(estado);
    limpio.setObservaciones(observaciones);

    mensaje.clear();
    return ResultadoOperacion::Exito;
}

}  // namespace

ResultadoOperacion PacienteService::crear(const Paciente& datos, Paciente& creado, std::string& mensaje) const {
    Paciente paciente;
    const ResultadoOperacion validacion = prepararPaciente(datos, paciente, mensaje);
    if (validacion != ResultadoOperacion::Exito) {
        return validacion;
    }

    PacienteRepository repository;
    if (repository.existeDocumento(paciente.getDocumentoIdentidad())) {
        mensaje = "Ya existe un paciente con ese documento de identidad";
        return ResultadoOperacion::Duplicado;
    }

    long long idGenerado = 0;
    std::string error;
    if (!repository.crear(paciente, idGenerado, error)) {
        // El UNIQUE del documento puede saltar igualmente si dos peticiones
        // entran a la vez: la base es la que manda, no la comprobación previa.
        if (error.find("UNIQUE") != std::string::npos) {
            mensaje = "Ya existe un paciente con ese documento de identidad";
            return ResultadoOperacion::Duplicado;
        }

        mensaje = error.empty() ? "No se pudo registrar el paciente" : error;
        return ResultadoOperacion::ErrorInterno;
    }

    if (!repository.buscarPorId(idGenerado, creado)) {
        paciente.setIdPaciente(idGenerado);
        creado = paciente;
    }

    mensaje = "Paciente registrado";
    return ResultadoOperacion::Exito;
}

ResultadoOperacion PacienteService::actualizar(long long idPaciente,
                                               const Paciente& datos,
                                               Paciente& actualizado,
                                               std::string& mensaje) const {
    if (idPaciente <= 0) {
        mensaje = "El identificador del paciente no es válido";
        return ResultadoOperacion::DatosInvalidos;
    }

    Paciente paciente;
    const ResultadoOperacion validacion = prepararPaciente(datos, paciente, mensaje);
    if (validacion != ResultadoOperacion::Exito) {
        return validacion;
    }

    paciente.setIdPaciente(idPaciente);

    PacienteRepository repository;
    if (!repository.existeId(idPaciente)) {
        mensaje = "No existe un paciente con ese identificador";
        return ResultadoOperacion::NoEncontrado;
    }

    if (repository.existeDocumento(paciente.getDocumentoIdentidad(), idPaciente)) {
        mensaje = "Ya existe otro paciente con ese documento de identidad";
        return ResultadoOperacion::Duplicado;
    }

    std::string error;
    if (!repository.actualizar(paciente, error)) {
        if (error.find("UNIQUE") != std::string::npos) {
            mensaje = "Ya existe otro paciente con ese documento de identidad";
            return ResultadoOperacion::Duplicado;
        }

        // El trigger trg_pacientes_bu_estado_salud impide cambiar el estado de
        // salud de un paciente que ya tiene controles de evolución: ese estado
        // se cambia registrando un control nuevo, no editando la ficha.
        if (error.find("controles") != std::string::npos) {
            mensaje = error;
            return ResultadoOperacion::Conflicto;
        }

        mensaje = error.empty() ? "No se pudo actualizar el paciente" : error;
        return ResultadoOperacion::ErrorInterno;
    }

    if (!repository.buscarPorId(idPaciente, actualizado)) {
        actualizado = paciente;
    }

    mensaje = "Paciente actualizado";
    return ResultadoOperacion::Exito;
}
