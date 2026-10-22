#include "ingestion/MapaBulletinImporter.h"
#include <stdexcept>

namespace agro {

std::vector<PricePoint> MapaBulletinImporter::load(const std::string& path) {
    warnings_.clear();
    // TODO: Implementar lectura del PDF del MAPA
    warnings_.push_back("El importador de PDF del MAPA está en construcción para: " + path);
    return {};
}

} // namespace agro