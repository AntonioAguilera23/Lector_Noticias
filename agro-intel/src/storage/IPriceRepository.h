#pragma once
#include "agro/PricePoint.h"
#include "storage/Database.h"
#include <vector>
#include <string>
#include <memory>

namespace agro {

// 1. Interfaz Pura
class IPriceRepository {
public:
    virtual ~IPriceRepository() = default;
    virtual int save(const std::vector<PricePoint>& prices) = 0;
    virtual std::vector<PricePoint> getRecentHistory(const std::string& product, 
                                                     const std::string& category, 
                                                     int limit) const = 0;
    virtual std::vector<PricePoint> getByDateRange(const std::string& product, 
                                                   const std::string& start_date, 
                                                   const std::string& end_date) const = 0;
};

// 2. Implementación Concreta
class SqlitePriceRepository : public IPriceRepository {
public:
    explicit SqlitePriceRepository(std::shared_ptr<Database> db);

    int save(const std::vector<PricePoint>& prices) override;
    std::vector<PricePoint> getRecentHistory(const std::string& product, 
                                             const std::string& category, 
                                             int limit) const override;
    std::vector<PricePoint> getByDateRange(const std::string& product, 
                                           const std::string& start_date, 
                                           const std::string& end_date) const override;
private:
    std::shared_ptr<Database> db_; // Ahora sí está dentro de una clase válida
};

} // namespace agro