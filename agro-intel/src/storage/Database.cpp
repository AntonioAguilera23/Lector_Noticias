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
    try {
        createSchema();
    } catch (...) {
        sqlite3_close(db_);
        db_ = nullptr;
        throw;
    }
}

Database::~Database() {
    if (db_) {
        sqlite3_close(db_);
    }
}

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
    if (prices.empty()) return 0;

    // La transacción se revierte automáticamente si cualquier operación falla.
    struct TransactionGuard {
        sqlite3* db;
        bool committed = false;

        explicit TransactionGuard(sqlite3* connection) : db(connection) {
            char* error = nullptr;
            const int rc = sqlite3_exec(db, "BEGIN TRANSACTION", nullptr, nullptr, &error);
            if (rc != SQLITE_OK) {
                const std::string message = error ? error : sqlite3_errmsg(db);
                sqlite3_free(error);
                throw std::runtime_error("No se pudo iniciar la transacción: " + message);
            }
        }

        ~TransactionGuard() {
            if (!committed) {
                sqlite3_exec(db, "ROLLBACK", nullptr, nullptr, nullptr);
            }
        }

        void commit() {
            char* error = nullptr;
            const int rc = sqlite3_exec(db, "COMMIT", nullptr, nullptr, &error);
            if (rc != SQLITE_OK) {
                const std::string message = error ? error : sqlite3_errmsg(db);
                sqlite3_free(error);
                throw std::runtime_error("No se pudo confirmar la transacción: " + message);
            }
            committed = true;
        }
    } transaction(db_);

    const char* sql =
        "INSERT OR IGNORE INTO prices "
        "(date, product, category, market, source, price_eur_kg) "
        "VALUES (?, ?, ?, ?, ?, ?);";

    Stmt stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt.s, nullptr) != SQLITE_OK) {
        throw std::runtime_error(
            std::string("Error al preparar la sentencia SQL: ") + sqlite3_errmsg(db_));
    }

    int inserted = 0;
    for (const auto& price : prices) {
        const auto bindText = [this, &stmt](int index, const std::string& value) {
            if (sqlite3_bind_text(stmt.s, index, value.c_str(), -1, SQLITE_TRANSIENT) != SQLITE_OK) {
                throw std::runtime_error(
                    std::string("Error al enlazar un campo SQL: ") + sqlite3_errmsg(db_));
            }
        };

        bindText(1, price.date);
        bindText(2, price.product);
        bindText(3, price.category);
        bindText(4, price.market);
        bindText(5, price.source);
        if (sqlite3_bind_double(stmt.s, 6, price.price_eur_kg) != SQLITE_OK) {
            throw std::runtime_error(
                std::string("Error al enlazar el precio: ") + sqlite3_errmsg(db_));
        }

        const int rc = sqlite3_step(stmt.s);
        if (rc != SQLITE_DONE) {
            throw std::runtime_error(
                std::string("Error al insertar el registro: ") + sqlite3_errmsg(db_));
        }

        // INSERT OR IGNORE deja 0 cambios cuando la fila ya existe.
        inserted += sqlite3_changes(db_);

        const int resetRc = sqlite3_reset(stmt.s);
        if (resetRc != SQLITE_OK) {
            throw std::runtime_error(
                std::string("Error al reiniciar la sentencia SQL: ") + sqlite3_errmsg(db_));
        }
        const int clearRc = sqlite3_clear_bindings(stmt.s);
        if (clearRc != SQLITE_OK) {
            throw std::runtime_error(
                std::string("Error al limpiar los parámetros SQL: ") + sqlite3_errmsg(db_));
        }
    }

    transaction.commit();
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

std::vector<PricePoint> Database::pricesByDateRange(const std::string& product,
                                                    const std::string& startDate,
                                                    const std::string& endDate) const {
    Stmt st;
    const char* sql =
        "SELECT date, product, category, market, price_eur_kg, source "
        "FROM prices WHERE product = ? AND date >= ? AND date <= ? "
        "ORDER BY date ASC"; // Orden cronológico para análisis
        
    if (sqlite3_prepare_v2(db_, sql, -1, &st.s, nullptr) != SQLITE_OK) {
        throw std::runtime_error(
            std::string("No se pudo preparar pricesByDateRange: ") + sqlite3_errmsg(db_));
    }
    
    sqlite3_bind_text(st.s, 1, product.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(st.s, 2, startDate.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(st.s, 3, endDate.c_str(), -1, SQLITE_TRANSIENT);

    std::vector<PricePoint> out;
    int rc = SQLITE_OK;
    while ((rc = sqlite3_step(st.s)) == SQLITE_ROW) {
        // Reutilizamos la función auxiliar col() del namespace anónimo
        out.push_back({col(st.s, 0), col(st.s, 1), col(st.s, 2), col(st.s, 3),
                       sqlite3_column_double(st.s, 4), col(st.s, 5)});
    }
    
    if (rc != SQLITE_DONE) {
        throw std::runtime_error(
            std::string("Error al consultar pricesByDateRange: ") + sqlite3_errmsg(db_));
    }
    return out;
}

} // namespace agro