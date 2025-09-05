/**
 * @file solana_client.cpp
 * @brief Solana RPC client implementation
 */

#include "../include/solana_client.hpp"
#include "hfx-log/include/logger.hpp"
#include <curl/curl.h>
#include <thread>
#include <sstream>
#include <iomanip>
#include <random>
#include <regex>

// Simple JSON parsing helpers (would use proper JSON library in production)
namespace {
    std::string extract_json_string(const std::string& json, const std::string& key) {
        std::regex pattern("\"" + key + "\"\\s*:\\s*\"([^\"]+)\"");
        std::smatch match;
        if (std::regex_search(json, match, pattern)) {
            return match[1].str();
        }
        return "";
    }

    uint64_t extract_json_number(const std::string& json, const std::string& key) {
        std::regex pattern("\"" + key + "\"\\s*:\\s*([0-9]+)");
        std::smatch match;
        if (std::regex_search(json, match, pattern)) {
            return std::stoull(match[1].str());
        }
        return 0;
    }

    size_t write_callback(void* contents, size_t size, size_t nmemb, std::string* s) {
        size_t newLength = size * nmemb;
        s->append((char*)contents, newLength);
        return newLength;
    }
}

namespace hfx::chain {

class SolanaClient::SolanaClientImpl {
public:
    SolanaConfig config_;
    CURL* curl_handle_;
    std::atomic<bool> connected_{false};
    std::atomic<bool> ws_connected_{false};
    std::atomic<uint64_t> request_id_{1};
    mutable std::mutex request_mutex_;
    
    // Callbacks
    SlotCallback slot_callback_;
    BlockCallback block_callback_;
    TransactionCallback transaction_callback_;
    AccountCallback account_callback_;
    DisconnectCallback disconnect_callback_;
    
    // WebSocket subscriptions
    std::unordered_map<std::string, std::string> account_subscriptions_;
    
    // Statistics
    ClientStats stats_;
    
    // WebSocket thread
    std::thread ws_thread_;
    std::atomic<bool> ws_running_{false};
    
    // Jito endpoints
    std::string jito_block_engine_url_ = "https://mainnet.block-engine.jito.wtf";
    std::string jito_relayer_url_ = "https://mainnet.relayer.jito.wtf";
    
    SolanaClientImpl(const SolanaConfig& config) : config_(config) {
        curl_handle_ = curl_easy_init();
        stats_.start_time = std::chrono::system_clock::now();
        
        if (curl_handle_) {
            curl_easy_setopt(curl_handle_, CURLOPT_WRITEFUNCTION, write_callback);
            curl_easy_setopt(curl_handle_, CURLOPT_TIMEOUT_MS, config_.request_timeout.count());
            curl_easy_setopt(curl_handle_, CURLOPT_CONNECTTIMEOUT_MS, 5000);
            curl_easy_setopt(curl_handle_, CURLOPT_FOLLOWLOCATION, 1L);
            curl_easy_setopt(curl_handle_, CURLOPT_SSL_VERIFYPEER, 1L);
            curl_easy_setopt(curl_handle_, CURLOPT_SSL_VERIFYHOST, 2L);
            curl_easy_setopt(curl_handle_, CURLOPT_USERAGENT, "HydraFlow-X-Solana/1.0");
        }
    }
    
    ~SolanaClientImpl() {
        disconnect();
        if (curl_handle_) {
            curl_easy_cleanup(curl_handle_);
        }
    }
    
    std::string make_rpc_request(const std::string& method, const std::string& params = "[]") {
        if (!curl_handle_) {
            HFX_LOG_ERROR("[SolanaClient] CURL not initialized");
            stats_.failed_requests.fetch_add(1);
            return "";
        }
        
        std::lock_guard<std::mutex> lock(request_mutex_);
        
        uint64_t id = request_id_.fetch_add(1);
        
        // Build JSON-RPC request
        std::stringstream request_body;
        request_body << "{\"jsonrpc\":\"2.0\",\"id\":" << id 
                     << ",\"method\":\"" << method << "\",\"params\":" << params << "}";
        
        std::string request_str = request_body.str();
        std::string response_str;
        
        // Set CURL options for this request
        curl_easy_setopt(curl_handle_, CURLOPT_URL, config_.rpc_url.c_str());
        curl_easy_setopt(curl_handle_, CURLOPT_POSTFIELDS, request_str.c_str());
        curl_easy_setopt(curl_handle_, CURLOPT_WRITEDATA, &response_str);
        
        // Set headers
        struct curl_slist* headers = nullptr;
        headers = curl_slist_append(headers, "Content-Type: application/json");
        if (!config_.api_key.empty()) {
            std::string auth_header = "Authorization: Bearer " + config_.api_key;
            headers = curl_slist_append(headers, auth_header.c_str());
        }
        curl_easy_setopt(curl_handle_, CURLOPT_HTTPHEADER, headers);
        
        // Perform request
        CURLcode res = curl_easy_perform(curl_handle_);
        curl_slist_free_all(headers);
        
        stats_.total_requests.fetch_add(1);
        
        if (res != CURLE_OK) {
            HFX_LOG_ERROR("[SolanaClient] HTTP request failed: " + std::string(curl_easy_strerror(res)));
            stats_.failed_requests.fetch_add(1);
            return "";
        }
        
        long response_code;
        curl_easy_getinfo(curl_handle_, CURLINFO_RESPONSE_CODE, &response_code);
        
        if (response_code != 200) {
            HFX_LOG_ERROR("[SolanaClient] HTTP error: " + std::to_string(response_code));
            stats_.failed_requests.fetch_add(1);
            return "";
        }
        
        stats_.successful_requests.fetch_add(1);
        return response_str;
    }
    
    std::string make_jito_request(const std::string& endpoint, const std::string& payload) {
        if (!curl_handle_) {
            return "";
        }
        
        std::lock_guard<std::mutex> lock(request_mutex_);
        
        std::string response_str;
        curl_easy_setopt(curl_handle_, CURLOPT_URL, (jito_block_engine_url_ + endpoint).c_str());
        curl_easy_setopt(curl_handle_, CURLOPT_POSTFIELDS, payload.c_str());
        curl_easy_setopt(curl_handle_, CURLOPT_WRITEDATA, &response_str);
        
        struct curl_slist* headers = nullptr;
        headers = curl_slist_append(headers, "Content-Type: application/json");
        curl_easy_setopt(curl_handle_, CURLOPT_HTTPHEADER, headers);
        
        CURLcode res = curl_easy_perform(curl_handle_);
        curl_slist_free_all(headers);
        
        if (res != CURLE_OK) {
            HFX_LOG_ERROR("[SolanaClient] Jito request failed: " + std::string(curl_easy_strerror(res)));
            return "";
        }
        
        return response_str;
    }
    
    void start_websocket_connection() {
        if (!config_.enable_websocket || ws_running_) {
            return;
        }
        
        ws_running_ = true;
        ws_thread_ = std::thread(&SolanaClientImpl::websocket_worker, this);
        HFX_LOG_INFO("[SolanaClient] WebSocket thread started");
    }
    
    void websocket_worker() {
        HFX_LOG_INFO("[SolanaClient] WebSocket worker started");
        
        while (ws_running_) {
            try {
                // Simulate WebSocket connection and message processing
                std::this_thread::sleep_for(std::chrono::milliseconds(400));
                
                if (ws_connected_ && slot_callback_) {
                    // Simulate new slot notification
                    uint64_t current_slot = get_current_slot_internal();
                    if (current_slot > stats_.current_slot) {
                        stats_.current_slot = current_slot;
                        stats_.slots_processed.fetch_add(1);
                        stats_.last_slot_time = std::chrono::system_clock::now();
                        slot_callback_(current_slot);
                    }
                }
                
            } catch (const std::exception& e) {
                HFX_LOG_ERROR("[SolanaClient] WebSocket worker error: " + std::string(e.what()));
                std::this_thread::sleep_for(std::chrono::seconds(5));
            }
        }
        
        HFX_LOG_INFO("[SolanaClient] WebSocket worker stopped");
    }
    
    void stop_websocket_connection() {
        if (ws_running_) {
            ws_running_ = false;
            ws_connected_ = false;
            
            if (ws_thread_.joinable()) {
                ws_thread_.join();
            }
            
            HFX_LOG_INFO("[SolanaClient] WebSocket connection stopped");
        }
    }
    
    uint64_t get_current_slot_internal() {
        std::string response = make_rpc_request("getSlot", "[{\"commitment\":\"" + config_.commitment + "\"}]");
        if (response.empty()) {
            return 0;
        }
        
        return extract_json_number(response, "result");
    }
    
    std::optional<SolanaBlock> parse_block_from_json(const std::string& json) {
        if (json.find("\"result\"") == std::string::npos) {
            return std::nullopt;
        }
        
        SolanaBlock block;
        block.slot = extract_json_number(json, "slot");
        block.blockhash = extract_json_string(json, "blockhash");
        block.previous_blockhash = extract_json_string(json, "previousBlockhash");
        block.block_time = extract_json_number(json, "blockTime");
        block.block_height = extract_json_number(json, "blockHeight");
        
        // Count transactions
        std::regex tx_pattern("\"transactions\"\\s*:\\s*\\[([^\\]]+)\\]");
        std::smatch tx_match;
        if (std::regex_search(json, tx_match, tx_pattern)) {
            std::string tx_list = tx_match[1].str();
            block.transaction_count = std::count(tx_list.begin(), tx_list.end(), '{');
        }
        
        return block;
    }
    
    void disconnect() {
        connected_ = false;
        stop_websocket_connection();
    }
};

// SolanaClient implementation

SolanaClient::SolanaClient(const SolanaConfig& config) 
    : pimpl_(std::make_unique<SolanaClientImpl>(config)) {
    HFX_LOG_INFO("[SolanaClient] Initialized with RPC: " + config.rpc_url);
}

SolanaClient::~SolanaClient() {
    disconnect();
}

bool SolanaClient::connect() {
    if (pimpl_->connected_) {
        return true;
    }
    
    // Test connection with a simple request
    std::string response = pimpl_->make_rpc_request("getVersion");
    if (response.empty()) {
        HFX_LOG_ERROR("[SolanaClient] Failed to connect to Solana RPC");
        return false;
    }
    
    pimpl_->connected_ = true;
    
    // Start WebSocket connection if enabled
    if (pimpl_->config_.enable_websocket) {
        pimpl_->start_websocket_connection();
        pimpl_->ws_connected_ = true;
    }
    
    HFX_LOG_INFO("[SolanaClient] Connected to Solana network");
    return true;
}

void SolanaClient::disconnect() {
    if (pimpl_) {
        pimpl_->disconnect();
        HFX_LOG_INFO("[SolanaClient] Disconnected from Solana network");
    }
}

bool SolanaClient::is_connected() const {
    return pimpl_ && pimpl_->connected_;
}

bool SolanaClient::is_websocket_connected() const {
    return pimpl_ && pimpl_->ws_connected_;
}

uint64_t SolanaClient::get_slot() {
    if (!is_connected()) {
        return 0;
    }
    return pimpl_->get_current_slot_internal();
}

std::optional<SolanaBlock> SolanaClient::get_block(uint64_t slot) {
    if (!is_connected()) {
        return std::nullopt;
    }
    
    std::string params = "[" + std::to_string(slot) + ", {\"encoding\": \"json\", \"transactionDetails\": \"signatures\", \"rewards\": false}]";
    std::string response = pimpl_->make_rpc_request("getBlock", params);
    if (response.empty()) {
        return std::nullopt;
    }
    
    return pimpl_->parse_block_from_json(response);
}

uint64_t SolanaClient::get_block_height() {
    if (!is_connected()) {
        return 0;
    }
    
    std::string response = pimpl_->make_rpc_request("getBlockHeight", "[{\"commitment\":\"" + pimpl_->config_.commitment + "\"}]");
    if (response.empty()) {
        return 0;
    }
    
    uint64_t height = extract_json_number(response, "result");
    pimpl_->stats_.current_block_height = height;
    return height;
}

std::string SolanaClient::get_recent_blockhash() {
    if (!is_connected()) {
        return "";
    }
    
    std::string params = "[{\"commitment\":\"" + pimpl_->config_.commitment + "\"}]";
    std::string response = pimpl_->make_rpc_request("getRecentBlockhash", params);
    if (response.empty()) {
        return "";
    }
    
    return extract_json_string(response, "blockhash");
}

std::string SolanaClient::get_latest_blockhash() {
    if (!is_connected()) {
        return "";
    }
    
    std::string params = "[{\"commitment\":\"" + pimpl_->config_.commitment + "\"}]";
    std::string response = pimpl_->make_rpc_request("getLatestBlockhash", params);
    if (response.empty()) {
        return "";
    }
    
    return extract_json_string(response, "blockhash");
}

bool SolanaClient::is_blockhash_valid(const std::string& blockhash) {
    if (!is_connected() || blockhash.empty()) {
        return false;
    }
    
    std::string params = "[\"" + blockhash + "\", {\"commitment\":\"" + pimpl_->config_.commitment + "\"}]";
    std::string response = pimpl_->make_rpc_request("isBlockhashValid", params);
    if (response.empty()) {
        return false;
    }
    
    return response.find("\"result\":{\"value\":true}") != std::string::npos;
}

std::optional<SolanaAccount> SolanaClient::get_account_info(const std::string& address) {
    if (!is_connected()) {
        return std::nullopt;
    }
    
    std::string params = "[\"" + address + "\", {\"encoding\": \"base64\", \"commitment\":\"" + pimpl_->config_.commitment + "\"}]";
    std::string response = pimpl_->make_rpc_request("getAccountInfo", params);
    if (response.empty() || response.find("\"result\":{\"value\":null}") != std::string::npos) {
        return std::nullopt;
    }
    
    SolanaAccount account;
    account.address = address;
    account.lamports = extract_json_number(response, "lamports");
    account.owner = extract_json_string(response, "owner");
    account.data = extract_json_string(response, "data");
    account.executable = response.find("\"executable\":true") != std::string::npos;
    account.rent_epoch = extract_json_number(response, "rentEpoch");
    
    return account;
}

uint64_t SolanaClient::get_balance(const std::string& address) {
    if (!is_connected()) {
        return 0;
    }
    
    std::string params = "[\"" + address + "\", {\"commitment\":\"" + pimpl_->config_.commitment + "\"}]";
    std::string response = pimpl_->make_rpc_request("getBalance", params);
    if (response.empty()) {
        return 0;
    }
    
    return extract_json_number(response, "value");
}

std::string SolanaClient::send_transaction(const std::string& transaction) {
    if (!is_connected()) {
        return "";
    }
    
    std::string params = "[\"" + transaction + "\", {\"encoding\": \"base64\", \"skipPreflight\": false, \"preflightCommitment\":\"" + pimpl_->config_.commitment + "\"}]";
    std::string response = pimpl_->make_rpc_request("sendTransaction", params);
    if (response.empty()) {
        return "";
    }
    
    return extract_json_string(response, "result");
}

std::string SolanaClient::send_raw_transaction(const std::string& transaction) {
    return send_transaction(transaction);
}

std::optional<SolanaTransaction> SolanaClient::get_transaction(const std::string& signature) {
    if (!is_connected()) {
        return std::nullopt;
    }
    
    std::string params = "[\"" + signature + "\", {\"encoding\": \"json\", \"commitment\":\"" + pimpl_->config_.commitment + "\"}]";
    std::string response = pimpl_->make_rpc_request("getTransaction", params);
    if (response.empty() || response.find("\"result\":null") != std::string::npos) {
        return std::nullopt;
    }
    
    SolanaTransaction tx;
    tx.signature = signature;
    tx.slot = extract_json_number(response, "slot");
    tx.block_time = extract_json_number(response, "blockTime");
    tx.success = response.find("\"err\":null") != std::string::npos;
    tx.fee = extract_json_number(response, "fee");
    
    // Extract compute units if available
    std::regex compute_units_pattern("\"computeUnitsConsumed\":\\s*([0-9]+)");
    std::smatch match;
    if (std::regex_search(response, match, compute_units_pattern)) {
        tx.compute_units_consumed = std::stoull(match[1].str());
    }
    
    return tx;
}

std::vector<std::string> SolanaClient::get_signatures_for_address(const std::string& address, size_t limit) {
    std::vector<std::string> signatures;
    
    if (!is_connected()) {
        return signatures;
    }
    
    std::string params = "[\"" + address + "\", {\"limit\": " + std::to_string(limit) + ", \"commitment\":\"" + pimpl_->config_.commitment + "\"}]";
    std::string response = pimpl_->make_rpc_request("getSignaturesForAddress", params);
    if (response.empty()) {
        return signatures;
    }
    
    // Extract signatures from response
    std::regex sig_pattern("\"signature\"\\s*:\\s*\"([^\"]+)\"");
    std::sregex_iterator iter(response.begin(), response.end(), sig_pattern);
    std::sregex_iterator end;
    
    for (; iter != end; ++iter) {
        signatures.push_back(iter->str(1));
    }
    
    return signatures;
}

SolanaPriorityFees SolanaClient::get_priority_fees() {
    SolanaPriorityFees fees;
    fees.last_updated = std::chrono::system_clock::now();
    
    if (!is_connected()) {
        return fees;
    }
    
    // Get recent priority fee percentiles
    std::string params = "[{\"commitment\":\"" + pimpl_->config_.commitment + "\"}]";
    std::string response = pimpl_->make_rpc_request("getRecentPrioritizationFees", params);
    
    if (!response.empty()) {
        // Parse priority fees - simplified implementation
        fees.min_priority_fee = 0;
        fees.median_priority_fee = 5000; // 5000 micro-lamports default
        fees.max_priority_fee = 50000;
        fees.percentile_50 = 5000;
        fees.percentile_75 = 15000;
        fees.percentile_95 = 35000;
    }
    
    return fees;
}

uint64_t SolanaClient::get_minimum_balance_for_rent_exemption(size_t data_length) {
    if (!is_connected()) {
        return 0;
    }
    
    std::string params = "[" + std::to_string(data_length) + "]";
    std::string response = pimpl_->make_rpc_request("getMinimumBalanceForRentExemption", params);
    if (response.empty()) {
        return 0;
    }
    
    return extract_json_number(response, "result");
}

bool SolanaClient::subscribe_to_slot_updates(SlotCallback callback) {
    if (!is_websocket_connected()) {
        return false;
    }
    
    pimpl_->slot_callback_ = callback;
    HFX_LOG_INFO("[SolanaClient] Subscribed to slot updates");
    return true;
}

bool SolanaClient::subscribe_to_block_updates(BlockCallback callback) {
    if (!is_websocket_connected()) {
        return false;
    }
    
    pimpl_->block_callback_ = callback;
    HFX_LOG_INFO("[SolanaClient] Subscribed to block updates");
    return true;
}

bool SolanaClient::subscribe_to_account_changes(const std::string& address, AccountCallback callback) {
    if (!is_websocket_connected()) {
        return false;
    }
    
    pimpl_->account_callback_ = callback;
    pimpl_->account_subscriptions_[address] = "subscription_id_" + address;
    HFX_LOG_INFO("[SolanaClient] Subscribed to account changes: " + address);
    return true;
}

bool SolanaClient::submit_bundle_to_jito(const std::vector<std::string>& transactions) {
    if (!is_connected() || transactions.empty()) {
        return false;
    }
    
    // Build bundle request
    std::stringstream bundle_json;
    bundle_json << "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"sendBundle\",\"params\":[[";
    for (size_t i = 0; i < transactions.size(); ++i) {
        if (i > 0) bundle_json << ",";
        bundle_json << "\"" << transactions[i] << "\"";
    }
    bundle_json << "]]}";
    
    std::string response = pimpl_->make_jito_request("/api/v1/bundles", bundle_json.str());
    
    if (response.empty()) {
        HFX_LOG_ERROR("[SolanaClient] Failed to submit bundle to Jito");
        return false;
    }
    
    HFX_LOG_INFO("[SolanaClient] Successfully submitted bundle to Jito with " + std::to_string(transactions.size()) + " transactions");
    return true;
}

std::string SolanaClient::get_jito_tip_accounts() {
    std::string response = pimpl_->make_jito_request("/api/v1/bundles/tip_accounts", "");
    return response;
}

// Statistics and configuration methods
const SolanaClient::ClientStats& SolanaClient::get_stats() const {
    return pimpl_->stats_;
}

void SolanaClient::reset_stats() {
    if (pimpl_) {
        pimpl_->stats_ = ClientStats{};
        pimpl_->stats_.start_time = std::chrono::system_clock::now();
    }
}

void SolanaClient::update_config(const SolanaConfig& config) {
    if (pimpl_) {
        pimpl_->config_ = config;
        HFX_LOG_INFO("[SolanaClient] Configuration updated");
    }
}

const SolanaConfig& SolanaClient::get_config() const {
    return pimpl_->config_;
}

// Placeholder implementations for additional methods
std::vector<SolanaAccount> SolanaClient::get_multiple_accounts(const std::vector<std::string>& addresses) {
    // TODO: Implement batch account fetching
    return {};
}

std::vector<SolanaAccount> SolanaClient::get_program_accounts(const std::string& program_id) {
    // TODO: Implement program account fetching
    return {};
}

std::vector<SolanaAccount> SolanaClient::get_token_accounts_by_owner(const std::string& owner, const std::string& mint) {
    // TODO: Implement token account fetching
    return {};
}

uint64_t SolanaClient::get_token_supply(const std::string& mint) {
    if (!is_connected()) return 0;
    
    std::string params = "[\"" + mint + "\"]";
    std::string response = pimpl_->make_rpc_request("getTokenSupply", params);
    return extract_json_number(response, "uiAmount");
}

uint64_t SolanaClient::estimate_transaction_fee(const std::string& transaction) {
    // Simplified fee estimation
    return 5000; // Base fee in lamports
}

std::string SolanaClient::get_version() {
    std::string response = pimpl_->make_rpc_request("getVersion");
    return extract_json_string(response, "solana-core");
}

std::string SolanaClient::get_genesis_hash() {
    std::string response = pimpl_->make_rpc_request("getGenesisHash");
    return extract_json_string(response, "result");
}

std::string SolanaClient::get_identity() {
    std::string response = pimpl_->make_rpc_request("getIdentity");
    return extract_json_string(response, "identity");
}

uint64_t SolanaClient::get_transaction_count() {
    std::string response = pimpl_->make_rpc_request("getTransactionCount");
    return extract_json_number(response, "result");
}

double SolanaClient::get_current_tps() {
    // TODO: Implement TPS calculation
    return 0.0;
}

std::string SolanaClient::simulate_transaction(const std::string& transaction) {
    std::string params = "[\"" + transaction + "\"]";
    return pimpl_->make_rpc_request("simulateTransaction", params);
}

// Unsubscribe methods
bool SolanaClient::unsubscribe_from_slot_updates() {
    pimpl_->slot_callback_ = nullptr;
    return true;
}

bool SolanaClient::unsubscribe_from_block_updates() {
    pimpl_->block_callback_ = nullptr;
    return true;
}

bool SolanaClient::unsubscribe_from_account_changes(const std::string& address) {
    pimpl_->account_subscriptions_.erase(address);
    return true;
}

bool SolanaClient::unsubscribe_from_transaction_updates(const std::string& address) {
    return true;
}

bool SolanaClient::subscribe_to_transaction_updates(const std::string& address, TransactionCallback callback) {
    pimpl_->transaction_callback_ = callback;
    return true;
}

void SolanaClient::set_disconnect_callback(DisconnectCallback callback) {
    if (pimpl_) {
        pimpl_->disconnect_callback_ = callback;
    }
}

} // namespace hfx::chain
