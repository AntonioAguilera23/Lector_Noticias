#include "analysis/MarketTrendAnalyzer.h"
#include <numeric>
#include <cmath>
#include <algorithm>
#include <stdexcept>
#include <vector>

namespace agro {

namespace {
    // --- CONSTANTES DE ANÁLISIS BÁSICO ---
    constexpr int DEFAULT_ANALYSIS_PERIODS = 4;

    // --- CONSTANTES DEL MERCADO AGRÍCOLA (CAPM) ---
    constexpr double HISTORICAL_AGRI_MARKET_RETURN = 0.048; // 4.8% retorno histórico del sector
    constexpr double HISTORICAL_RISK_FREE_RATE = 0.0426;    // 4.26% bonos del tesoro (tasa segura)

    // Función auxiliar para calcular Beta dinámicamente
    double calcularBetaAgricola(const std::vector<double>& retornosProducto, const std::vector<double>& retornosIndice) {
        if (retornosProducto.size() != retornosIndice.size() || retornosProducto.empty()) {
            throw std::invalid_argument("Los historicos deben tener la misma cantidad de registros N.");
        }

        size_t n = retornosProducto.size();
        double sumaProducto = 0.0;
        double sumaIndice = 0.0;

        for (size_t i = 0; i < n; ++i) {
            sumaProducto += retornosProducto[i];
            sumaIndice += retornosIndice[i];
        }
        double mediaProducto = sumaProducto / n;
        double mediaIndice = sumaIndice / n;

        double covarianza = 0.0;
        double varianzaIndice = 0.0;

        for (size_t i = 0; i < n; ++i) {
            double difProducto = retornosProducto[i] - mediaProducto;
            double difIndice = retornosIndice[i] - mediaIndice;
            
            covarianza += difProducto * difIndice;
            varianzaIndice += difIndice * difIndice;
        }

        if (varianzaIndice == 0.0) return 0.0; 
        return covarianza / varianzaIndice;
    }

    // Función auxiliar para calcular el Alpha
    double calcularAlphaAgricola(double beta, double retornoRealAnual) {
        double primaDeRiesgo = HISTORICAL_AGRI_MARKET_RETURN - HISTORICAL_RISK_FREE_RATE;
        double retornoEsperado = HISTORICAL_RISK_FREE_RATE + (beta * primaDeRiesgo);
        return retornoRealAnual - retornoEsperado;
    }
}

MarketTrendAnalyzer::MarketTrendAnalyzer(std::shared_ptr<IPriceRepository> repo) 
    : repo_(std::move(repo)) {}

// ==========================================
// TENDENCIA BÁSICA
// ==========================================
std::optional<TrendReport> MarketTrendAnalyzer::analyzeRecentTrend(const std::string& product, 
                                                                   const std::string& category) const {
    return analyzeRecentTrend(product, category, DEFAULT_ANALYSIS_PERIODS);
}

std::optional<TrendReport> MarketTrendAnalyzer::analyzeRecentTrend(const std::string& product, 
                                                                   const std::string& category, 
                                                                   int periods) const {
    if (!repo_ || periods < 2) return std::nullopt;

    auto history = repo_->getRecentHistory(product, category, periods);
    
    if (history.size() < 2) {
        return std::nullopt; 
    }

    double current = history[0].price_eur_kg;
    double previous = history[1].price_eur_kg;
    
    double variation = ((current - previous) / previous) * 100.0;

    std::string direction = "ESTABLE";
    if (variation > 0.5) direction = "ALCISTA";
    else if (variation < -0.5) direction = "BAJISTA";

    double sum = 0.0;
    for (const auto& point : history) {
        sum += point.price_eur_kg;
    }
    double moving_avg = sum / history.size();

    return TrendReport{
        product,
        current,
        previous,
        std::round(variation * 100.0) / 100.0, 
        std::round(moving_avg * 100.0) / 100.0,
        direction
    };
}

// ==========================================
// PREVISIÓN HÍBRIDA (BOLLINGER + HOLT)
// ==========================================
std::optional<HybridForecastReport> MarketTrendAnalyzer::generateHybridForecast(const std::string& product, 
                                                                                const std::string& category, 
                                                                                int periods) const {
    if (!repo_ || periods < 5) return std::nullopt;

    auto history = repo_->getRecentHistory(product, category, periods);
    
    if (history.size() < static_cast<size_t>(periods)) {
        return std::nullopt; 
    }

    double current_price = history.front().price_eur_kg;

    // FASE 1: BANDAS DE BOLLINGER (Volatilidad)
    double sum = 0.0;
    for (const auto& point : history) {
        sum += point.price_eur_kg;
    }
    double sma = sum / history.size(); 

    double variance_sum = 0.0;
    for (const auto& point : history) {
        variance_sum += std::pow(point.price_eur_kg - sma, 2);
    }
    double std_dev = std::sqrt(variance_sum / history.size()); 

    double upper_band = sma + (2 * std_dev);
    double lower_band = sma - (2 * std_dev);

    // FASE 2: MODELO DE HOLT (Doble Suavizado Exponencial)
    const double alpha = 0.3; 
    const double beta = 0.2;  

    auto it = history.rbegin();
    double level = it->price_eur_kg; 
    double trend = (std::next(it)->price_eur_kg - level); 

    for (++it; it != history.rend(); ++it) {
        double current_y = it->price_eur_kg;
        double last_level = level;
        
        level = alpha * current_y + (1.0 - alpha) * (last_level + trend);
        trend = beta * (level - last_level) + (1.0 - beta) * trend;
    }

    double forecast = level + trend;

    // FASE 3: MOTOR DE DECISIONES DE NEGOCIO
    std::string recommendation = "MANTENER";
    std::string explanation = "El precio fluctúa dentro de los niveles normales de volatilidad histórica.";

    if (current_price > upper_band) {
        if (forecast < current_price) {
            recommendation = "VENDER (Fuerte)";
            explanation = "Anomalía alcista detectada. El precio actual rompió el máximo estadístico y el modelo proyecta caídas a corto plazo.";
        } else {
            recommendation = "VENDER";
            explanation = "Mercado sobrecomprado. Buen momento para liquidar stock aunque la tendencia mantenga cierta inercia alcista.";
        }
    } else if (current_price < lower_band) {
        if (forecast > current_price) {
            recommendation = "ACUMULAR / PRODUCIR (Fuerte)";
            explanation = "Oportunidad de mercado. El precio es anormalmente bajo y se proyecta un rebote alcista inminente.";
        } else {
            recommendation = "MANTENER";
            explanation = "Precio hundido pero la tendencia sigue siendo negativa. Mantener posiciones a la espera de un cambio de ciclo.";
        }
    } else {
        if (trend > 0.05) { 
            explanation = "Mercado estable con inercia alcista clara. Favorable para mantener stock y esperar mejor precio.";
        } else if (trend < -0.05) {
            explanation = "Mercado estable pero con tendencia a la baja. Vigilar de cerca los márgenes de rentabilidad.";
        }
    }

    return HybridForecastReport{
        product,
        current_price,
        std::round(sma * 1000.0) / 1000.0,
        std::round(upper_band * 1000.0) / 1000.0,
        std::round(lower_band * 1000.0) / 1000.0,
        std::round(forecast * 1000.0) / 1000.0,
        std::round(trend * 1000.0) / 1000.0,
        recommendation,
        explanation
    };
}

} // namespace agro