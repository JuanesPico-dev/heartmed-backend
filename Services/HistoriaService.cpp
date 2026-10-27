#include "Services/HistoriaService.h"

#include <algorithm>
#include <cctype>
#include <string>
#include <vector>

namespace {

// Topes de texto. observaciones comparte el de PUT /api/pacientes, que escribe
// la misma columna: si no coincidieran, el mismo dato se aceptaría por una
// ruta y se rechazaría por la otra.
const std::size_t MAXIMO_OBSERVACIONES = 500;
const std::size_t MAXIMO_NARRATIVO = 2000;
const std::size_t MAXIMO_SUSTANCIA = 100;
const std::size_t MAXIMO_ALERGIAS = 50;

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

std::string aMinusculas(const std::string& valor) {
    std::string resultado = valor;
    for (char& caracter : resultado) {
        caracter = static_cast<char>(std::tolower(static_cast<unsigned char>(caracter)));
    }
    return resultado;
}

}  // namespace

ResultadoOperacion HistoriaService::consultar(long long idPaciente,
                                              HistoriaClinica& historia,
                                              std::string& mensaje) const {
    if (idPaciente <= 0) {
        mensaje = "El identificador del paciente no es válido";
        return ResultadoOperacion::DatosInvalidos;
    }

    HistoriaRepository repository;
    if (!repository.existePaciente(idPaciente)) {
        mensaje = "No existe un paciente con ese identificador";
        return ResultadoOperacion::NoEncontrado;
    }

    std::string error;
    if (!repository.buscarPorPaciente(idPaciente, historia, error)) {
        if (!error.empty()) {
            mensaje = error;
            return ResultadoOperacion::ErrorInterno;
        }

        // El paciente existe pero le falta la historia. No debería pasar: la
        // crea el alta del paciente. Se distingue del 404 anterior para que
        // quien lo vea sepa que es un dato incompleto, no un id equivocado.
        mensaje = "El paciente no tiene historia clínica registrada";
        return ResultadoOperacion::NoEncontrado;
    }

    mensaje = "Historia clínica";
    return ResultadoOperacion::Exito;
}

ResultadoOperacion HistoriaService::actualizar(long long idPaciente,
                                               const CambiosHistoria& cambios,
                                               HistoriaClinica& actualizada,
                                               std::string& mensaje) const {
    HistoriaClinica actual;
    const ResultadoOperacion existente = consultar(idPaciente, actual, mensaje);
    if (existente != ResultadoOperacion::Exito) {
        return existente;
    }

    CambiosHistoria limpios;
    HistoriaRepository repository;

    if (cambios.antecedentes.has_value()) {
        const std::string valor = limpiarTexto(cambios.antecedentes.value());
        if (valor.size() > MAXIMO_NARRATIVO) {
            mensaje = "Los antecedentes admiten máximo 2000 caracteres";
            return ResultadoOperacion::DatosInvalidos;
        }
        limpios.antecedentes = valor;
    }

    if (cambios.cirugiasPrevias.has_value()) {
        const std::string valor = limpiarTexto(cambios.cirugiasPrevias.value());
        if (valor.size() > MAXIMO_NARRATIVO) {
            mensaje = "Las cirugías previas admiten máximo 2000 caracteres";
            return ResultadoOperacion::DatosInvalidos;
        }
        limpios.cirugiasPrevias = valor;
    }

    if (cambios.observaciones.has_value()) {
        const std::string valor = limpiarTexto(cambios.observaciones.value());
        if (valor.size() > MAXIMO_OBSERVACIONES) {
            mensaje = "Las observaciones admiten máximo 500 caracteres";
            return ResultadoOperacion::DatosInvalidos;
        }
        limpios.observaciones = valor;
    }

    if (cambios.frutaDiablo.has_value()) {
        limpios.frutaDiablo = cambios.frutaDiablo.value();
    }

    if (cambios.alergias.has_value()) {
        std::vector<Alergia> alergias;
        std::vector<std::string> vistas;

        for (const Alergia& entrada : cambios.alergias.value()) {
            const std::string sustancia = limpiarTexto(entrada.getSustancia());

            if (sustancia.empty() || sustancia.size() > MAXIMO_SUSTANCIA) {
                mensaje = "Cada alergia necesita una sustancia de 1 a 100 caracteres";
                return ResultadoOperacion::DatosInvalidos;
            }

            // La base tiene un UNIQUE (id_historia, sustancia) que además
            // ignora mayúsculas. Se comprueba aquí para decir cuál es la
            // repetida en vez de soltar el error crudo del UNIQUE.
            const std::string clave = aMinusculas(sustancia);
            if (std::find(vistas.begin(), vistas.end(), clave) != vistas.end()) {
                mensaje = "La alergia a " + sustancia + " viene repetida en la lista";
                return ResultadoOperacion::DatosInvalidos;
            }
            vistas.push_back(clave);

            Alergia alergia;
            alergia.setSustancia(sustancia);

            if (entrada.tieneIdMedicamento()) {
                if (!repository.existeMedicamento(entrada.getIdMedicamento())) {
                    mensaje = "No existe el medicamento indicado en la alergia a " + sustancia;
                    return ResultadoOperacion::NoEncontrado;
                }
                alergia.setIdMedicamento(entrada.getIdMedicamento());
            }

            alergias.push_back(alergia);
        }

        if (alergias.size() > MAXIMO_ALERGIAS) {
            mensaje = "No se pueden registrar más de 50 alergias en una historia";
            return ResultadoOperacion::DatosInvalidos;
        }

        limpios.alergias = alergias;

        // Enviar la lista, aunque venga vacía, significa que ya se le preguntó
        // al paciente: "revisadas y ninguna" es un dato clínico, distinto de
        // "todavía sin preguntar". Un alergias_revisadas explícito manda sobre
        // esta deducción.
        limpios.alergiasRevisadas = true;
    }

    if (cambios.alergiasRevisadas.has_value()) {
        limpios.alergiasRevisadas = cambios.alergiasRevisadas.value();
    }

    std::string error;
    if (!repository.actualizar(actual.getIdHistoria(), limpios, error)) {
        if (error.find("UNIQUE") != std::string::npos) {
            mensaje = "Hay dos alergias con la misma sustancia";
            return ResultadoOperacion::Duplicado;
        }

        mensaje = error.empty() ? "No se pudo actualizar la historia clínica" : error;
        return ResultadoOperacion::ErrorInterno;
    }

    std::string mensajeLectura;
    const ResultadoOperacion relectura = consultar(idPaciente, actualizada, mensajeLectura);
    if (relectura != ResultadoOperacion::Exito) {
        mensaje = mensajeLectura;
        return relectura;
    }

    mensaje = "Historia clínica actualizada";
    return ResultadoOperacion::Exito;
}
