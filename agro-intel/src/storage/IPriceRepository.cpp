#include "storage/IPriceRepository.h"

#include <stdexcept>
#include <utility>

namespace agro {

SqlitePriceRepository::SqlitePriceRepository(std::shared_ptr<Database> db)
    : db_(std::move(db)) {
    if (!db_) {
        throw std::invalid_argument("SqlitePriceRepository requiere una base de datos válida");
    }
}

int SqlitePriceRepository::save(const std::vector<PricePoint>& prices) {
    return db_->insertPrices(prices);
}

std::vector<PricePoint> SqlitePriceRepository::getRecentHistory(
    const std::string& product,
    const std::string& category,
    int limit) const {
    return db_->latestPrices(product, category, limit);
}

std::vector<PricePoint> SqlitePriceRepository::getByDateRange(
    const std::string& product,
    const std::string& start_date,
    const std::string& end_date) const {
    return db_->pricesByDateRange(product, start_date, end_date);
}

} // namespace agro
