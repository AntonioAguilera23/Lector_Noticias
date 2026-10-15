#include "storage/Database.h"
#include <filesystem>
#include <stdexcept>

namespace agro {

namespace {
// Libera el statement aunque salte una excepciÃ³n.
struct Stmt {
    sqlite3_stmt* s = nullptr;
    ~Stmt() { sqlite3_finalize(s); }
};

std::string col(sqlite3_stmt* s, int i) {
    auto* t = sqlite3_column_text(s, i);
    return t ? reinterpret_cast<const char*>(t) : "";
}
} // namespace

Database::Database(const std::string& path) {
    // Crea el directorio padre si existe (p. ej. data/agro.db), incluso
    // cuando el ejecutable se lanza desde build/.
    const std::filesystem::path dbPath(path);
    if (!dbPath.parent_path().empty()) {
        std::error_code ec;
        std::filesystem::create_directories(dbPath.parent_path(), ec);
        if (ec) {
            throw std::runtime_error(
                "No se pudo crear el directorio de la BD: " + ec.message());
        }
    }

    if (sqlite3_open(path.c_str(), &db_) != SQLITE_OK) {
        std::string msg = sqlite3_errmsg(db_);
        sqlite3_close(db_);
        throw std::runtime_error("No se pudo abrir la BD: " + msg);
    }
    createSchema();
}

Database::~Database() { sqlite3_close(db_); }

void Database::exec(const std::string& sql) {
    char* err = nullptr;
    if (sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &err) != SQLITE_OK) {
        std::string msg = err ? err : "error desconocido";
        sqlite3_free(err);
        throw std::runtime_error("SQL: " + msg);
    }
}

void Database::createSchema() {
    exec(R"(
        CREATE TABLE IF NOT EXISTS prices (
            id           INTEGER PRIMARY KEY,
            date         TEXT NOT NULL,
            product      TEXT NOT NULL,
            category     TEXT NOT NULL,
            market       TEXT NOT NULL,
            price_eur_kg REAL NOT NULL,
            source       TEXT NOT NULL,
            UNIQUE(date, product, category, market, source)
        );
        CREATE INDEX IF NOT EXISTS idx_prices_lookup
            ON prices(product, category, date);
    )");
}

int Database::insertPrices(const std::vector<PricePoint>& prices) {
    exec("BEGIN");
    Stmt st;
    const char* sql =
        "INSERT OR IGNORE INTO prices "
        "(date, product, category, market, price_eur_kg, source) "
        "VALUES (?, ?, ?, ?, ?, ?)";
    if (sqlite3_prepare_v2(db_, sql, -1, &st.s, nullptr) != SQLITE_OK) {
        exec("ROLLBACK");
        throw std::runtime_error(sqlite3_errmsg(db_));
    }

    int inserted = 0;
    for (const auto& p : prices) {
        sqlite3_bind_text(st.s, 1, p.date.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(st.s, 2, p.product.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(st.s, 3, p.category.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(st.s, 4, p.market.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_double(st.s, 5, p.price_eur_kg);
        sqlite3_bind_text(st.s, 6, p.source.c_str(), -1, SQLITE_TRANSIENT);

        if (sqlite3_step(st.s) != SQLITE_DONE) {
            std::string msg = sqlite3_errmsg(db_);
            exec("ROLLBACK");
            throw std::runtime_error(msg);
        }
        inserted += sqlite3_changes(db_);
        sqlite3_reset(st.s);
    }
    exec("COMMIT");
    return inserted;
}

std::vector<PricePoint> Database::latestPrices(const std::string& product,
                                               const std::string& category,
                                               int limit) const {
    Stmt st;
    const char* sql =
        "SELECT date, product, category, market, price_eur_kg, source "
        "FROM prices WHERE product = ? AND category = ? "
        "ORDER BY date DESC LIMIT ?";
    if (sqlite3_prepare_v2(db_, sql, -1, &st.s, nullptr) != SQLITE_OK) {
        throw std::runtime_error(
            std::string("No se pudo preparar latestPrices: ") + sqlite3_errmsg(db_));
    }
    sqlite3_bind_text(st.s, 1, product.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(st.s, 2, category.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(st.s, 3, limit);

    std::vector<PricePoint> out;
    int rc = SQLITE_OK;
    while ((rc = sqlite3_step(st.s)) == SQLITE_ROW) {
        out.push_back({col(st.s, 0), col(st.s, 1), col(st.s, 2), col(st.s, 3),
                       sqlite3_column_double(st.s, 4), col(st.s, 5)});
    }
    if (rc != SQLITE_DONE) {
        throw std::runtime_error(
            std::string("Error al consultar latestPrices: ") + sqlite3_errmsg(db_));
    }
    return out;
}

int Database::countPrices() const {
    Stmt st;
    if (sqlite3_prepare_v2(db_, "SELECT COUNT(*) FROM prices", -1, &st.s, nullptr)
        != SQLITE_OK) {
        throw std::runtime_error(
            std::string("No se pudo preparar countPrices: ") + sqlite3_errmsg(db_));
    }

    const int rc = sqlite3_step(st.s);
    if (rc != SQLITE_ROW) {
        throw std::runtime_error(
            std::string("Error al consultar countPrices: ") + sqlite3_errmsg(db_));
    }
    return sqlite3_column_int(st.s, 0);
}

} // namespace agro