#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <atomic>
#include <mutex>
#include <thread>
#include <queue>
#include <chrono>
#include <unordered_map>
#include <unordered_set>

namespace hfx {
namespace mempool {

// Forward declarations
struct Transaction;
struct BlockInfo;

// Transaction structure
struct Transaction {
    std::string hash;
    std::string from;
    std::string to;
    uint64_t value;
    uint64_t gas_price;
    uint64_t gas_limit;
    std::string data;
    uint64_t nonce;
    std::chrono::system_clock::time_point timestamp;
    uint32_t chain_id;
    bool is_contract_call;
    
    // MEV related fields
    bool is_dex_trade;
    bool is_arbitrage;
    bool is_sandwich_attack;
    std::string pool_address;
    std::string token_in;
    std::string token_out;
    uint64_t amount_in;
    uint64_t amount_out;
    
    Transaction() = default;
    Transaction(const Transaction& other);
    Transaction& operator=(const Transaction& other);
};

// Block information
struct BlockInfo {
    uint64_t number;
    std::string hash;
    std::string parent_hash;
    uint64_t timestamp;
    uint64_t base_fee;
    uint64_t gas_limit;
    uint64_t gas_used;
    std::vector<std::string> transaction_hashes;
};

// Mempool statistics
struct MempoolStats {
    std::atomic<uint64_t> total_transactions{0};
    std::atomic<uint64_t> pending_transactions{0};
    std::atomic<uint64_t> dex_transactions{0};
    std::atomic<uint64_t> mev_opportunities{0};
    std::atomic<uint64_t> failed_transactions{0};
    std::atomic<double> avg_gas_price{0.0};
    std::atomic<double> avg_processing_time_ms{0.0};
    std::chrono::system_clock::time_point last_update;
};

// Transaction filter callback
using TransactionCallback = std::function<void(const Transaction&)>;
using BlockCallback = std::function<void(const BlockInfo&)>;

// Mempool Monitor Configuration
struct MempoolConfig {
    std::vector<std::string> rpc_endpoints;
    std::vector<uint32_t> chain_ids;
    bool monitor_pending = true;
    bool monitor_confirmed = true;
    bool filter_dex_transactions = true;
    bool filter_mev_opportunities = true;
    uint32_t max_concurrent_connections = 10;
    uint32_t polling_interval_ms = 100;
    uint32_t batch_size = 1000;
    uint32_t max_mempool_size = 100000;
    bool enable_websocket = true;
    bool enable_http_polling = false;
    std::unordered_set<std::string> watched_addresses;
    std::unordered_set<std::string> watched_tokens;
    uint64_t min_value_threshold = 0;
    uint64_t max_gas_price = 1000000000000; // 1000 gwei
};

// Main Mempool Monitor Class
class MempoolMonitor {
public:
    explicit MempoolMonitor(const MempoolConfig& config);
    ~MempoolMonitor();

    // Lifecycle management
    bool start();
    void stop();
    bool is_running() const;

    // Callback registration
    void register_transaction_callback(TransactionCallback callback);
    void register_block_callback(BlockCallback callback);
    void register_mev_callback(TransactionCallback callback);

    // Transaction management
    void add_transaction(const Transaction& tx);
    void remove_transaction(const std::string& hash);
    bool has_transaction(const std::string& hash) const;
    Transaction get_transaction(const std::string& hash) const;
    std::vector<Transaction> get_pending_transactions() const;
    std::vector<Transaction> get_transactions_by_address(const std::string& address) const;

    // Filtering and querying
    std::vector<Transaction> filter_transactions(
        std::function<bool(const Transaction&)> predicate) const;
    std::vector<Transaction> get_dex_transactions() const;
    std::vector<Transaction> get_high_value_transactions(uint64_t min_value) const;
    std::vector<Transaction> get_transactions_by_gas_price(uint64_t min_gas_price) const;

    // Statistics and monitoring
    MempoolStats get_statistics() const;
    void reset_statistics();
    uint64_t get_pending_count() const;
    uint64_t get_confirmed_count() const;
    double get_average_gas_price() const;

    // Configuration management
    void update_config(const MempoolConfig& config);
    MempoolConfig get_config() const;
    void add_watched_address(const std::string& address);
    void remove_watched_address(const std::string& address);
    void add_watched_token(const std::string& token);
    void remove_watched_token(const std::string& token);

    // Chain-specific operations
    void add_chain(uint32_t chain_id, const std::string& rpc_endpoint);
    void remove_chain(uint32_t chain_id);
    std::vector<uint32_t> get_monitored_chains() const;

    // Advanced features
    void enable_real_time_mode();
    void disable_real_time_mode();
    void set_max_latency_ms(uint32_t max_latency);
    void enable_priority_queue();
    void disable_priority_queue();

private:
    MempoolConfig config_;
    std::atomic<bool> running_;
    std::atomic<bool> real_time_mode_;
    
    // Thread management
    std::vector<std::thread> worker_threads_;
    std::vector<std::thread> monitor_threads_;
    std::thread statistics_thread_;
    
    // Data storage
    std::unordered_map<std::string, Transaction> pending_transactions_;
    std::unordered_map<std::string, Transaction> confirmed_transactions_;
    std::queue<Transaction> priority_queue_;
    
    // Synchronization
    mutable std::mutex transactions_mutex_;
    mutable std::mutex config_mutex_;
    mutable std::mutex callbacks_mutex_;
    
    // Callbacks
    std::vector<TransactionCallback> transaction_callbacks_;
    std::vector<BlockCallback> block_callbacks_;
    std::vector<TransactionCallback> mev_callbacks_;
    
    // Statistics
    mutable MempoolStats stats_;
    
    // Connection management
    std::unordered_map<uint32_t, std::string> chain_endpoints_;
    std::unordered_map<uint32_t, std::unique_ptr<class ChainConnection>> connections_;
    
    // Internal methods
    void monitor_chain_worker(uint32_t chain_id);
    void process_transaction_batch();
    void update_statistics();
    void handle_new_transaction(const Transaction& tx);
    void handle_confirmed_transaction(const Transaction& tx);
    void handle_new_block(const BlockInfo& block);
    void notify_transaction_callbacks(const Transaction& tx);
    void notify_block_callbacks(const BlockInfo& block);
    void notify_mev_callbacks(const Transaction& tx);
    
    // Chain interaction
    std::vector<Transaction> fetch_pending_transactions(uint32_t chain_id);
    std::vector<Transaction> fetch_block_transactions(uint32_t chain_id, uint64_t block_number);
    BlockInfo fetch_block_info(uint32_t chain_id, uint64_t block_number);
    
    // Transaction parsing
    Transaction parse_raw_transaction(const std::string& raw_tx, uint32_t chain_id);
    bool is_dex_transaction(const Transaction& tx);
    bool is_mev_opportunity(const Transaction& tx);
    
    // Performance optimization
    void cleanup_old_transactions();
    void optimize_data_structures();
    bool should_process_transaction(const Transaction& tx);
};

// Utility functions
std::string transaction_to_string(const Transaction& tx);
std::string calculate_transaction_hash(const Transaction& tx);
bool is_address_contract(const std::string& address, uint32_t chain_id);
uint64_t estimate_confirmation_time(const Transaction& tx);
double calculate_transaction_priority(const Transaction& tx);

} // namespace mempool
} // namespace hfx
