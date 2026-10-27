#pragma once

#include <string>

// Una alergia registrada en la historia clínica. La sustancia es texto libre;
// id_medicamento solo se rellena cuando la alergia es a un medicamento que el
// pañol tiene en catálogo.
class Alergia {
public:
    Alergia();

    void setIdAlergia(long long idAlergia);
    long long getIdAlergia() const;

    void setSustancia(const std::string& sustancia);
    std::string getSustancia() const;

    void setIdMedicamento(long long idMedicamento);
    void quitarIdMedicamento();
    long long getIdMedicamento() const;
    bool tieneIdMedicamento() const;

    // Solo de lectura: lo resuelve el repositorio para no obligar al cliente a
    // cruzar la alergia con el catálogo de medicamentos.
    void setNombreMedicamento(const std::string& nombreMedicamento);
    std::string getNombreMedicamento() const;

private:
    long long idAlergia_;
    std::string sustancia_;
    long long idMedicamento_;
    bool tieneIdMedicamento_;
    std::string nombreMedicamento_;
};
