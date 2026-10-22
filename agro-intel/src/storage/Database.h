#pragma once
#include "agro/PricePoint.h"
#include <sqlite3.h>
#include <string>
#include <vector>

namespace agro {

// Envoltorio RAII sobre SQLite: abre en el constructor, cierra en el destructor.
class Database {
public:

    explicit Database(const std::string& path);
    ~Database();

    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;

    // Devuelve cuántas filas nuevas se insertaron (los duplicados se ignoran).
    int insertPrices(const std::vector<PricePoint>& prices);

    std::vector<PricePoint> latestPrices(const std::string& product,
                                         const std::string& category,
                                         int limit) const;
    int countPrices() const;

private:

    void exec(const std::string& sql);
    void createSchema();

    sqlite3* db_ = nullptr;
};

} // namespace agro
