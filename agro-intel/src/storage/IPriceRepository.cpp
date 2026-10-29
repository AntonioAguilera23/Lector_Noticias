#include "storage/PriceRepository.h" // Ajusta el path según tu estructura de carpetas

namespace agro {

// Constructor
SqlitePriceRepository::SqlitePriceRepository(std::shared_ptr<Database> db) 
    : db_(std::move(db)) {}

int SqlitePriceRepository::save(const std::vector<PricePoint>& prices) {
    if (!db_) return 0;
    return db_->insertPrices(prices);
}

std::vector<PricePoint> SqlitePriceRepository::getRecentHistory(const std::string& product, 
                                                                const std::string& category, 
                                                                int limit) const {
    if (!db_) return {};
    return db_->latestPrices(product, category, limit);
}

std::vector<PricePoint> SqlitePriceRepository::getByDateRange(const std::string& product,
                                                              const std::string& startDate,
                                                              const std::string& endDate) const {
    if (!db_) return {};
    return db_->pricesByDateRange(product, startDate, endDate); 
}

} // namespace agro