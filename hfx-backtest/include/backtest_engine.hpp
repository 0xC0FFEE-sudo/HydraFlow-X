#pragma once

#include "backtest_data.hpp"
#include "trading_strategy.hpp"
#include <memory>
#include <vector>
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>

namespace hfx {
namespace backtest {

// Backtesting engine configuration
struct BacktestEngineConfig {
    bool enable_parallel_processing;
    int max_threads;
    bool enable_progress_reporting;
    std::chrono::milliseconds progress_report_interval;
    bool enable_detailed_logging;
    bool save_intermediate_results;
    std::string results_directory;

    BacktestEngineConfig() : enable_parallel_processing(true),
                            max_threads(std::thread::hardware_concurrency()),
                            enable_progress_reporting(true),
                            progress_report_interval(std::chrono::seconds(5)),
                            enable_detailed_logging(false),
                            save_intermediate_results(false) {}
};

// Progress callback
using ProgressCallback = std::function<void(double progress, const std::string& message)>;

// Backtesting engine class
class BacktestEngine {
public:
    explicit BacktestEngine(const BacktestEngineConfig& config = BacktestEngineConfig());
    ~BacktestEngine();

    // Setup methods
    void set_data_source(std::unique_ptr<IDataSource> data_source);
    void add_strategy(std::unique_ptr<ITradingStrategy> strategy);
    void set_config(const BacktestConfig& config);

    // Execution methods
    BacktestResult run_backtest();
    BacktestResult run_backtest_async(ProgressCallback progress_callback = nullptr);

    // Multi-strategy testing
    std::vector<BacktestResult> run_multiple_strategies(
        const std::vector<std::unique_ptr<ITradingStrategy>>& strategies);

    // Parameter optimization
    struct ParameterRange {
        std::string parameter_name;
        double min_value;
        double max_value;
        double step_size;

        ParameterRange() : min_value(0.0), max_value(0.0), step_size(1.0) {}
    };

    BacktestResult optimize_strategy_parameters(
        std::unique_ptr<ITradingStrategy> strategy_template,
        const std::vector<ParameterRange>& parameter_ranges,
        std::function<double(const BacktestResult&)> fitness_function =
            [](const BacktestResult& result) { return result.metrics.sharpe_ratio; });

    // Walk-forward analysis
    struct WalkForwardConfig {
        std::chrono::days training_window;
        std::chrono::days testing_window;
        std::chrono::days step_size;
        bool enable_parameter_optimization;

        WalkForwardConfig() : training_window(std::chrono::days(365)),
                             testing_window(std::chrono::days(30)),
                             step_size(std::chrono::days(30)),
                             enable_parameter_optimization(false) {}
    };

    std::vector<BacktestResult> run_walk_forward_analysis(
        std::unique_ptr<ITradingStrategy> strategy,
        const WalkForwardConfig& config);

    // Monte Carlo simulation
    struct MonteCarloConfig {
        int num_simulations;
        bool randomize_entry_timing;
        bool randomize_exit_timing;
        bool randomize_transaction_costs;
        std::vector<double> return_scenarios;

        MonteCarloConfig() : num_simulations(1000),
                            randomize_entry_timing(false),
                            randomize_exit_timing(false),
                            randomize_transaction_costs(false) {}
    };

    std::vector<BacktestResult> run_monte_carlo_analysis(
        std::unique_ptr<ITradingStrategy> strategy,
        const MonteCarloConfig& config);

    // Results analysis
    static void compare_results(const std::vector<BacktestResult>& results);
    static BacktestResult find_best_result(const std::vector<BacktestResult>& results,
                                         std::function<double(const BacktestResult&)> metric =
                                             [](const BacktestResult& r) { return r.metrics.sharpe_ratio; });

    // Export methods
    static void export_results_to_csv(const BacktestResult& result, const std::string& filename);
    static void export_results_to_json(const BacktestResult& result, const std::string& filename);
    static void export_equity_curve(const BacktestResult& result, const std::string& filename);

    // Validation methods
    static bool validate_backtest_config(const BacktestConfig& config);
    static bool validate_strategy_parameters(const StrategyParameters& params);

private:
    BacktestEngineConfig engine_config_;
    BacktestConfig backtest_config_;
    std::unique_ptr<IDataSource> data_source_;
    std::vector<std::unique_ptr<ITradingStrategy>> strategies_;

    // Execution state
    std::atomic<bool> is_running_;
    std::mutex execution_mutex_;
    std::condition_variable execution_cv_;

    // Internal methods
    BacktestResult execute_backtest(ITradingStrategy* strategy,
                                   const BacktestConfig& config,
                                   ProgressCallback progress_callback = nullptr);

    void initialize_portfolio(PortfolioSnapshot& portfolio, const BacktestConfig& config);
    void update_portfolio(PortfolioSnapshot& portfolio, const MarketData& data);
    std::vector<Order> process_signals(const std::vector<TradingSignal>& signals,
                                     PortfolioSnapshot& portfolio,
                                     const MarketData& data);
    void execute_orders(std::vector<Order>& orders, PortfolioSnapshot& portfolio,
                       const MarketData& data, std::vector<Trade>& trades,
                       const BacktestConfig& config);
    void calculate_performance_metrics(BacktestResult& result);
    void calculate_monthly_performance(BacktestResult& result);

    // Slippage and commission calculations
    double calculate_slippage(const Order& order, const MarketData& data, const BacktestConfig& config);
    double calculate_commission(const Order& order, const BacktestConfig& config);
    double apply_market_impact(double price, double volume, const BacktestConfig& config);

    // Utility methods
    std::vector<MarketData> load_market_data(const std::string& symbol,
                                           const BacktestConfig& config);
    void validate_data_availability(const BacktestConfig& config);
    std::chrono::milliseconds estimate_execution_time(const BacktestConfig& config);

    // Progress reporting
    void report_progress(double progress, const std::string& message,
                        ProgressCallback callback = nullptr);

    // Threading utilities
    void run_parallel_backtests(const std::vector<std::unique_ptr<ITradingStrategy>>& strategies,
                               std::vector<BacktestResult>& results,
                               ProgressCallback progress_callback = nullptr);
};

// Strategy performance comparison
struct StrategyComparison {
    std::string strategy_name;
    double total_return;
    double annualized_return;
    double sharpe_ratio;
    double max_drawdown;
    double win_rate;
    int total_trades;
    double profit_factor;

    StrategyComparison() : total_return(0.0), annualized_return(0.0), sharpe_ratio(0.0),
                          max_drawdown(0.0), win_rate(0.0), total_trades(0), profit_factor(0.0) {}
};

class BacktestAnalyzer {
public:
    static std::vector<StrategyComparison> compare_strategies(
        const std::vector<BacktestResult>& results);

    static void generate_performance_report(
        const std::vector<BacktestResult>& results,
        const std::string& output_file);

    static void generate_risk_report(
        const std::vector<BacktestResult>& results,
        const std::string& output_file);

    static void generate_trade_analysis_report(
        const std::vector<BacktestResult>& results,
        const std::string& output_file);

    static void plot_equity_curves(
        const std::vector<BacktestResult>& results,
        const std::string& output_file);

    // Statistical tests
    static double calculate_t_statistic(const BacktestResult& result1, const BacktestResult& result2);
    static bool perform_significance_test(const BacktestResult& result1, const BacktestResult& result2,
                                        double confidence_level = 0.95);

    // Benchmark comparisons
    static void compare_with_benchmark(
        const BacktestResult& strategy_result,
        const std::vector<MarketData>& benchmark_data,
        const std::string& output_file);
};

// Walk-forward analysis result
struct WalkForwardResult {
    std::vector<BacktestResult> training_results;
    std::vector<BacktestResult> testing_results;
    double overall_performance;
    double consistency_score; // How consistent the strategy is across periods
    std::vector<double> rolling_sharpe_ratios;

    WalkForwardResult() : overall_performance(0.0), consistency_score(0.0) {}
};

// Parameter optimization result
struct OptimizationResult {
    std::vector<std::pair<StrategyParameters, double>> parameter_combinations;
    StrategyParameters best_parameters;
    double best_fitness_score;
    std::chrono::milliseconds optimization_time;
    int total_combinations_tested;

    OptimizationResult() : best_fitness_score(0.0), optimization_time(0),
                          total_combinations_tested(0) {}
};

} // namespace backtest
} // namespace hfx
