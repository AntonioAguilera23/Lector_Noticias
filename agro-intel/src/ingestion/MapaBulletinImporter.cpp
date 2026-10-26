#include "ingestion/MapaBulletinImporter.h"
#include <poppler/cpp/poppler-document.h>
#include <poppler/cpp/poppler-page.h>
#include <memory>
#include <stdexcept>
#include <iostream>

namespace agro {

std::vector<PricePoint> MapaBulletinImporter::load(const std::string& path) {
    warnings_.clear();
    std::vector<PricePoint> prices;

    // 1. Cargar el documento PDF usando Poppler C++
    std::unique_ptr<poppler::document> doc(poppler::document::load_from_file(path));
    if (!doc || doc->is_locked()) {
        warnings_.push_back("No se pudo abrir o el PDF está protegido: " + path);
        return prices;
    }

    int num_pages = doc->pages();

    // 2. Iterar sobre las páginas del boletín para extraer texto
    for (int i = 0; i < num_pages; ++i) {
        std::unique_ptr<poppler::page> page(doc->create_page(i));
        if (!page) continue;

        // Extraer texto plano de la página actual
        ustring text = page->text();
        
        // Convertir ustring (poppler) a std::string estándar
        std::string page_text = text.to_utf8();

        // TODO: Implementar la lógica de análisis (parsing) del texto extraído
        // para buscar patrones de precios, fechas y productos del MAPA.
    }

    if (prices.empty()) {
        warnings_.push_back("Se leyó el PDF pero no se extrajeron precios estructurados: " + path);
    }

    return prices;
}

} // namespace agro