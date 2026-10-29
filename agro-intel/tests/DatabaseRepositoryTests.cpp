#include "storage/Database.h"
#include "storage/IPriceRepository.h"

#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void require(bool condition, const std::string& message) {
    if (!condition) {
        throw std::runtime_error("Fallo de prueba: " + message);
    }
}

} // namespace

int main() {
    try {
        auto database = std::make_shared<agro::Database>(":memory:");
        agro::SqlitePriceRepository repository(database);

        const std::vector<agro::PricePoint> prices{
            {"2026-10-01", "Tomate", "Hortaliza", "Almería", 1.50, "test"},
            {"2026-10-02", "Tomate", "Hortaliza", "Almería", 1.80, "test"},
            {"2026-10-03", "Tomate", "Hortaliza", "Almería", 1.70, "test"},
        };

        require(repository.save(prices) == 3, "save debe devolver el número de filas insertadas");
        require(database->countPrices() == 3, "countPrices debe devolver tres registros");
        require(repository.save(prices) == 0, "los duplicados deben ignorarse");
        require(repository.save({}) == 0, "guardar una colección vacía debe devolver cero");

        const auto recent = repository.getRecentHistory("Tomate", "Hortaliza", 2);
        require(recent.size() == 2, "getRecentHistory debe respetar el límite");
        require(recent[0].date == "2026-10-03", "historial reciente en orden descendente");
        require(recent[1].date == "2026-10-02", "segundo precio más reciente");

        const auto range = repository.getByDateRange("Tomate", "2026-10-02", "2026-10-03");
        require(range.size() == 2, "consulta por rango debe incluir ambos extremos");
        require(range[0].date == "2026-10-02", "rango en orden cronológico ascendente");
        require(range[1].date == "2026-10-03", "último registro del rango");

        std::cout << "Todas las pruebas de base de datos y repositorio han pasado.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
