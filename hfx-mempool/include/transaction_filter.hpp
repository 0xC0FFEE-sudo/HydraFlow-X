#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <unordered_set>
#include <unordered_map>
#include <atomic>
#include <mutex>
#include <regex>
#include <chrono>

namespace hfx {
namespace mempool {

// Forward declarations
struct Transaction;
struct ParsedIntent;
enum class IntentType;

// Filter types
enum class FilterType {
    ALLOWLIST = 0,
    BLOCKLIST,
    VALUE_RANGE,
    GAS_PRICE_RANGE,
    ADDRESS_PATTERN,
    TOKEN_FILTER,
    PROTOCOL_FILTER,
    INTENT_TYPE_FILTER,
    MEV_FILTER,
    TIME_BASED,
    CUSTOM_FUNCTION
};

// Filter criteria
struct FilterCriteria {
    FilterType type;
    std::string name;
    std::string description;
    bool enabled;
    uint32_t priority;
    
    // Value-based filters
    uint64_t min_value;
    uint64_t max_value;
    uint64_t min_gas_price;
    uint64_t max_gas_price;
    
    // Address filters
    std::unordered_set<std::string> allowed_addresses;
    std::unordered_set<std::string> blocked_addresses;
    std::regex address_pattern;
    bool case_sensitive;
    
    // Token filters
    std::unordered_set<std::string> allowed_tokens;
    std::unordered_set<std::string> blocked_tokens;
    uint64_t min_token_value_usd;
    uint64_t max_token_value_usd;
    
    // Protocol filters
    std::unordered_set<std::string> allowed_protocols;
    std::unordered_set<std::string> blocked_protocols;
    
    // Intent filters
    std::unordered_set<IntentType> allowed_intent_types;
    std::unordered_set<IntentType> blocked_intent_types;
    double min_confidence;
    
    // MEV filters
    bool allow_mev_opportunities;
    bool allow_sandwich_attacks;
    bool allow_frontrunning;
    bool allow_arbitrage;
    double min_mev_profit_usd;
    
    // Time-based filters
    std::chrono::system_clock::time_point start_time;
    std::chrono::system_clock::time_point end_time;
    uint32_t max_age_seconds;
    
    // Chain filters
    std::unordered_set<uint32_t> allowed_chains;
    std::unordered_set<uint32_t> blocked_chains;
    
    // Custom function filter
    std::function<bool(const Transaction&)> custom_filter;
    
    FilterCriteria() : type(FilterType::ALLOWLIST), enabled(true), priority(0),
                      min_value(0), max_value(UINT64_MAX), 
                      min_gas_price(0), max_gas_price(UINT64_MAX),
                      case_sensitive(false), min_token_value_usd(0), max_token_value_usd(UINT64_MAX),
                      min_confidence(0.0), allow_mev_opportunities(true),
                      allow_sandwich_attacks(true), allow_frontrunning(true),
                      allow_arbitrage(true), min_mev_profit_usd(0.0), max_age_seconds(3600) {}
};

// Filter rule combining multiple criteria
struct FilterRule {
    std::string name;
    std::string description;
    bool enabled;
    uint32_t priority;
    std::vector<FilterCriteria> criteria;
    
    // Logic operators
    enum class LogicOperator { AND, OR, NOT };
    LogicOperator logic_operator;
    
    // Actions
    enum class Action { PASS, BLOCK, MODIFY, LOG_ONLY };
    Action action;
    
    // Statistics
    std::atomic<uint64_t> matches{0};
    std::atomic<uint64_t> blocks{0};
    std::atomic<uint64_t> passes{0};
    std::chrono::system_clock::time_point last_match;
    
    FilterRule() : enabled(true), priority(0), logic_operator(LogicOperator::AND), 
                  action(Action::PASS) {}
};

// Filter configuration
struct FilterConfig {
    bool enable_parallel_filtering = true;
    uint32_t max_concurrent_filters = 8;
    uint32_t filter_timeout_ms = 100;
    bool enable_caching = true;
    uint32_t cache_size = 10000;
    uint32_t cache_ttl_seconds = 300;
    
    // Performance settings
    bool enable_bloom_filter = true;
    uint32_t bloom_filter_size = 1000000;
    double bloom_filter_error_rate = 0.01;
    
    // Monitoring
    bool enable_statistics = true;
    uint32_t statistics_interval_seconds = 60;
    bool log_filtered_transactions = false;
    bool log_performance_metrics = true;
};

// Filter statistics
struct FilterStats {
    std::atomic<uint64_t> total_processed{0};
    std::atomic<uint64_t> total_passed{0};
    std::atomic<uint64_t> total_blocked{0};
    std::atomic<uint64_t> cache_hits{0};
    std::atomic<uint64_t> cache_misses{0};
    std::atomic<double> avg_filter_time_ms{0.0};
    std::atomic<double> throughput_tps{0.0};
    std::chrono::system_clock::time_point last_reset;
};

// Filter result
struct FilterResult {
    bool passed;
    std::string reason;
    std::vector<std::string> matched_rules;
    std::vector<std::string> blocked_by_rules;
    double filter_time_ms;
    bool from_cache;
    
    FilterResult() : passed(false), filter_time_ms(0.0), from_cache(false) {}
};

// Main Transaction Filter Class
class TransactionFilter {
public:
    explicit TransactionFilter(const FilterConfig& config);
    ~TransactionFilter();

    // Core filtering functionality
    FilterResult filter_transaction(const Transaction& tx);
    std::vector<FilterResult> filter_batch(const std::vector<Transaction>& transactions);
    bool should_process(const Transaction& tx);
    
    // Async filtering
    using FilterCallback = std::function<void(const Transaction&, const FilterResult&)>;
    void filter_async(const Transaction& tx, FilterCallback callback);
    void filter_batch_async(const std::vector<Transaction>& transactions, FilterCallback callback);

    // Rule management
    void add_rule(const FilterRule& rule);
    void remove_rule(const std::string& name);
    void update_rule(const std::string& name, const FilterRule& rule);
    FilterRule get_rule(const std::string& name) const;
    std::vector<std::string> get_rule_names() const;
    void enable_rule(const std::string& name);
    void disable_rule(const std::string& name);

    // Criteria management
    void add_address_allowlist(const std::vector<std::string>& addresses);
    void add_address_blocklist(const std::vector<std::string>& addresses);
    void add_token_allowlist(const std::vector<std::string>& tokens);
    void add_token_blocklist(const std::vector<std::string>& tokens);
    void set_value_range(uint64_t min_value, uint64_t max_value);
    void set_gas_price_range(uint64_t min_gas, uint64_t max_gas);

    // Quick filters (common use cases)
    void enable_high_value_filter(uint64_t min_value_usd);
    void enable_mev_only_filter();
    void enable_dex_only_filter();
    void enable_protocol_filter(const std::vector<std::string>& protocols);
    void enable_chain_filter(const std::vector<uint32_t>& chain_ids);

    // Intent-based filtering
    void filter_by_intent_type(const std::vector<IntentType>& types, bool allow = true);
    void filter_by_confidence(double min_confidence);
    void filter_by_mev_profit(double min_profit_usd);

    // Configuration
    void update_config(const FilterConfig& config);
    FilterConfig get_config() const;
    void set_cache_enabled(bool enabled);
    void clear_cache();

    // Statistics and monitoring
    FilterStats get_statistics() const;
    void reset_statistics();
    std::vector<FilterRule> get_active_rules() const;
    std::unordered_map<std::string, uint64_t> get_rule_statistics() const;

    // Performance optimization
    void optimize_filters();
    void reorder_rules_by_performance();
    void enable_bloom_filter();
    void disable_bloom_filter();

    // Import/Export
    void export_rules(const std::string& filename) const;
    void import_rules(const std::string& filename);
    std::string serialize_rules() const;
    void deserialize_rules(const std::string& data);

    // Presets
    void load_memecoin_filters();
    void load_arbitrage_filters();
    void load_mev_filters();
    void load_high_frequency_filters();
    void reset_to_defaults();

private:
    FilterConfig config_;
    std::vector<FilterRule> rules_;
    mutable std::mutex rules_mutex_;
    mutable std::mutex config_mutex_;
    
    // Cache
    std::unordered_map<std::string, FilterResult> filter_cache_;
    mutable std::mutex cache_mutex_;
    
    // Bloom filter for fast lookups
    std::unique_ptr<class BloomFilter> bloom_filter_;
    
    // Statistics
    mutable FilterStats stats_;
    
    // Address/token sets for fast lookup
    std::unordered_set<std::string> global_address_allowlist_;
    std::unordered_set<std::string> global_address_blocklist_;
    std::unordered_set<std::string> global_token_allowlist_;
    std::unordered_set<std::string> global_token_blocklist_;
    mutable std::mutex lookup_sets_mutex_;
    
    // Internal filtering methods
    bool apply_rule(const Transaction& tx, const FilterRule& rule);
    bool apply_criteria(const Transaction& tx, const FilterCriteria& criteria);
    FilterResult create_filter_result(const Transaction& tx, bool passed, 
                                    const std::vector<std::string>& matched_rules,
                                    const std::vector<std::string>& blocked_rules,
                                    double filter_time_ms, bool from_cache);
    
    // Criteria evaluation
    bool evaluate_value_criteria(const Transaction& tx, const FilterCriteria& criteria);
    bool evaluate_address_criteria(const Transaction& tx, const FilterCriteria& criteria);
    bool evaluate_token_criteria(const Transaction& tx, const FilterCriteria& criteria);
    bool evaluate_protocol_criteria(const Transaction& tx, const FilterCriteria& criteria);
    bool evaluate_intent_criteria(const Transaction& tx, const FilterCriteria& criteria);
    bool evaluate_mev_criteria(const Transaction& tx, const FilterCriteria& criteria);
    bool evaluate_time_criteria(const Transaction& tx, const FilterCriteria& criteria);
    bool evaluate_chain_criteria(const Transaction& tx, const FilterCriteria& criteria);
    
    // Cache management
    std::string generate_cache_key(const Transaction& tx);
    void update_cache(const std::string& key, const FilterResult& result);
    std::optional<FilterResult> get_from_cache(const std::string& key);
    void cleanup_expired_cache();
    
    // Performance optimization
    void sort_rules_by_priority();
    void update_lookup_sets();
    bool quick_reject(const Transaction& tx);
    
    // Statistics update
    void update_statistics(const FilterResult& result, double processing_time_ms);
    void update_rule_statistics(const std::string& rule_name, bool matched, bool blocked);
    
    // Utility methods
    bool is_address_in_set(const std::string& address, const std::unordered_set<std::string>& set);
    bool matches_address_pattern(const std::string& address, const std::regex& pattern);
    uint64_t get_transaction_value_usd(const Transaction& tx);
    std::string normalize_address(const std::string& address);
};

// Utility functions
FilterRule create_value_filter(uint64_t min_value, uint64_t max_value);
FilterRule create_address_allowlist_filter(const std::vector<std::string>& addresses);
FilterRule create_address_blocklist_filter(const std::vector<std::string>& addresses);
FilterRule create_mev_filter(double min_profit_usd = 0.0);
FilterRule create_protocol_filter(const std::vector<std::string>& protocols, bool allow = true);
FilterRule create_intent_type_filter(const std::vector<IntentType>& types, bool allow = true);

std::string filter_type_to_string(FilterType type);
std::string filter_result_to_string(const FilterResult& result);
bool validate_filter_rule(const FilterRule& rule);

} // namespace mempool
} // namespace hfx
