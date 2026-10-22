#include "ingestion/CsvPriceImporter.h"
#include "storage/Database.h"
#include <iomanip>
#include <iostream>

namespace {

void usage() {
    std::cout << "Uso:\n"
                 "  agro-intel import <fichero.csv>\n"
                 "  agro-intel show <producto> <categoria> [n]\n";
}

} // namespace

int main(int argc, char** argv) {

    if (argc < 2) { usage(); return 1; }
    
    const std::string cmd = argv[1];

    try {
        agro::Database db("data/agro.db");

        if (cmd == "import" && argc == 3) {
            agro::CsvPriceImporter importer;
            auto prices = importer.load(argv[2]);
            for (const auto& w : importer.warnings()) std::cerr << "Aviso: " << w << '\n';
            int added = db.insertPrices(prices);
            std::cout << "Leídos " << prices.size() << ", nuevos " << added
                      << ", total en BD " << db.countPrices() << '\n';
        } else if (cmd == "show" && (argc == 4 || argc == 5)) {
            int n = argc == 5 ? std::stoi(argv[4]) : 10;
            for (const auto& p : db.latestPrices(argv[2], argv[3], n)) {
                std::cout << p.date << "  " << std::setw(14) << p.market << "  "
                          << std::fixed << std::setprecision(3) << p.price_eur_kg
                          << " €/kg  (" << p.source << ")\n";
            }
        } else {
            usage();
            return 1;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << '\n';
        return 1;
    }
    return 0;
}
