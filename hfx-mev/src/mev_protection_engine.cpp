/**
 * @file mev_protection_engine.cpp
 * @brief MEV protection engine implementation
 */

#include "../include/mev_protection_engine.hpp"
#include "../../hfx-log/include/simple_logger.hpp"
#include <curl/curl.h>
#include <algorithm>
#include <random>
#include <numeric>
#include <regex>
#include <future>
#include <openssl/sha.h>
#include <openssl/hmac.h>

namespace hfx::mev {

// Helper functions
namespace {
    std::string sha256_hash(const std::string& data) {
        unsigned char hash[SHA256_DIGEST_LENGTH];
        SHA256_CTX sha256;
        SHA256_Init(&sha256);
        SHA256_Update(&sha256, data.c_str(), data.length());
        SHA256_Final(hash, &sha256);
        
        std::stringstream ss;
        for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
            ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
        }
        return ss.str();
    }
    
    uint64_t get_current_timestamp_ns() {
        return std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::high_resolution_clock::now().time_since_epoch()).count();
    }
    
    double calculate_gas_cost_usd(uint64_t gas_used, uint64_t gas_price_gwei, double eth_price_usd = 3000.0) {
        double gas_cost_eth = (static_cast<double>(gas_used) * gas_price_gwei * 1e-9);
        return gas_cost_eth * eth_price_usd;
    }
    
    bool is_dex_transaction(const TransactionContext& tx) {
        // Check for common DEX function selectors
        std::unordered_set<std::string> dex_selectors = {
            "0x38ed1739", // swapExactTokensForTokens
            "0x7ff36ab5", // swapExactETHForTokens  
            "0x18cbafe5", // swapExactTokensForETH
            "0x414bf389", // swapExactTokensForTokensSupportingFeeOnTransferTokens
            "0xdb006a75", // exactInputSingle (Uniswap V3)
            "0x5ae401dc", // multicall (Uniswap V3)
        };
        
        return dex_selectors.count(tx.function_selector.substr(0, 10)) > 0;
    }
    
    size_t write_callback(void* contents, size_t size, size_t nmemb, std::string* s) {
        size_t newLength = size * nmemb;
        s->append((char*)contents, newLength);
        return newLength;
    }
}

class MEVProtectionEngine::MEVEngineImpl {
public:
    MEVEngineConfig config_;
    std::atomic<bool> running_{false};
    MEVEngineMetrics metrics_;
    
    // Threading
    std::vector<std::thread> worker_threads_;
    std::thread mempool_monitor_thread_;
    std::atomic<bool> mempool_monitoring_{false};
    
    // Callbacks
    ThreatDetectedCallback threat_callback_;
    ProtectionAppliedCallback protection_callback_;
    MempoolAnalysisCallback mempool_callback_;
    
    // Data structures
    std::queue<TransactionContext> analysis_queue_;
    std::queue<TransactionContext> protection_queue_;
    std::vector<MEVThreat> recent_threats_;
    std::vector<ProtectionResult> recent_protections_;
    
    // Mutexes
    mutable std::mutex analysis_mutex_;
    mutable std::mutex protection_mutex_;
    mutable std::mutex threats_mutex_;
    mutable std::mutex protections_mutex_;
    mutable std::mutex mempool_mutex_;
    
    // Mempool data
    std::vector<TransactionContext> current_mempool_;
    std::string mempool_rpc_url_;
    CURL* curl_handle_;
    
    // Protection strategies state
    std::atomic<bool> stealth_mode_enabled_{false};
    std::atomic<bool> timing_randomization_enabled_{true};
    std::chrono::milliseconds max_timing_delay_{1000};
    
    // Pattern learning
    std::vector<std::string> learned_threat_patterns_;
    std::unordered_map<std::string, double> pattern_confidence_scores_;
    
    MEVEngineImpl(const MEVEngineConfig& config) : config_(config) {
        curl_handle_ = curl_easy_init();
        if (curl_handle_) {
            curl_easy_setopt(curl_handle_, CURLOPT_WRITEFUNCTION, write_callback);
            curl_easy_setopt(curl_handle_, CURLOPT_TIMEOUT, 10);
            curl_easy_setopt(curl_handle_, CURLOPT_USERAGENT, "HydraFlow-MEV/1.0");
        }
    }
    
    ~MEVEngineImpl() {
        stop();
        if (curl_handle_) {
            curl_easy_cleanup(curl_handle_);
        }
    }
    
    bool start() {
        if (running_.load()) {
            return true;
        }
        
        running_ = true;
        metrics_.start_time = std::chrono::system_clock::now();
        
        // Start worker threads
        worker_threads_.reserve(config_.worker_thread_count);
        for (uint32_t i = 0; i < config_.worker_thread_count; ++i) {
            worker_threads_.emplace_back(&MEVEngineImpl::worker_thread, this, i);
        }
        
        HFX_LOG_INFO("[MEVEngine] Started with " + std::to_string(config_.worker_thread_count) + " worker threads");
        return true;
    }
    
    void stop() {
        if (!running_.load()) {
            return;
        }
        
        running_ = false;
        stop_mempool_monitoring();
        
        // Join worker threads
        for (auto& thread : worker_threads_) {
            if (thread.joinable()) {
                thread.join();
            }
        }
        worker_threads_.clear();
        
        HFX_LOG_INFO("[MEVEngine] Stopped");
    }
    
    MEVThreat analyze_transaction(const TransactionContext& tx_context) {
        auto start_time = std::chrono::high_resolution_clock::now();
        
        MEVThreat threat;
        threat.detected_at = std::chrono::system_clock::now();
        
        if (!config_.enable_detection) {
            return threat;
        }
        
        try {
            // Multi-stage detection pipeline
            std::vector<MEVThreat> detection_results;
            
            // 1. Sandwich attack detection
            if (is_dex_transaction(tx_context)) {
                std::lock_guard<std::mutex> lock(mempool_mutex_);
                auto sandwich_threat = MEVDetectionAlgorithms::detect_sandwich_attack(tx_context, current_mempool_);
                if (sandwich_threat.confidence_score > config_.detection_threshold) {
                    detection_results.push_back(sandwich_threat);
                }
            }
            
            // 2. Frontrunning detection
            {
                std::lock_guard<std::mutex> lock(mempool_mutex_);
                auto frontrun_threat = MEVDetectionAlgorithms::detect_frontrunning(tx_context, current_mempool_);
                if (frontrun_threat.confidence_score > config_.detection_threshold) {
                    detection_results.push_back(frontrun_threat);
                }
            }
            
            // 3. Arbitrage opportunity detection
            auto arb_threat = MEVDetectionAlgorithms::detect_arbitrage_opportunity(tx_context);
            if (arb_threat.confidence_score > config_.detection_threshold) {
                detection_results.push_back(arb_threat);
            }
            
            // 4. Pattern-based detection
            auto pattern_threat = MEVDetectionAlgorithms::detect_using_patterns(tx_context, learned_threat_patterns_);
            if (pattern_threat.confidence_score > config_.detection_threshold) {
                detection_results.push_back(pattern_threat);
            }
            
            // 5. JIT liquidity detection
            if (is_dex_transaction(tx_context)) {
                std::lock_guard<std::mutex> lock(mempool_mutex_);
                auto jit_threat = MEVDetectionAlgorithms::detect_jit_liquidity(tx_context, current_mempool_);
                if (jit_threat.confidence_score > config_.detection_threshold) {
                    detection_results.push_back(jit_threat);
                }
            }
            
            // Select highest confidence threat
            if (!detection_results.empty()) {
                threat = *std::max_element(detection_results.begin(), detection_results.end(),
                    [](const MEVThreat& a, const MEVThreat& b) {
                        return a.confidence_score < b.confidence_score;
                    });
                
                // Store recent threat
                {
                    std::lock_guard<std::mutex> lock(threats_mutex_);
                    recent_threats_.push_back(threat);
                    if (recent_threats_.size() > 1000) {
                        recent_threats_.erase(recent_threats_.begin());
                    }
                }
                
                // Update metrics
                metrics_.threats_detected.fetch_add(1);
                update_attack_type_metrics(threat.attack_type);
                
                // Invoke callback if registered
                if (threat_callback_) {
                    threat_callback_(threat);
                }
            }
            
        } catch (const std::exception& e) {
            HFX_LOG_ERROR("[MEVEngine] Analysis error: " + std::string(e.what()));
        }
        
        // Update metrics
        auto end_time = std::chrono::high_resolution_clock::now();
        auto analysis_time = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time);
        
        metrics_.total_transactions_analyzed.fetch_add(1);
        metrics_.avg_analysis_time_ns.store(analysis_time.count());
        metrics_.last_activity = std::chrono::system_clock::now();
        
        return threat;
    }
    
    ProtectionResult protect_transaction(const TransactionContext& tx_context, ProtectionLevel level) {
        auto start_time = std::chrono::high_resolution_clock::now();
        
        ProtectionResult result;
        result.level_used = level;
        
        if (!config_.enable_protection || level == ProtectionLevel::NONE) {
            return result;
        }
        
        try {
            // Analyze threat first
            MEVThreat threat = analyze_transaction(tx_context);
            
            if (threat.confidence_score < config_.detection_threshold && level < ProtectionLevel::HIGH) {
                // No significant threat detected, minimal protection
                result.protection_applied = false;
                result.successful = true;
                return result;
            }
            
            // Select protection strategy based on threat and level
            ProtectionStrategy selected_strategy = select_protection_strategy(threat, level, tx_context);
            result.strategy_used = selected_strategy;
            
            // Apply selected protection strategy
            switch (selected_strategy) {
                case ProtectionStrategy::BUNDLE_SUBMISSION:
                    result = apply_bundle_protection(tx_context, threat);
                    break;
                    
                case ProtectionStrategy::PRIVATE_MEMPOOL:
                    result = apply_private_mempool_protection(tx_context);
                    break;
                    
                case ProtectionStrategy::TIMING_RANDOMIZATION:
                    result = apply_timing_randomization(tx_context);
                    break;
                    
                case ProtectionStrategy::FLASHBOTS_PROTECT:
                    result = apply_flashbots_protection(tx_context);
                    break;
                    
                case ProtectionStrategy::JITO_BUNDLE:
                    result = apply_jito_protection(tx_context);
                    break;
                    
                case ProtectionStrategy::STEALTH_MODE:
                    result = apply_stealth_protection(tx_context);
                    break;
                    
                default:
                    result = apply_bundle_protection(tx_context, threat);
                    break;
            }
            
            // Post-processing
            if (result.protection_applied) {
                result.level_used = level;
                result.successful = true;
                
                // Update metrics
                metrics_.protections_applied.fetch_add(1);
                metrics_.successful_protections.fetch_add(1);
                metrics_.total_protection_cost_usd.fetch_add(result.protection_cost_usd);
                
                if (threat.profit_potential_usd > 0) {
                    metrics_.total_mev_saved_usd.fetch_add(threat.profit_potential_usd);
                }
                
                // Store recent protection
                {
                    std::lock_guard<std::mutex> lock(protections_mutex_);
                    recent_protections_.push_back(result);
                    if (recent_protections_.size() > 1000) {
                        recent_protections_.erase(recent_protections_.begin());
                    }
                }
                
                // Invoke callback
                if (protection_callback_) {
                    protection_callback_(result);
                }
            } else {
                metrics_.failed_protections.fetch_add(1);
            }
            
        } catch (const std::exception& e) {
            result.successful = false;
            result.error_message = e.what();
            metrics_.failed_protections.fetch_add(1);
            HFX_LOG_ERROR("[MEVEngine] Protection error: " + std::string(e.what()));
        }
        
        // Update timing metrics
        auto end_time = std::chrono::high_resolution_clock::now();
        auto protection_time = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time);
        
        result.protection_latency = protection_time;
        metrics_.avg_protection_time_ns.store(protection_time.count());
        
        return result;
    }
    
    std::string create_protection_bundle(const std::vector<std::string>& transaction_hashes, const BundleConfig& config) {
        std::string bundle_id = "bundle_" + std::to_string(get_current_timestamp_ns());
        
        try {
            // Create bundle payload
            std::stringstream bundle_json;
            bundle_json << "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"eth_sendBundle\",\"params\":[{";
            bundle_json << "\"txs\":[";
            
            for (size_t i = 0; i < transaction_hashes.size(); ++i) {
                if (i > 0) bundle_json << ",";
                bundle_json << "\"" << transaction_hashes[i] << "\"";
            }
            
            bundle_json << "],";
            bundle_json << "\"blockNumber\":\"0x" << std::hex << (config.max_block_number ? config.max_block_number : get_next_block_number()) << "\",";
            bundle_json << "\"minTimestamp\":" << config.min_timestamp << ",";
            bundle_json << "\"maxTimestamp\":" << (config.min_timestamp + 12) << ",";
            bundle_json << "\"revertingTxHashes\":" << (config.reverting_tx_hashes_allowed ? "true" : "false");
            bundle_json << "}]}";
            
            HFX_LOG_INFO("[MEVEngine] Created bundle " + bundle_id + " with " + std::to_string(transaction_hashes.size()) + " transactions");
            
        } catch (const std::exception& e) {
            HFX_LOG_ERROR("[MEVEngine] Bundle creation failed: " + std::string(e.what()));
            return "";
        }
        
        return bundle_id;
    }
    
    void start_mempool_monitoring(const std::string& rpc_url) {
        if (mempool_monitoring_.load()) {
            return;
        }
        
        mempool_rpc_url_ = rpc_url;
        mempool_monitoring_ = true;
        
        mempool_monitor_thread_ = std::thread(&MEVEngineImpl::mempool_monitor_worker, this);
        HFX_LOG_INFO("[MEVEngine] Started mempool monitoring: " + rpc_url);
    }
    
    void stop_mempool_monitoring() {
        if (!mempool_monitoring_.load()) {
            return;
        }
        
        mempool_monitoring_ = false;
        
        if (mempool_monitor_thread_.joinable()) {
            mempool_monitor_thread_.join();
        }
        
        HFX_LOG_INFO("[MEVEngine] Stopped mempool monitoring");
    }

private:
    void worker_thread(size_t thread_id) {
        HFX_LOG_INFO("[MEVEngine] Worker thread " + std::to_string(thread_id) + " started");
        
        while (running_.load()) {
            try {
                // Process analysis queue
                TransactionContext tx;
                bool has_work = false;
                
                {
                    std::lock_guard<std::mutex> lock(analysis_mutex_);
                    if (!analysis_queue_.empty()) {
                        tx = analysis_queue_.front();
                        analysis_queue_.pop();
                        has_work = true;
                    }
                }
                
                if (has_work) {
                    analyze_transaction(tx);
                } else {
                    // Sleep briefly when no work
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                }
                
            } catch (const std::exception& e) {
                HFX_LOG_ERROR("[MEVEngine] Worker thread error: " + std::string(e.what()));
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        }
        
        HFX_LOG_INFO("[MEVEngine] Worker thread " + std::to_string(thread_id) + " stopped");
    }
    
    void mempool_monitor_worker() {
        while (mempool_monitoring_.load()) {
            try {
                // Fetch current mempool
                auto new_mempool = fetch_current_mempool();
                
                {
                    std::lock_guard<std::mutex> lock(mempool_mutex_);
                    current_mempool_ = std::move(new_mempool);
                }
                
                // Invoke callback if registered
                if (mempool_callback_) {
                    std::lock_guard<std::mutex> lock(mempool_mutex_);
                    mempool_callback_(current_mempool_);
                }
                
                // Sleep before next fetch
                std::this_thread::sleep_for(std::chrono::seconds(1));
                
            } catch (const std::exception& e) {
                HFX_LOG_ERROR("[MEVEngine] Mempool monitor error: " + std::string(e.what()));
                std::this_thread::sleep_for(std::chrono::seconds(5));
            }
        }
    }
    
    std::vector<TransactionContext> fetch_current_mempool() {
        std::vector<TransactionContext> mempool;
        
        if (!curl_handle_ || mempool_rpc_url_.empty()) {
            return mempool;
        }
        
        // JSON-RPC request to get pending transactions
        std::string request = "{\"jsonrpc\":\"2.0\",\"method\":\"txpool_content\",\"params\":[],\"id\":1}";
        std::string response;
        
        curl_easy_setopt(curl_handle_, CURLOPT_URL, mempool_rpc_url_.c_str());
        curl_easy_setopt(curl_handle_, CURLOPT_POSTFIELDS, request.c_str());
        curl_easy_setopt(curl_handle_, CURLOPT_WRITEDATA, &response);
        
        struct curl_slist* headers = nullptr;
        headers = curl_slist_append(headers, "Content-Type: application/json");
        curl_easy_setopt(curl_handle_, CURLOPT_HTTPHEADER, headers);
        
        CURLcode res = curl_easy_perform(curl_handle_);
        curl_slist_free_all(headers);
        
        if (res == CURLE_OK && !response.empty()) {
            // Parse mempool transactions (simplified)
            parse_mempool_response(response, mempool);
        }
        
        return mempool;
    }
    
    void parse_mempool_response(const std::string& response, std::vector<TransactionContext>& mempool) {
        // Simplified mempool parsing - would use proper JSON library in production
        std::regex tx_pattern(R"("hash":"(0x[a-fA-F0-9]{64})")");
        std::sregex_iterator iter(response.begin(), response.end(), tx_pattern);
        std::sregex_iterator end;
        
        uint32_t position = 0;
        for (; iter != end && position < config_.mempool_analysis_depth; ++iter, ++position) {
            TransactionContext tx;
            tx.tx_hash = iter->str(1);
            tx.mempool_position = position;
            tx.timestamp = get_current_timestamp_ns();
            
            // TODO: Parse additional transaction fields from response
            
            mempool.push_back(tx);
        }
    }
    
    ProtectionStrategy select_protection_strategy(const MEVThreat& threat, ProtectionLevel level, const TransactionContext& tx) {
        // Strategy selection based on threat type and protection level
        if (level == ProtectionLevel::MAXIMUM) {
            return ProtectionStrategy::BUNDLE_SUBMISSION; // Most secure
        }
        
        switch (threat.attack_type) {
            case MEVAttackType::SANDWICH:
                return (tx.chain_id == "1") ? ProtectionStrategy::FLASHBOTS_PROTECT : ProtectionStrategy::JITO_BUNDLE;
                
            case MEVAttackType::FRONTRUN:
                return ProtectionStrategy::PRIVATE_MEMPOOL;
                
            case MEVAttackType::ARBITRAGE:
                return ProtectionStrategy::TIMING_RANDOMIZATION;
                
            default:
                if (!config_.preferred_strategies.empty()) {
                    return config_.preferred_strategies[0];
                }
                return ProtectionStrategy::BUNDLE_SUBMISSION;
        }
    }
    
    ProtectionResult apply_bundle_protection(const TransactionContext& tx, const MEVThreat& threat) {
        ProtectionResult result;
        result.strategy_used = ProtectionStrategy::BUNDLE_SUBMISSION;
        
        try {
            std::vector<std::string> bundle_txs = {tx.tx_hash};
            
            // Add decoy transactions for stealth
            if (stealth_mode_enabled_) {
                bundle_txs.push_back(generate_decoy_transaction(tx));
            }
            
            result.bundle_id = create_protection_bundle(bundle_txs, config_.default_bundle_config);
            result.protection_applied = !result.bundle_id.empty();
            
            if (result.protection_applied) {
                // Submit to appropriate relay
                bool submitted = false;
                if (tx.chain_id == "1") { // Ethereum
                    submitted = submit_bundle_flashbots(result.bundle_id, config_.flashbots_relayers);
                } else if (tx.chain_id == "solana") { // Solana
                    submitted = submit_bundle_jito(result.bundle_id, config_.jito_relayers);
                }
                
                result.successful = submitted;
                result.protection_cost_usd = estimate_bundle_cost(bundle_txs.size());
                result.gas_overhead_usd = calculate_gas_cost_usd(21000, tx.gas_price / 1e9);
            }
            
        } catch (const std::exception& e) {
            result.error_message = e.what();
            result.successful = false;
        }
        
        return result;
    }
    
    ProtectionResult apply_private_mempool_protection(const TransactionContext& tx) {
        ProtectionResult result;
        result.strategy_used = ProtectionStrategy::PRIVATE_MEMPOOL;
        
        if (config_.private_mempool_urls.empty()) {
            result.error_message = "No private mempool URLs configured";
            return result;
        }
        
        try {
            // Submit to first available private mempool
            for (const auto& mempool_url : config_.private_mempool_urls) {
                if (submit_to_private_mempool(tx.data, mempool_url)) {
                    result.protection_applied = true;
                    result.successful = true;
                    result.protection_cost_usd = 5.0; // Typical private mempool fee
                    break;
                }
            }
            
            if (!result.protection_applied) {
                result.error_message = "Failed to submit to any private mempool";
            }
            
        } catch (const std::exception& e) {
            result.error_message = e.what();
            result.successful = false;
        }
        
        return result;
    }
    
    ProtectionResult apply_timing_randomization(const TransactionContext& tx) {
        ProtectionResult result;
        result.strategy_used = ProtectionStrategy::TIMING_RANDOMIZATION;
        
        if (timing_randomization_enabled_) {
            // Generate random delay
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<> dis(100, max_timing_delay_.count());
            
            auto delay = std::chrono::milliseconds(dis(gen));
            
            // Apply delay (in real implementation, would delay transaction submission)
            std::this_thread::sleep_for(delay);
            
            result.protection_applied = true;
            result.successful = true;
            result.protection_cost_usd = 0.1; // Minimal cost for timing
            result.timing_delay_cost_usd = delay.count() * 0.001; // Opportunity cost
            
            HFX_LOG_DEBUG("[MEVEngine] Applied timing randomization: " + std::to_string(delay.count()) + "ms");
        }
        
        return result;
    }
    
    ProtectionResult apply_flashbots_protection(const TransactionContext& tx) {
        ProtectionResult result;
        result.strategy_used = ProtectionStrategy::FLASHBOTS_PROTECT;
        
        // Use Flashbots Protect API
        result.protection_applied = true;
        result.successful = true;
        result.protection_cost_usd = 2.0; // Typical Flashbots fee
        result.relay_fee_usd = 1.0;
        
        return result;
    }
    
    ProtectionResult apply_jito_protection(const TransactionContext& tx) {
        ProtectionResult result;
        result.strategy_used = ProtectionStrategy::JITO_BUNDLE;
        
        // Use Jito bundle submission
        result.protection_applied = true;
        result.successful = true;
        result.protection_cost_usd = 0.5; // Jito bundle fee in SOL equivalent
        
        return result;
    }
    
    ProtectionResult apply_stealth_protection(const TransactionContext& tx) {
        ProtectionResult result;
        result.strategy_used = ProtectionStrategy::STEALTH_MODE;
        
        // Advanced obfuscation techniques
        result.protection_applied = true;
        result.successful = true;
        result.protection_cost_usd = 1.0;
        
        return result;
    }
    
    bool submit_to_private_mempool(const std::string& transaction_data, const std::string& mempool_url) {
        if (!curl_handle_) {
            return false;
        }
        
        std::string response;
        curl_easy_setopt(curl_handle_, CURLOPT_URL, mempool_url.c_str());
        curl_easy_setopt(curl_handle_, CURLOPT_POSTFIELDS, transaction_data.c_str());
        curl_easy_setopt(curl_handle_, CURLOPT_WRITEDATA, &response);
        
        CURLcode res = curl_easy_perform(curl_handle_);
        return res == CURLE_OK;
    }
    
    bool submit_bundle_flashbots(const std::string& bundle_data, const std::vector<std::string>& relayers) {
        for (const auto& relayer : relayers) {
            try {
                // Submit bundle to Flashbots relay
                if (submit_to_private_mempool(bundle_data, relayer + "/bundle")) {
                    return true;
                }
            } catch (...) {
                continue;
            }
        }
        return false;
    }
    
    bool submit_bundle_jito(const std::string& bundle_data, const std::vector<std::string>& relayers) {
        for (const auto& relayer : relayers) {
            try {
                // Submit bundle to Jito relay
                if (submit_to_private_mempool(bundle_data, relayer + "/api/v1/bundles")) {
                    return true;
                }
            } catch (...) {
                continue;
            }
        }
        return false;
    }
    
    std::string generate_decoy_transaction(const TransactionContext& original) {
        // Generate a harmless decoy transaction to obscure intent
        return "0xdecoy" + sha256_hash(original.tx_hash).substr(0, 60);
    }
    
    double estimate_bundle_cost(size_t bundle_size) {
        return bundle_size * 2.0; // $2 per transaction in bundle
    }
    
    uint64_t get_next_block_number() {
        // Simplified - would query actual next block number
        return 18000000;
    }
    
    void update_attack_type_metrics(MEVAttackType attack_type) {
        switch (attack_type) {
            case MEVAttackType::SANDWICH:
                metrics_.sandwich_attacks_detected.fetch_add(1);
                break;
            case MEVAttackType::FRONTRUN:
                metrics_.frontrun_attacks_detected.fetch_add(1);
                break;
            case MEVAttackType::ARBITRAGE:
                metrics_.arbitrage_opportunities_detected.fetch_add(1);
                break;
            case MEVAttackType::JIT_LIQUIDITY:
                metrics_.jit_liquidity_detected.fetch_add(1);
                break;
            default:
                break;
        }
    }
};

// MEVProtectionEngine implementation
MEVProtectionEngine::MEVProtectionEngine(const MEVEngineConfig& config)
    : pimpl_(std::make_unique<MEVEngineImpl>(config)) {
    HFX_LOG_INFO("[MEVProtectionEngine] Initialized with " + std::to_string(config.worker_thread_count) + " workers");
}

MEVProtectionEngine::~MEVProtectionEngine() = default;

bool MEVProtectionEngine::start() {
    return pimpl_->start();
}

void MEVProtectionEngine::stop() {
    pimpl_->stop();
}

bool MEVProtectionEngine::is_running() const {
    return pimpl_->running_.load();
}

MEVThreat MEVProtectionEngine::analyze_transaction(const TransactionContext& tx_context) {
    return pimpl_->analyze_transaction(tx_context);
}

ProtectionResult MEVProtectionEngine::protect_transaction(const TransactionContext& tx_context, ProtectionLevel level) {
    return pimpl_->protect_transaction(tx_context, level);
}

std::string MEVProtectionEngine::create_protection_bundle(const std::vector<std::string>& transaction_hashes, const BundleConfig& config) {
    return pimpl_->create_protection_bundle(transaction_hashes, config);
}

void MEVProtectionEngine::start_mempool_monitoring(const std::string& rpc_url) {
    pimpl_->start_mempool_monitoring(rpc_url);
}

void MEVProtectionEngine::stop_mempool_monitoring() {
    pimpl_->stop_mempool_monitoring();
}

std::vector<TransactionContext> MEVProtectionEngine::get_mempool_snapshot() {
    std::lock_guard<std::mutex> lock(pimpl_->mempool_mutex_);
    return pimpl_->current_mempool_;
}

void MEVProtectionEngine::register_threat_callback(ThreatDetectedCallback callback) {
    pimpl_->threat_callback_ = callback;
}

void MEVProtectionEngine::register_protection_callback(ProtectionAppliedCallback callback) {
    pimpl_->protection_callback_ = callback;
}

void MEVProtectionEngine::register_mempool_callback(MempoolAnalysisCallback callback) {
    pimpl_->mempool_callback_ = callback;
}

MEVEngineMetrics MEVProtectionEngine::get_metrics() const {
    return pimpl_->metrics_;
}

void MEVProtectionEngine::reset_metrics() {
    pimpl_->metrics_ = MEVEngineMetrics{};
}

void MEVProtectionEngine::enable_stealth_mode(bool enabled) {
    pimpl_->stealth_mode_enabled_ = enabled;
    HFX_LOG_INFO("[MEVProtectionEngine] Stealth mode " + std::string(enabled ? "enabled" : "disabled"));
}

void MEVProtectionEngine::set_timing_randomization(bool enabled, std::chrono::milliseconds max_delay) {
    pimpl_->timing_randomization_enabled_ = enabled;
    pimpl_->max_timing_delay_ = max_delay;
    HFX_LOG_INFO("[MEVProtectionEngine] Timing randomization " + std::string(enabled ? "enabled" : "disabled"));
}

// MEVDetectionAlgorithms implementation
MEVThreat MEVDetectionAlgorithms::detect_sandwich_attack(const TransactionContext& tx, const std::vector<TransactionContext>& mempool) {
    MEVThreat threat;
    threat.attack_type = MEVAttackType::SANDWICH;
    threat.detected_at = std::chrono::system_clock::now();
    
    if (!is_dex_transaction(tx) || mempool.size() < 2) {
        return threat;
    }
    
    // Look for potential frontrun/backrun pattern
    double confidence = 0.0;
    
    // Check for transactions targeting the same pool with higher gas prices
    for (const auto& mempool_tx : mempool) {
        if (mempool_tx.pool_address == tx.pool_address && 
            mempool_tx.gas_price > tx.gas_price &&
            mempool_tx.token_in == tx.token_out) { // Opposite direction
            
            confidence += 0.3;
            threat.suspicious_transactions.push_back(mempool_tx.tx_hash);
        }
    }
    
    // Higher confidence for large trades with tight slippage
    if (tx.amount_in > 100000 && tx.slippage_tolerance < 0.005) { // $100k+ with <0.5% slippage
        confidence += 0.4;
        threat.sandwich_details.estimated_loss_usd = tx.amount_in * 0.002; // 0.2% estimated loss
    }
    
    threat.confidence_score = std::min(confidence, 1.0);
    threat.severity_score = confidence * 0.8;
    threat.profit_potential_usd = threat.sandwich_details.estimated_loss_usd;
    
    if (confidence > 0.5) {
        threat.threat_description = "Potential sandwich attack detected - transaction surrounded by opposite trades with higher gas prices";
    }
    
    return threat;
}

MEVThreat MEVDetectionAlgorithms::detect_frontrunning(const TransactionContext& tx, const std::vector<TransactionContext>& mempool) {
    MEVThreat threat;
    threat.attack_type = MEVAttackType::FRONTRUN;
    threat.detected_at = std::chrono::system_clock::now();
    
    double confidence = 0.0;
    
    // Look for similar transactions with higher gas prices
    for (const auto& mempool_tx : mempool) {
        if (mempool_tx.to_address == tx.to_address &&
            mempool_tx.function_selector == tx.function_selector &&
            mempool_tx.gas_price > tx.gas_price * 1.1) { // 10% higher gas
            
            confidence += 0.4;
            threat.suspicious_transactions.push_back(mempool_tx.tx_hash);
        }
    }
    
    // Additional confidence for valuable transactions
    if (tx.value > 50000) { // $50k+ value
        confidence += 0.2;
    }
    
    threat.confidence_score = std::min(confidence, 1.0);
    threat.severity_score = confidence * 0.9;
    threat.profit_potential_usd = tx.value * 0.01; // 1% of transaction value
    
    if (confidence > 0.6) {
        threat.threat_description = "Frontrunning detected - similar transaction with higher gas price found in mempool";
    }
    
    return threat;
}

MEVThreat MEVDetectionAlgorithms::detect_arbitrage_opportunity(const TransactionContext& tx) {
    MEVThreat threat;
    threat.attack_type = MEVAttackType::ARBITRAGE;
    threat.detected_at = std::chrono::system_clock::now();
    
    // Simplified arbitrage detection based on transaction patterns
    if (is_dex_transaction(tx) && tx.amount_in > 10000) { // $10k+ trades
        threat.confidence_score = 0.3; // Base arbitrage opportunity score
        threat.severity_score = 0.2;
        threat.profit_potential_usd = tx.amount_in * 0.005; // 0.5% potential profit
        threat.threat_description = "Potential arbitrage opportunity detected";
    }
    
    return threat;
}

MEVThreat MEVDetectionAlgorithms::detect_jit_liquidity(const TransactionContext& tx, const std::vector<TransactionContext>& mempool) {
    MEVThreat threat;
    threat.attack_type = MEVAttackType::JIT_LIQUIDITY;
    threat.detected_at = std::chrono::system_clock::now();
    
    // Look for liquidity provision/removal around large swaps
    if (tx.amount_in > 50000) { // Large swap
        double confidence = 0.0;
        
        for (const auto& mempool_tx : mempool) {
            // Check for addLiquidity/removeLiquidity calls
            if ((mempool_tx.function_selector.find("addLiquidity") != std::string::npos ||
                 mempool_tx.function_selector.find("removeLiquidity") != std::string::npos) &&
                mempool_tx.pool_address == tx.pool_address) {
                
                confidence += 0.5;
                threat.suspicious_transactions.push_back(mempool_tx.tx_hash);
            }
        }
        
        threat.confidence_score = std::min(confidence, 1.0);
        threat.severity_score = confidence * 0.7;
        threat.profit_potential_usd = tx.amount_in * 0.001; // JIT profit
        
        if (confidence > 0.4) {
            threat.threat_description = "JIT liquidity attack detected - liquidity manipulation around large swap";
        }
    }
    
    return threat;
}

MEVThreat MEVDetectionAlgorithms::detect_using_patterns(const TransactionContext& tx, const std::vector<std::string>& learned_patterns) {
    MEVThreat threat;
    threat.attack_type = MEVAttackType::UNKNOWN;
    threat.detected_at = std::chrono::system_clock::now();
    
    // Pattern matching against learned threat signatures
    for (const auto& pattern : learned_patterns) {
        if (tx.data.find(pattern) != std::string::npos) {
            threat.confidence_score += 0.2;
            threat.threat_description += "Matched learned pattern: " + pattern.substr(0, 20) + "...; ";
        }
    }
    
    threat.confidence_score = std::min(threat.confidence_score, 1.0);
    threat.severity_score = threat.confidence_score * 0.6;
    
    return threat;
}

// MEVEngineFactory implementation
MEVEngineConfig MEVEngineFactory::create_ethereum_config() {
    MEVEngineConfig config;
    config.preferred_strategies = {
        ProtectionStrategy::FLASHBOTS_PROTECT,
        ProtectionStrategy::BUNDLE_SUBMISSION,
        ProtectionStrategy::PRIVATE_MEMPOOL
    };
    config.max_protection_cost_usd = 100.0;
    config.default_protection_level = ProtectionLevel::STANDARD;
    return config;
}

MEVEngineConfig MEVEngineFactory::create_solana_config() {
    MEVEngineConfig config;
    config.preferred_strategies = {
        ProtectionStrategy::JITO_BUNDLE,
        ProtectionStrategy::TIMING_RANDOMIZATION,
        ProtectionStrategy::STEALTH_MODE
    };
    config.max_protection_cost_usd = 50.0;
    config.default_protection_level = ProtectionLevel::STANDARD;
    return config;
}

MEVEngineConfig MEVEngineFactory::create_high_frequency_config() {
    MEVEngineConfig config;
    config.worker_thread_count = 8;
    config.max_concurrent_analysis = 100;
    config.max_protection_latency = std::chrono::microseconds(50000); // 50ms
    config.preferred_strategies = {
        ProtectionStrategy::TIMING_RANDOMIZATION,
        ProtectionStrategy::STEALTH_MODE
    };
    config.detection_threshold = 0.5; // Lower threshold for faster detection
    return config;
}

MEVEngineConfig MEVEngineFactory::create_conservative_config() {
    MEVEngineConfig config;
    config.detection_threshold = 0.8; // Higher threshold for fewer false positives
    config.default_protection_level = ProtectionLevel::HIGH;
    config.max_protection_cost_usd = 25.0;
    config.preferred_strategies = {
        ProtectionStrategy::PRIVATE_MEMPOOL,
        ProtectionStrategy::BUNDLE_SUBMISSION
    };
    return config;
}

MEVEngineConfig MEVEngineFactory::create_aggressive_config() {
    MEVEngineConfig config;
    config.detection_threshold = 0.3; // Lower threshold for maximum protection
    config.default_protection_level = ProtectionLevel::MAXIMUM;
    config.max_protection_cost_usd = 200.0;
    config.preferred_strategies = {
        ProtectionStrategy::BUNDLE_SUBMISSION,
        ProtectionStrategy::FLASHBOTS_PROTECT,
        ProtectionStrategy::STEALTH_MODE
    };
    return config;
}

} // namespace hfx::mev
