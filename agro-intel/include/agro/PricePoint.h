#pragma once
#include <string>
using namespace std; 

namespace agro {

// Un precio observado: la unidad básica de todo el sistema.
struct PricePoint {

    string date;      // ISO 8601: "2025-11-03"
    string product;   // "aceite"
    string category;  // "virgen_extra", "virgen", "lampante"
    string market;    // "espana_origen", "italia"...
    double price_eur_kg = 0.0;
    string source; 
       // "poolred", "mapa", "manual"...
};

} // namespace agro
