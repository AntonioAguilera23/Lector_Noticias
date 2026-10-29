#pragma once

#include "storage/IPriceRepository.h" // Corregido al nombre e interfaz correctos acordados[cite: 4]
#include <string>
#include <vector>
#include <memory>
#include <optional>

namespace agro {

// Estructura original para el análisis de tendencia básico[cite: 4]
struct TrendReport {
    std::string product;
    double current_price;
    double previous_price;
    double percentage_change;
    double moving_average; 
    std::string trend_direction; 
};

// Nueva estructura para empaquetar los resultados del análisis híbrido
struct HybridForecastReport {
    std::string product;
    double current_price;
    
    // Métricas Bollinger (Volatilidad y extremos)
    double bollinger_sma;
    double bollinger_upper;
    double bollinger_lower;
    
    // Métricas de Holt (Tendencia)
    double forecasted_price;
    double trend_strength;
    
    // Decisión de negocio
    std::string recommendation;
    std::string explanation;
};

class MarketTrendAnalyzer {
public:
    // Inyección de dependencias mediante la interfaz[cite: 4]
    explicit MarketTrendAnalyzer(std::shared_ptr<IPriceRepository> repo);

    // ==========================================
    // TENDENCIA BÁSICA
    // ==========================================
    // 1. Sobrecarga que usa el número de periodos por defecto interno (DEFAULT_ANALYSIS_PERIODS = 4)[cite: 4]
    std::optional<TrendReport> analyzeRecentTrend(const std::string& product, 
                                                  const std::string& category) const;

    // 2. Método que permite especificar los periodos exactos[cite: 4]
    std::optional<TrendReport> analyzeRecentTrend(const std::string& product, 
                                                  const std::string& category, 
                                                  int periods) const;

    // ==========================================
    // PREVISIÓN HÍBRIDA (BOLLINGER + HOLT)
    // ==========================================
    // Genera el informe predictivo. Por defecto usa 20 periodos para garantizar 
    // que la desviación estándar y el suavizado de Holt tengan rigor estadístico.
    std::optional<HybridForecastReport> generateHybridForecast(const std::string& product, 
                                                               const std::string& category, 
                                                               int periods = 20) const;

private:
    std::shared_ptr<IPriceRepository> repo_; //[cite: 4]
};

} // namespace agro