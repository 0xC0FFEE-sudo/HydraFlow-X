#pragma once

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <atomic>
#include <chrono>
#include <functional>

namespace hfx::ai {

struct ResearchPaper {
    std::string id;
    std::string title;
    std::string abstract;
    std::string authors;
    std::string url;
    std::string category;
    double relevance_score;
    std::chrono::system_clock::time_point published_date;
    std::vector<std::string> keywords;
    std::string strategy_insights;
};

struct TradingStrategy {
    std::string name;
    std::string description;
    std::vector<std::string> indicators;
    std::unordered_map<std::string, double> parameters;
    double backtested_sharpe;
    double win_rate;
    std::string source_paper_id;
    bool is_active;
    std::chrono::system_clock::time_point created_at;
    std::chrono::system_clock::time_point last_updated;
};

struct ResearchMetrics {
    std::atomic<uint64_t> papers_analyzed{0};
    std::atomic<uint64_t> strategies_generated{0};
    std::atomic<uint64_t> strategies_deployed{0};
    std::atomic<double> avg_strategy_performance{0.0};
    std::atomic<uint64_t> last_research_cycle_ms{0};
    
    // Copy constructor and assignment for atomic members
    ResearchMetrics(const ResearchMetrics& other) {
        papers_analyzed.store(other.papers_analyzed.load());
        strategies_generated.store(other.strategies_generated.load());
        strategies_deployed.store(other.strategies_deployed.load());
        avg_strategy_performance.store(other.avg_strategy_performance.load());
        last_research_cycle_ms.store(other.last_research_cycle_ms.load());
    }
    
    ResearchMetrics& operator=(const ResearchMetrics& other) {
        if (this != &other) {
            papers_analyzed.store(other.papers_analyzed.load());
            strategies_generated.store(other.strategies_generated.load());
            strategies_deployed.store(other.strategies_deployed.load());
            avg_strategy_performance.store(other.avg_strategy_performance.load());
            last_research_cycle_ms.store(other.last_research_cycle_ms.load());
        }
        return *this;
    }
    
    ResearchMetrics() = default;
    ResearchMetrics(ResearchMetrics&&) = delete;
    ResearchMetrics& operator=(ResearchMetrics&&) = delete;
};

class AutonomousResearchEngine {
private:
    struct Impl;
    std::unique_ptr<Impl> pimpl_;

public:
    AutonomousResearchEngine();
    ~AutonomousResearchEngine();

    // Core research functionality - inspired by Blackrock's Aladdin
    bool initialize();
    void start_research_cycle();
    void stop_research_cycle();
    
    // Paper discovery and analysis - like GMGN's smart money tracking
    std::vector<ResearchPaper> discover_latest_papers(const std::string& query = "");
    ResearchPaper analyze_paper_content(const std::string& paper_url);
    double calculate_relevance_score(const ResearchPaper& paper);
    
    // Strategy generation - inspired by fastest bot architectures
    TradingStrategy generate_strategy_from_paper(const ResearchPaper& paper);
    bool validate_strategy_logic(const TradingStrategy& strategy);
    double backtest_strategy(const TradingStrategy& strategy);
    
    // Strategy deployment and monitoring
    bool deploy_strategy(const TradingStrategy& strategy);
    void monitor_strategy_performance();
    void adapt_strategies_based_on_performance();
    
    // Real-time trend analysis - like Photon's market scanning
    std::vector<std::string> identify_emerging_trends();
    std::vector<std::string> analyze_market_sentiment_trends();
    void update_strategy_parameters_based_on_trends();
    
    // Advanced AI capabilities
    std::string summarize_paper_insights(const ResearchPaper& paper);
    std::vector<TradingStrategy> recommend_strategy_modifications();
    bool detect_market_regime_changes();
    
    // Metrics and monitoring
    ResearchMetrics get_research_metrics() const;
    std::vector<TradingStrategy> get_active_strategies() const;
    std::vector<ResearchPaper> get_recent_papers() const;
    
    // Configuration
    void set_research_categories(const std::vector<std::string>& categories);
    void set_min_relevance_threshold(double threshold);
    void set_research_frequency(std::chrono::milliseconds frequency);
};

// Advanced research algorithms inspired by fastest trading bots
class PaperAnalysisEngine {
public:
    static std::vector<std::string> extract_trading_signals(const std::string& abstract);
    static std::vector<std::string> identify_key_indicators(const std::string& content);
    static double calculate_implementation_feasibility(const ResearchPaper& paper);
    static std::string generate_strategy_pseudocode(const ResearchPaper& paper);
};

// Strategy adaptation system - inspired by BonkBot's MEV protection evolution
class StrategyEvolutionEngine {
public:
    static TradingStrategy mutate_strategy(const TradingStrategy& base_strategy);
    static std::vector<TradingStrategy> crossover_strategies(
        const TradingStrategy& strategy1, 
        const TradingStrategy& strategy2
    );
    static bool is_strategy_obsolete(const TradingStrategy& strategy);
    static TradingStrategy optimize_for_current_market(const TradingStrategy& strategy);
};

} // namespace hfx::ai
