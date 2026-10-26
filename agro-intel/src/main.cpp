#include "agro/PriceSource.h"
#include "ingestion/CsvPriceImporter.h"
#include "ingestion/MapaBulletinImporter.h"
#include "storage/Database.h"
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <memory>
#include <stdexcept>

namespace {

    void usage() {
        std::cout << "Uso:\n"
                    "  agro-intel import <fichero.csv | fichero.pdf>\n"
                    "  agro-intel show <producto> <categoria> [n]\n";
    }

// Factoría polimórfica: decide qué clase concreta instanciar según la extensión,
// pero devuelve un puntero a la interfaz base PriceSource.
    std::unique_ptr<agro::PriceSource> createSourceForFile(const std::string& path){

        auto ext = std::filesystem::path(path).extension().string();  //Esto guarda en ext .pdf o .csv
        if (ext == ".csv") {
            return std::make_unique<agro::CsvPriceImporter>();
        }
        if (ext == ".pdf") {
            return std::make_unique<agro::MapaBulletinImporter>();
        }

        throw std::runtime_error("Formato de fichero no soportado: " + ext);

    }

} // namespace

int main(int argc, char** argv) {

    if (argc < 2) { usage(); return 1; }
    
    const std::string cmd = argv[1];

    try {
        agro::Database db("data/agro.db");

        if (cmd == "import" && argc == 3) {
            // Polimorfismo en acción: 'source' es un puntero a PriceSource.
            // No sabemos ni nos importa si por debajo es CSV o PDF.
            std::unique_ptr<agro::PriceSource> source = createSourceForFile(argv[2]);

            auto prices = source->load(argv[2]);
            for (const auto& w : source->warnings()) {
                std::cerr << "Aviso: " << w << '\n';
            }

            int added = db.insertPrices(prices);
            std::cout << "Leídos " << prices.size() << ", nuevos " << added << ", total en BD " << db.countPrices() << '\n';
        } else if (cmd == "show" && (argc == 4 || argc == 5)) {
            int n = argc == 5 ? std::stoi(argv[4]) : 10;
            for (const auto& p : db.latestPrices(argv[2], argv[3], n)) {
                std::cout << p.date << "  " << std::setw(14) << p.market << "  " << std::fixed << std::setprecision(3) << p.price_eur_kg << " €/kg  (" << p.source << ")\n";
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