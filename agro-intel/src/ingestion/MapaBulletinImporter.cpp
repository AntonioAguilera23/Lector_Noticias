#include "ingestion/MapaBulletinImporter.h"
#include <fpdfview.h>   // Cabecera principal de PDFium
#include <fpdf_text.h>  // Cabecera de PDFium para extracción de texto
#include <memory>
#include <stdexcept>
#include <iostream>
#include <algorithm> 
#include <sstream>   
#include <regex>     
#include <vector>
#include <codecvt>
#include <locale>

namespace agro {

namespace {
    // Convierte el precio manejando la coma decimal
    double parsePriceFromPdf(std::string priceStr) {
        if (priceStr == "-" || priceStr == "s/c") {
            throw std::invalid_argument("Dato nulo o sin cotización");
        }

        std::replace(priceStr.begin(), priceStr.end(), ',', '.');
        std::size_t used = 0;
        double price = std::stod(priceStr, &used);
        
        if (used != priceStr.size() || price <= 0) {
            throw std::invalid_argument("Precio inválido o menor/igual a cero: " + priceStr);
        }
        return price;
    }

    // Función para limpiar el "ruido" de notas al pie en los nombres o precios
    std::string limpiarRuido(const std::string& line) {
        static const std::regex ruido_regex(R"(\(\d+\)|\*|s/c)");
        return std::regex_replace(line, ruido_regex, "");
    }

    // Helper: PDFium extrae el texto en UTF-16, necesitamos convertirlo a UTF-8 para las Regex
    std::string convertUtf16ToUtf8(const std::vector<unsigned short>& buffer) {
        std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> conv;
        return conv.to_bytes(reinterpret_cast<const char16_t*>(buffer.data()));
    }
}

std::vector<PricePoint> MapaBulletinImporter::load(const std::string& path) {
    warnings_.clear();
    std::vector<PricePoint> prices;

    // Inicializar la librería PDFium 
    // (Nota: En una app real, FPDF_InitLibrary() suele llamarse una vez al arrancar el programa)
    FPDF_InitLibrary();

    // 1. Cargar el documento PDF usando PDFium
    FPDF_DOCUMENT doc = FPDF_LoadDocument(path.c_str(), nullptr);
    if (!doc) {
        warnings_.push_back("No se pudo abrir o el PDF está protegido: " + path);
        FPDF_DestroyLibrary();
        return prices;
    }

    int num_pages = FPDF_GetPageCount(doc);

    // Marcadores para delimitar dónde empiezan y terminan las tablas
    std::regex regex_inicio_tabla(R"(PRODUCTOS AGRÍCOLAS|PRODUCTOS GANADEROS|PRECIOS MEDIOS|Semana)", std::regex_constants::icase);
    std::regex regex_fin_tabla(R"(COMENTARIOS DE MERCADO|Posición comercial|Fuente:)", std::regex_constants::icase);
    
    // Regex actualizada: [Texto] + 2 Precios + 2 Variaciones (Opcionales)
    std::regex regex_linea_datos(R"(^(.+?)\s+(\d{1,4},\d{2}|-)\s+(\d{1,4},\d{2}|-)(?:\s+(-?\d{1,4},\d{2}|-))?(?:\s+(-?\d{1,4},\d{2}|-))?.*$)");

    std::string current_date = "2026-10-11"; 
    std::string current_category = "General"; 

    // 2. Iterar sobre las páginas del boletín para extraer texto
    for (int i = 0; i < num_pages; ++i) {
        FPDF_PAGE page = FPDF_LoadPage(doc, i);
        if (!page) continue;

        FPDF_TEXTPAGE textPage = FPDFText_LoadPage(page);
        
        // Extraer texto plano de toda la página con PDFium
        int charsCount = FPDFText_CountChars(textPage);
        std::vector<unsigned short> buffer(charsCount + 1, 0); // +1 para el null terminator
        FPDFText_GetText(textPage, 0, charsCount, buffer.data());
        
        // Convertimos a UTF-8 para procesar línea a línea
        std::string page_text = convertUtf16ToUtf8(buffer);
        
        // Liberar recursos de la página actual
        FPDFText_ClosePage(textPage);
        FPDF_ClosePage(page);

        std::istringstream stream(page_text);
        std::string line;
        int line_num = 0;
        bool extraccion_activa = false;

        // Bucle de procesamiento idéntico al original
        while (std::getline(stream, line)) {
            line_num++;
            
            line.erase(line.find_last_not_of(" \n\r\t") + 1);
            line = limpiarRuido(line);

            if (line.empty()) continue;

            if (std::regex_search(line, regex_inicio_tabla)) {
                extraccion_activa = true;
                continue;
            }
            if (extraccion_activa && std::regex_search(line, regex_fin_tabla)) {
                extraccion_activa = false;
                continue;
            }

            if (extraccion_activa) {
                std::smatch match;
                if (std::regex_match(line, match, regex_linea_datos)) {
                    try {
                        std::string product_name = match[1].str();
                        std::string current_price_str = match[3].str(); 

                        if (current_price_str == "-") {
                            continue;
                        }

                        double price = parsePriceFromPdf(current_price_str);
                        std::string trackable_source = path + " (Pág. " + std::to_string(i + 1) + ")";
                        
                        prices.push_back({
                            current_date, 
                            product_name, 
                            current_category, 
                            "Nacional",
                            price, 
                            trackable_source
                        });

                    } catch (const std::exception& e) {
                        warnings_.push_back("Página " + std::to_string(i+1) + " Línea " + std::to_string(line_num) + " ('" + line + "'): " + e.what());
                    }
                }
            }
        }
    }

    // 3. Cerrar el documento y destruir la librería
    FPDF_CloseDocument(doc);
    
    // (Igual que FPDF_InitLibrary, si lo usas en varios sitios, Destroy no iría aquí sino al cerrar la app)
    FPDF_DestroyLibrary();

    if (prices.empty()) {
        warnings_.push_back("Se leyó el PDF pero no se extrajeron precios estructurados: " + path);
    }

    return prices;
}

} // namespace agro