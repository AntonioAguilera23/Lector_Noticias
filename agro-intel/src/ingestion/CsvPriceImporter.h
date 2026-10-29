#pragma once
#include "agro/PricePoint.h"
#include "agro/PriceSource.h"
#include <string>
#include <vector>

namespace agro {

// Lee un CSV con cabecera: fecha,producto,categoria,mercado,precio_eur_kg,fuente
// Admite coma decimal ("3,85") si el campo va entre comillas.
class CsvPriceImporter : public PriceSource {

public:

    std::vector<PricePoint> load(const std::string& path) override;
    const std::vector<std::string>& warnings() const override{ return warnings_; }

private:
    std::vector<std::string> warnings_;
};

} // namespace agro
