#pragma once

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <optional>
#include <functional>
#include <regex>
#include <chrono>

namespace hfx {
namespace mempool {

// Forward declarations
struct Transaction;

// Intent types
enum class IntentType {
    UNKNOWN = 0,
    SWAP,
    ADD_LIQUIDITY,
    REMOVE_LIQUIDITY,
    ARBITRAGE,
    MEV_SANDWICH,
    MEV_FRONTRUN,
    MEV_BACKRUN,
    TOKEN_TRANSFER,
    NFT_TRADE,
    LENDING_BORROW,
    LENDING_REPAY,
    STAKING,
    UNSTAKING,
    GOVERNANCE_VOTE,
    BRIDGE_TRANSFER,
    LIQUIDATION,
    FLASH_LOAN,
    CUSTOM
};

// Token information
struct TokenInfo {
    std::string address;
    std::string symbol;
    std::string name;
    uint8_t decimals;
    uint64_t total_supply;
    bool is_verified;
    double price_usd;
    uint64_t market_cap;
    std::chrono::system_clock::time_point last_updated;
};

// Parsed intent structure
struct ParsedIntent {
    IntentType type;
    std::string description;
    double confidence_score;
    
    // Transaction details
    std::string transaction_hash;
    std::string from_address;
    std::string to_address;
    uint64_t value;
    uint64_t gas_price;
    uint32_t chain_id;
    
    // DEX/Trading specific
    std::string protocol;
    std::string pool_address;
    TokenInfo token_in;
    TokenInfo token_out;
    uint64_t amount_in;
    uint64_t amount_out;
    uint64_t min_amount_out;
    uint64_t max_slippage_bps;
    std::string router_address;
    
    // MEV specific
    bool is_mev_opportunity;
    double estimated_profit_usd;
    uint32_t block_delay;
    std::vector<std::string> related_transactions;
    
    // Timing
    std::chrono::system_clock::time_point timestamp;
    std::chrono::system_clock::time_point deadline;
    uint32_t priority_level;
    
    // Additional metadata
    std::unordered_map<std::string, std::string> metadata;
    
    ParsedIntent() : type(IntentType::UNKNOWN), confidence_score(0.0), 
                    value(0), gas_price(0), chain_id(0), amount_in(0), 
                    amount_out(0), min_amount_out(0), max_slippage_bps(0),
                    is_mev_opportunity(false), estimated_profit_usd(0.0),
                    block_delay(0), priority_level(0) {}
};

// Protocol definitions
struct ProtocolDefinition {
    std::string name;
    std::string version;
    std::vector<std::string> router_addresses;
    std::vector<std::string> factory_addresses;
    std::unordered_map<std::string, std::string> function_signatures;
    std::regex abi_pattern;
    IntentType default_intent_type;
    
    ProtocolDefinition() : default_intent_type(IntentType::UNKNOWN) {}
};

// Parser configuration
struct ParserConfig {
    bool enable_deep_parsing = true;
    bool enable_mev_detection = true;
    bool enable_price_analysis = true;
    bool cache_token_info = true;
    uint32_t max_analysis_depth = 5;
    double min_confidence_threshold = 0.7;
    uint32_t cache_expiry_minutes = 15;
    std::vector<uint32_t> supported_chains;
    std::unordered_map<std::string, ProtocolDefinition> known_protocols;
    
    // Performance settings
    uint32_t max_concurrent_parses = 10;
    uint32_t parsing_timeout_ms = 1000;
    bool enable_parallel_processing = true;
};

// Statistics
struct ParserStats {
    std::atomic<uint64_t> total_parsed{0};
    std::atomic<uint64_t> successful_parses{0};
    std::atomic<uint64_t> failed_parses{0};
    std::atomic<uint64_t> mev_detected{0};
    std::atomic<uint64_t> high_confidence_parses{0};
    std::atomic<double> avg_parsing_time_ms{0.0};
    std::atomic<double> avg_confidence_score{0.0};
    std::chrono::system_clock::time_point last_reset;
};

// Main Intent Parser Class
class IntentParser {
public:
    explicit IntentParser(const ParserConfig& config);
    ~IntentParser();

    // Core parsing functionality
    ParsedIntent parse_transaction(const Transaction& tx);
    std::vector<ParsedIntent> parse_batch(const std::vector<Transaction>& transactions);
    std::optional<ParsedIntent> try_parse(const Transaction& tx, double min_confidence = 0.5);

    // Async parsing
    using ParseCallback = std::function<void(const ParsedIntent&)>;
    void parse_async(const Transaction& tx, ParseCallback callback);
    void parse_batch_async(const std::vector<Transaction>& transactions, ParseCallback callback);

    // Intent type detection
    IntentType detect_intent_type(const Transaction& tx);
    double calculate_confidence(const Transaction& tx, IntentType type);
    bool is_likely_mev(const Transaction& tx);
    bool is_dex_interaction(const Transaction& tx);

    // Protocol management
    void register_protocol(const std::string& name, const ProtocolDefinition& definition);
    void unregister_protocol(const std::string& name);
    std::vector<std::string> get_supported_protocols() const;
    ProtocolDefinition get_protocol_definition(const std::string& name) const;

    // Token management
    void cache_token_info(const std::string& address, const TokenInfo& info);
    std::optional<TokenInfo> get_token_info(const std::string& address) const;
    void update_token_price(const std::string& address, double price_usd);
    void refresh_token_cache();

    // Configuration
    void update_config(const ParserConfig& config);
    ParserConfig get_config() const;
    void add_supported_chain(uint32_t chain_id);
    void remove_supported_chain(uint32_t chain_id);

    // Statistics and monitoring
    ParserStats get_statistics() const;
    void reset_statistics();
    double get_success_rate() const;
    double get_average_confidence() const;

    // Advanced features
    std::vector<ParsedIntent> find_arbitrage_opportunities(const std::vector<Transaction>& transactions);
    std::vector<ParsedIntent> detect_sandwich_attacks(const std::vector<Transaction>& transactions);
    std::vector<ParsedIntent> find_related_transactions(const ParsedIntent& intent);
    double estimate_mev_profit(const ParsedIntent& intent);

    // Validation and verification
    bool validate_intent(const ParsedIntent& intent);
    bool verify_transaction_data(const Transaction& tx);
    std::vector<std::string> check_intent_consistency(const ParsedIntent& intent);

private:
    ParserConfig config_;
    mutable std::mutex config_mutex_;
    
    // Caches
    std::unordered_map<std::string, TokenInfo> token_cache_;
    std::unordered_map<std::string, ProtocolDefinition> protocol_cache_;
    std::unordered_map<std::string, ParsedIntent> intent_cache_;
    mutable std::mutex cache_mutex_;
    
    // Statistics
    mutable ParserStats stats_;
    
    // Protocol parsers
    std::unordered_map<std::string, std::unique_ptr<class ProtocolParser>> protocol_parsers_;
    
    // ABI decoder
    std::unique_ptr<class ABIDecoder> abi_decoder_;
    
    // Price oracle
    std::unique_ptr<class PriceOracle> price_oracle_;
    
    // Internal parsing methods
    ParsedIntent parse_swap_transaction(const Transaction& tx);
    ParsedIntent parse_liquidity_transaction(const Transaction& tx);
    ParsedIntent parse_transfer_transaction(const Transaction& tx);
    ParsedIntent parse_contract_interaction(const Transaction& tx);
    
    // Protocol-specific parsers
    ParsedIntent parse_uniswap_transaction(const Transaction& tx);
    ParsedIntent parse_sushiswap_transaction(const Transaction& tx);
    ParsedIntent parse_pancakeswap_transaction(const Transaction& tx);
    ParsedIntent parse_1inch_transaction(const Transaction& tx);
    
    // MEV detection
    bool detect_frontrun_opportunity(const Transaction& tx);
    bool detect_sandwich_opportunity(const Transaction& tx);
    bool detect_arbitrage_opportunity(const Transaction& tx);
    double calculate_mev_profit(const Transaction& tx);
    
    // Data extraction
    std::string extract_function_selector(const Transaction& tx);
    std::vector<std::string> decode_function_parameters(const Transaction& tx);
    std::string identify_protocol(const Transaction& tx);
    
    // Token analysis
    TokenInfo fetch_token_info(const std::string& address, uint32_t chain_id);
    double get_token_price(const std::string& address, uint32_t chain_id);
    uint64_t calculate_token_amount(const std::string& raw_amount, uint8_t decimals);
    
    // Confidence calculation
    double calculate_swap_confidence(const Transaction& tx);
    double calculate_liquidity_confidence(const Transaction& tx);
    double calculate_mev_confidence(const Transaction& tx);
    
    // Utility methods
    void update_statistics(const ParsedIntent& intent, bool success, double parse_time_ms);
    void cleanup_expired_cache();
    bool is_supported_chain(uint32_t chain_id) const;
    std::string normalize_address(const std::string& address);
};

// Utility functions
std::string intent_type_to_string(IntentType type);
IntentType string_to_intent_type(const std::string& str);
std::string format_intent_description(const ParsedIntent& intent);
bool is_high_value_intent(const ParsedIntent& intent, double threshold_usd = 10000.0);
double calculate_impact_score(const ParsedIntent& intent);

} // namespace mempool
} // namespace hfx
