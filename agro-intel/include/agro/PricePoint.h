#pragma once

#include <string>

namespace agro {

// Un precio observado: la unidad básica de todo el sistema.
struct PricePoint {
    std::string date;       // ISO 8601: "2025-11-03"
    std::string product;    // "aceite"
    std::string category;   // "virgen_extra", "virgen", "lampante"
    std::string market;     // "espana_origen", "italia"...
    double price_eur_kg = 0.0;
    std::string source;     // "poolred", "mapa", "manual"...
};

} // namespace agro
