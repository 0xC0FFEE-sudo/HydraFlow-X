/**
 * @file solana_client.hpp
 * @brief High-performance Solana RPC client with WebSocket support
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

struct SolanaAccount {
    std::string address;
    uint64_t lamports;
    std::string owner;
    std::string data;
    bool executable;
    uint64_t rent_epoch;
};

struct SolanaTransaction {
    std::string signature;
    uint64_t slot;
    uint64_t block_time;
    bool success;
    std::string error_message;
    uint64_t fee;
    std::vector<std::string> account_keys;
    std::vector<std::string> log_messages;
    uint64_t compute_units_consumed;
};

struct SolanaBlock {
    uint64_t slot;
    std::string blockhash;
    std::string previous_blockhash;
    uint64_t block_time;
    uint64_t block_height;
    std::vector<std::string> transaction_signatures;
    size_t transaction_count;
    uint64_t total_fee;
};

struct SolanaPriorityFees {
    uint64_t min_priority_fee;
    uint64_t median_priority_fee;
    uint64_t max_priority_fee;
    double percentile_50;
    double percentile_75;
    double percentile_95;
    std::chrono::system_clock::time_point last_updated;
};

struct SolanaConfig {
    std::string rpc_url = "https://api.mainnet-beta.solana.com";
    std::string ws_url = "wss://api.mainnet-beta.solana.com";
    std::string api_key;
    std::chrono::milliseconds request_timeout = std::chrono::milliseconds(10000);
    std::chrono::milliseconds ws_ping_interval = std::chrono::seconds(30);
    size_t max_concurrent_requests = 20;
    size_t max_retries = 3;
    std::chrono::milliseconds retry_delay = std::chrono::milliseconds(100);
    bool enable_websocket = true;
    bool enable_transaction_monitoring = true;
    std::string commitment = "confirmed"; // finalized, confirmed, processed
};

class SolanaClient {
public:
    // Callback types
    using SlotCallback = std::function<void(uint64_t)>;
    using BlockCallback = std::function<void(const SolanaBlock&)>;
    using TransactionCallback = std::function<void(const SolanaTransaction&)>;
    using AccountCallback = std::function<void(const std::string&, const SolanaAccount&)>;
    using DisconnectCallback = std::function<void()>;

    explicit SolanaClient(const SolanaConfig& config);
    ~SolanaClient();

    // Connection management
    bool connect();
    void disconnect();
    bool is_connected() const;
    bool is_websocket_connected() const;

    // Slot and block operations
    uint64_t get_slot();
    std::optional<SolanaBlock> get_block(uint64_t slot);
    uint64_t get_block_height();
    std::string get_recent_blockhash();
    std::string get_latest_blockhash();
    bool is_blockhash_valid(const std::string& blockhash);

    // Transaction operations
    std::optional<SolanaTransaction> get_transaction(const std::string& signature);
    std::string send_transaction(const std::string& transaction);
    std::string send_raw_transaction(const std::string& transaction);
    std::vector<std::string> get_signatures_for_address(const std::string& address, 
                                                       size_t limit = 1000);
    
    // Account operations
    std::optional<SolanaAccount> get_account_info(const std::string& address);
    uint64_t get_balance(const std::string& address);
    std::vector<SolanaAccount> get_multiple_accounts(const std::vector<std::string>& addresses);
    std::vector<SolanaAccount> get_program_accounts(const std::string& program_id);

    // Token operations
    std::vector<SolanaAccount> get_token_accounts_by_owner(const std::string& owner,
                                                         const std::string& mint = "");
    uint64_t get_token_supply(const std::string& mint);
    
    // Fee operations
    uint64_t get_minimum_balance_for_rent_exemption(size_t data_length);
    SolanaPriorityFees get_priority_fees();
    uint64_t estimate_transaction_fee(const std::string& transaction);

    // Network information
    std::string get_version();
    std::string get_genesis_hash();
    std::string get_identity();
    uint64_t get_transaction_count();
    double get_current_tps();

    // Program operations
    std::string simulate_transaction(const std::string& transaction);
    
    // WebSocket subscriptions
    bool subscribe_to_slot_updates(SlotCallback callback);
    bool subscribe_to_block_updates(BlockCallback callback);
    bool subscribe_to_account_changes(const std::string& address, AccountCallback callback);
    bool subscribe_to_transaction_updates(const std::string& address, TransactionCallback callback);
    bool unsubscribe_from_slot_updates();
    bool unsubscribe_from_block_updates();
    bool unsubscribe_from_account_changes(const std::string& address);
    bool unsubscribe_from_transaction_updates(const std::string& address);

    // Event handlers
    void set_disconnect_callback(DisconnectCallback callback);

    // Statistics
    struct ClientStats {
        std::atomic<uint64_t> total_requests{0};
        std::atomic<uint64_t> successful_requests{0};
        std::atomic<uint64_t> failed_requests{0};
        std::atomic<uint64_t> websocket_messages{0};
        std::atomic<uint64_t> slots_processed{0};
        std::atomic<uint64_t> transactions_processed{0};
        std::chrono::system_clock::time_point start_time;
        std::chrono::system_clock::time_point last_slot_time;
        uint64_t current_slot{0};
        uint64_t current_block_height{0};
    };

    const ClientStats& get_stats() const;
    void reset_stats();

    // Configuration
    void update_config(const SolanaConfig& config);
    const SolanaConfig& get_config() const;

    // Jito MEV integration
    bool submit_bundle_to_jito(const std::vector<std::string>& transactions);
    std::string get_jito_tip_accounts();

private:
    class SolanaClientImpl;
    std::unique_ptr<SolanaClientImpl> pimpl_;
};

} // namespace hfx::chain
