/**
 * @file ethereum_client.hpp
 * @brief High-performance Ethereum RPC client with WebSocket support
 * @version 1.0.0
 */

#pragma once

#include <string>
#include <memory>
#include <vector>
#include <functional>
#include <atomic>
#include <chrono>
#include <optional>
#include <unordered_map>
#include <mutex>

namespace hfx::chain {

struct EthereumBlock {
    uint64_t number;
    std::string hash;
    std::string parent_hash;
    uint64_t timestamp;
    uint64_t gas_limit;
    uint64_t gas_used;
    double base_fee_per_gas;
    std::vector<std::string> transaction_hashes;
    size_t transaction_count;
    std::string miner;
    uint64_t difficulty;
    std::string mix_hash;
};

struct EthereumTransaction {
    std::string hash;
    std::string from;
    std::string to;
    std::string value;
    uint64_t gas;
    std::string gas_price;
    std::string max_fee_per_gas;
    std::string max_priority_fee_per_gas;
    uint64_t nonce;
    std::string data;
    uint64_t block_number;
    uint32_t transaction_index;
    uint8_t transaction_type;  // 0=legacy, 1=EIP-2930, 2=EIP-1559
};

struct GasEstimate {
    std::string gas_limit;
    std::string gas_price;
    std::string max_fee_per_gas;
    std::string max_priority_fee_per_gas;
    std::string base_fee;
    double confidence_level;
    std::chrono::milliseconds estimated_time;
};

struct EthereumConfig {
    std::string rpc_url = "https://eth-mainnet.g.alchemy.com/v2/YOUR_API_KEY";
    std::string ws_url = "wss://eth-mainnet.g.alchemy.com/v2/YOUR_API_KEY";
    std::string api_key;
    std::chrono::milliseconds request_timeout = std::chrono::milliseconds(5000);
    std::chrono::milliseconds ws_ping_interval = std::chrono::seconds(30);
    size_t max_concurrent_requests = 10;
    size_t max_retries = 3;
    std::chrono::milliseconds retry_delay = std::chrono::milliseconds(100);
    bool enable_websocket = true;
    bool enable_mempool_monitoring = true;
    uint64_t chain_id = 1; // Mainnet
};

class EthereumClient {
public:
    // Callback types
    using BlockCallback = std::function<void(const EthereumBlock&)>;
    using TransactionCallback = std::function<void(const EthereumTransaction&)>;
    using PendingTransactionCallback = std::function<void(const std::string&)>;
    using LogCallback = std::function<void(const std::string&)>;
    using DisconnectCallback = std::function<void()>;

    explicit EthereumClient(const EthereumConfig& config);
    ~EthereumClient();

    // Connection management
    bool connect();
    void disconnect();
    bool is_connected() const;
    bool is_websocket_connected() const;

    // Block operations
    std::optional<EthereumBlock> get_latest_block();
    std::optional<EthereumBlock> get_block_by_number(uint64_t block_number);
    std::optional<EthereumBlock> get_block_by_hash(const std::string& block_hash);
    uint64_t get_block_number();

    // Transaction operations
    std::optional<EthereumTransaction> get_transaction(const std::string& tx_hash);
    std::string send_raw_transaction(const std::string& signed_tx);
    GasEstimate estimate_gas(const std::string& from, const std::string& to, 
                           const std::string& data = "", const std::string& value = "0x0");
    
    // Account operations
    std::string get_balance(const std::string& address);
    uint64_t get_transaction_count(const std::string& address);
    std::string get_code(const std::string& address);

    // Contract operations
    std::string call_contract(const std::string& to, const std::string& data, 
                            uint64_t block_number = 0);
    
    // Mempool operations
    std::vector<std::string> get_pending_transactions();
    uint32_t get_pending_transaction_count();

    // Gas price operations
    std::string get_gas_price();
    GasEstimate get_fee_history(uint64_t block_count = 20, 
                               const std::vector<double>& reward_percentiles = {25.0, 50.0, 75.0});

    // Network information
    uint64_t get_chain_id();
    std::string get_network_version();
    bool is_syncing();

    // WebSocket subscriptions
    bool subscribe_to_new_heads(BlockCallback callback);
    bool subscribe_to_pending_transactions(PendingTransactionCallback callback);
    bool subscribe_to_logs(const std::string& filter, LogCallback callback);
    bool unsubscribe_from_new_heads();
    bool unsubscribe_from_pending_transactions();
    bool unsubscribe_from_logs();

    // Event handlers
    void set_disconnect_callback(DisconnectCallback callback);

    // Statistics
    struct ClientStats {
        std::atomic<uint64_t> total_requests{0};
        std::atomic<uint64_t> successful_requests{0};
        std::atomic<uint64_t> failed_requests{0};
        std::atomic<uint64_t> websocket_messages{0};
        std::atomic<uint64_t> blocks_processed{0};
        std::atomic<uint64_t> transactions_processed{0};
        std::chrono::system_clock::time_point start_time;
        std::chrono::system_clock::time_point last_block_time;
        uint64_t current_block_number{0};
    };

    const ClientStats& get_stats() const;
    void reset_stats();

    // Configuration
    void update_config(const EthereumConfig& config);
    const EthereumConfig& get_config() const;

private:
    class EthereumClientImpl;
    std::unique_ptr<EthereumClientImpl> pimpl_;
};

} // namespace hfx::chain
