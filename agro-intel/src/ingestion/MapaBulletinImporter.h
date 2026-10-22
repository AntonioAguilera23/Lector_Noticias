#pragma once
#include "agro/PriceSource.h"
#include <string>
#include <vector>

namespace agro {

// Extrae precios desde los boletines semanales en PDF del MAPA.
class MapaBulletinImporter : public PriceSource {
public:
    std::vector<PricePoint> load(const std::string& path) override;
    const std::vector<std::string>& warnings() const override { return warnings_; }

private:
    std::vector<std::string> warnings_;
};

} // namespace agro