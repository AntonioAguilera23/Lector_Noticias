#pragma once

#include "agro/PricePoint.h"
#include "storage/Database.h" 
#include <vector>
#include <string>
#include <memory>

namespace agro {

/**
 * @brief Interfaz para el repositorio de precios.
 * Desacopla la lógica de negocio (MarketTrendAnalyzer, etc.) de la tecnología 
 * de base de datos subyacente.
 */
class IPriceRepository {
public:
    virtual ~IPriceRepository() = default;

    // Guarda un lote de precios (ej. provenientes del MapaBulletinImporter)
    // Devuelve el número de registros insertados correctamente.
    virtual int save(const std::vector<PricePoint>& prices) = 0;

    // Recupera los últimos precios de un producto para análisis rápido
    virtual std::vector<PricePoint> getRecentHistory(const std::string& product, 
                                                     const std::string& category, 
                                                     int limit = 10) const = 0;

    // TODO: Métodos adicionales que tu analizador de tendencias necesitará
    virtual std::vector<PricePoint> getByDateRange(const std::string& product,
                                                   const std::string& startDate,
                                                   const std::string& endDate) const = 0;
};

private:
    std::shared_ptr<Database> db_;
};

} // namespace agro