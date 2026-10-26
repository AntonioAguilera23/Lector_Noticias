#include "ingestion/CsvPriceImporter.h"
#include <algorithm>
#include <fstream>
#include <stdexcept>

namespace agro {

namespace {
    
std::vector<std::string> splitCsvLine(const std::string& line) {

    std::vector<std::string> fields;
    std::string cur;
    bool inQuotes = false;
    for (char c : line) {
        if (c == '"') inQuotes = !inQuotes;
        else if (c == ',' && !inQuotes) { fields.push_back(cur); cur.clear(); }
        else if (c != '\r') cur += c;
    }
    fields.push_back(cur);
    return fields;
    
}

bool looksLikeIsoDate(const std::string& s) {
    return s.size() == 10 && s[4] == '-' && s[7] == '-';
}
} // namespace

std::vector<PricePoint> CsvPriceImporter::load(const std::string& path) {
    std::ifstream in(path);
    if (!in) throw std::runtime_error("No se pudo abrir " + path);

    warnings_.clear();
    std::vector<PricePoint> out;
    std::string line;
    std::getline(in, line); // cabecera

    for (int n = 2; std::getline(in, line); ++n) {

        if (line.empty() || line[0] == '#') continue;

        auto f = splitCsvLine(line);

        const std::string where = "Línea " + std::to_string(n) + ": ";

        if (f.size() != 6) {

            warnings_.push_back(where + "se esperaban 6 campos");
            continue;

        }

        if (!looksLikeIsoDate(f[0])) {

            warnings_.push_back(where + "fecha no ISO (" + f[0] + ")");
            continue;
        }

        std::replace(f[4].begin(), f[4].end(), ',', '.');

        try {

            std::size_t used = 0;
            double price = std::stod(f[4], &used);
            if (used != f[4].size() || price <= 0) throw std::invalid_argument("");
            out.push_back({f[0], f[1], f[2], f[3], price, f[5]});

        } catch (const std::exception&) {
            warnings_.push_back(where + "precio inválido (" + f[4] + ")");
        }

    }

    return out;
}

} // namespace agro
