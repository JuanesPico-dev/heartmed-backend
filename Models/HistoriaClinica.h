#pragma once

#include <string>
#include <vector>

#include "Models/Alergia.h"

// La historia clínica de un paciente. Es 1 a 1 con él: la crea el backend al
// registrarlo y nunca se borra.
//
// alergiasRevisadas distingue "todavía no se le ha preguntado" (false) de "ya
// se preguntó y no tiene ninguna" (true sin alergias en la lista). Son dos
// situaciones clínicas distintas y una lista vacía sola no las separa.
class HistoriaClinica {
public:
    HistoriaClinica();

    void setIdHistoria(long long idHistoria);
    long long getIdHistoria() const;

    void setIdPaciente(long long idPaciente);
    long long getIdPaciente() const;

    // Solo de lectura, para que la pantalla no tenga que pedir el paciente aparte.
    void setNombrePaciente(const std::string& nombrePaciente);
    std::string getNombrePaciente() const;

    void setAlergiasRevisadas(bool alergiasRevisadas);
    bool getAlergiasRevisadas() const;

    void setAntecedentes(const std::string& antecedentes);
    std::string getAntecedentes() const;

    void setCirugiasPrevias(const std::string& cirugiasPrevias);
    std::string getCirugiasPrevias() const;

    void setObservaciones(const std::string& observaciones);
    std::string getObservaciones() const;

    void setFrutaDiablo(bool frutaDiablo);
    bool getFrutaDiablo() const;

    void setFechaCreacion(const std::string& fechaCreacion);
    std::string getFechaCreacion() const;

    void agregarAlergia(const Alergia& alergia);
    const std::vector<Alergia>& getAlergias() const;

private:
    long long idHistoria_;
    long long idPaciente_;
    std::string nombrePaciente_;
    bool alergiasRevisadas_;
    std::string antecedentes_;
    std::string cirugiasPrevias_;
    std::string observaciones_;
    bool frutaDiablo_;
    std::string fechaCreacion_;
    std::vector<Alergia> alergias_;
};
