#include "hsm_key_manager.hpp"
#include <thread>
#include <random>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <openssl/evp.h>
#include <openssl/ec.h>
#include <openssl/rand.h>

#ifdef __APPLE__
#include <Security/Security.h>
#endif

namespace hfx::ultra {

HSMKeyManager::HSMKeyManager(const HSMConfig& config) 
    : config_(config), maintenance_running_(false) {
}

HSMKeyManager::~HSMKeyManager() {
    shutdown();
}

bool HSMKeyManager::initialize() {
    if (connected_.load()) {
        return true;
    }
    
    // Connect to HSM
    if (!connect_to_hsm()) {
        log_audit_event("HSM_INIT_FAILED", "system", "", "initialize", false, 
                        "Failed to connect to HSM provider");
        return false;
    }
    
    // Start maintenance thread
    start_maintenance_thread();
    
    connected_.store(true);
    log_audit_event("HSM_INITIALIZED", "system", "", "initialize", true, 
                    "HSM key manager initialized successfully");
    
    return true;
}

bool HSMKeyManager::shutdown() {
    if (!connected_.load()) {
        return true;
    }
    
    stop_maintenance_thread();
    
    // Close all active sessions
    {
        std::lock_guard<std::mutex> lock(sessions_mutex_);
        for (const auto& [session_id, session] : active_sessions_) {
            log_audit_event("SESSION_FORCE_CLOSED", session.operator_id, "", 
                          "shutdown", true, "Session closed during shutdown");
        }
        active_sessions_.clear();
    }
    
    disconnect_from_hsm();
    connected_.store(false);
    
    log_audit_event("HSM_SHUTDOWN", "system", "", "shutdown", true, 
                    "HSM key manager shutdown successfully");
    
    return true;
}

std::string HSMKeyManager::create_session(const std::string& operator_id, const std::string& pin) {
    if (!connected_.load()) {
        return "";
    }
    
    // Validate PIN
    if (!validate_pin(pin)) {
        log_audit_event("SESSION_AUTH_FAILED", operator_id, "", "create_session", false, 
                        "Invalid PIN provided");
        metrics_.security_violations.fetch_add(1);
        return "";
    }
    
    std::string session_id = generate_session_id();
    
    HSMSession session;
    session.session_id = session_id;
    session.provider = config_.provider;
    session.created_at = std::chrono::system_clock::now();
    session.last_activity = session.created_at;
    session.is_authenticated.store(true);
    session.max_authorized_level = SecurityLevel::MEDIUM; // Default level
    session.operator_id = operator_id;
    
    {
        std::lock_guard<std::mutex> lock(sessions_mutex_);
        active_sessions_[session_id] = session;
        metrics_.active_sessions.store(active_sessions_.size());
    }
    
    log_audit_event("SESSION_CREATED", operator_id, "", "create_session", true, 
                    "New HSM session created");
    
    return session_id;
}

bool HSMKeyManager::authenticate_session(const std::string& session_id, SecurityLevel max_level) {
    std::lock_guard<std::mutex> lock(sessions_mutex_);
    
    auto it = active_sessions_.find(session_id);
    if (it == active_sessions_.end()) {
        return false;
    }
    
    // Check session timeout
    auto now = std::chrono::system_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::minutes>(now - it->second.last_activity);
    
    if (elapsed > config_.session_timeout) {
        log_audit_event("SESSION_TIMEOUT", it->second.operator_id, "", 
                        "authenticate_session", false, "Session timed out");
        active_sessions_.erase(it);
        metrics_.active_sessions.store(active_sessions_.size());
        return false;
    }
    
    it->second.max_authorized_level = max_level;
    it->second.last_activity = now;
    
    log_audit_event("SESSION_AUTHENTICATED", it->second.operator_id, "", 
                    "authenticate_session", true, "Session authentication updated");
    
    return true;
}

void HSMKeyManager::close_session(const std::string& session_id) {
    std::lock_guard<std::mutex> lock(sessions_mutex_);
    
    auto it = active_sessions_.find(session_id);
    if (it != active_sessions_.end()) {
        log_audit_event("SESSION_CLOSED", it->second.operator_id, "", 
                        "close_session", true, "Session closed normally");
        active_sessions_.erase(it);
        metrics_.active_sessions.store(active_sessions_.size());
    }
}

std::string HSMKeyManager::generate_key(KeyRole role, const std::string& label, 
                                       SecurityLevel level, const std::string& algorithm) {
    if (!connected_.load()) {
        return "";
    }
    
    if (!validate_key_algorithm(algorithm)) {
        log_audit_event("KEY_GEN_FAILED", "system", "", "generate_key", false, 
                        "Invalid algorithm: " + algorithm);
        return "";
    }
    
    std::string key_id = generate_request_id(); // Reuse ID generation
    
    // Create key metadata
    KeyInfo key_info;
    key_info.key_id = key_id;
    key_info.label = label;
    key_info.role = role;
    key_info.security_level = level;
    key_info.algorithm = algorithm;
    key_info.created_at = std::chrono::system_clock::now();
    key_info.expires_at = key_info.created_at + config_.key_rotation_interval;
    key_info.usage_counter = 0;
    key_info.is_active = true;
    key_info.requires_multi_auth = (level >= SecurityLevel::HIGH);
    
    // Set authorized operations based on role
    switch (role) {
        case KeyRole::TRADING_OPERATIONAL:
            key_info.authorized_operations = {"trade", "swap", "cancel"};
            break;
        case KeyRole::MEV_EXECUTION:
            key_info.authorized_operations = {"mev", "arbitrage", "sandwich"};
            break;
        case KeyRole::EMERGENCY_RECOVERY:
            key_info.authorized_operations = {"emergency", "recover", "admin"};
            break;
        case KeyRole::API_AUTHENTICATION:
            key_info.authorized_operations = {"authenticate", "authorize"};
            break;
        default:
            key_info.authorized_operations = {"read"};
            break;
    }
    
    // Generate public key hash (simplified)
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<uint8_t> dis(0, 255);
    for (auto& byte : key_info.public_key_hash) {
        byte = dis(gen);
    }
    
    {
        std::lock_guard<std::mutex> lock(keys_mutex_);
        keys_[key_id] = key_info;
    }
    
    log_audit_event("KEY_GENERATED", "system", key_id, "generate_key", true, 
                    "Key generated for role: " + hsm_utils::key_role_to_string(role));
    
    return key_id;
}

std::vector<KeyInfo> HSMKeyManager::list_keys(KeyRole role) const {
    std::lock_guard<std::mutex> lock(keys_mutex_);
    
    std::vector<KeyInfo> result;
    for (const auto& [key_id, key_info] : keys_) {
        if (role == KeyRole::READ_ONLY || key_info.role == role) {
            result.push_back(key_info);
        }
    }
    
    return result;
}

std::string HSMKeyManager::submit_signing_request(const SigningRequest& request) {
    if (!connected_.load()) {
        return "";
    }
    
    // Risk assessment
    RiskAssessment risk = assess_transaction_risk(request);
    if (!is_transaction_approved(request)) {
        log_audit_event("SIGNING_REJECTED", "system", request.key_id, "submit_signing_request", 
                        false, "Transaction rejected by risk assessment");
        return "";
    }
    
    // Rate limiting check
    if (!check_rate_limits(request.key_id)) {
        log_audit_event("SIGNING_RATE_LIMITED", "system", request.key_id, "submit_signing_request", 
                        false, "Rate limit exceeded");
        return "";
    }
    
    {
        std::lock_guard<std::mutex> lock(requests_mutex_);
        pending_requests_[request.request_id] = request;
    }
    
    // For urgent MEV operations, process immediately
    if (request.urgent && request.operation_type == "mev") {
        auto result = perform_hsm_signing(request.key_id, request.data_to_sign);
        
        {
            std::lock_guard<std::mutex> lock(requests_mutex_);
            completed_results_[request.request_id] = result;
            pending_requests_.erase(request.request_id);
        }
    }
    
    metrics_.total_signing_requests.fetch_add(1);
    log_audit_event("SIGNING_REQUEST_SUBMITTED", "system", request.key_id, 
                    "submit_signing_request", true, "Request ID: " + request.request_id);
    
    return request.request_id;
}

SigningResult HSMKeyManager::get_signing_result(const std::string& request_id) {
    std::lock_guard<std::mutex> lock(requests_mutex_);
    
    auto it = completed_results_.find(request_id);
    if (it != completed_results_.end()) {
        return it->second;
    }
    
    // Check if still pending
    auto pending_it = pending_requests_.find(request_id);
    if (pending_it != pending_requests_.end()) {
        // Process the request now
        auto result = perform_hsm_signing(pending_it->second.key_id, pending_it->second.data_to_sign);
        completed_results_[request_id] = result;
        pending_requests_.erase(pending_it);
        return result;
    }
    
    // Request not found
    SigningResult result{};
    result.request_id = request_id;
    result.success = false;
    result.error_message = "Request not found";
    return result;
}

SigningResult HSMKeyManager::fast_sign(const std::string& key_id, const std::vector<uint8_t>& data,
                                     const std::string& session_id) {
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // Fast path validation (minimal checks for speed)
    if (!validate_session(session_id, SecurityLevel::MEDIUM)) {
        SigningResult result{};
        result.success = false;
        result.error_message = "Invalid session";
        return result;
    }
    
    auto result = perform_hsm_signing(key_id, data);
    
    auto end_time = std::chrono::high_resolution_clock::now();
    result.signing_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    update_signing_metrics(result.signing_time, result.success);
    
    return result;
}

RiskAssessment HSMKeyManager::assess_transaction_risk(const SigningRequest& request) const {
    RiskAssessment assessment{};
    
    // Use custom risk callback if provided
    if (risk_callback_) {
        return risk_callback_(request);
    }
    
    // Default risk assessment
    assessment.risk_score = 0.1; // Low risk by default
    assessment.recommended_level = SecurityLevel::MEDIUM;
    assessment.requires_multi_sig = false;
    assessment.requires_manual_approval = false;
    assessment.max_approved_value_wei = 10000000000000000000ULL; // 10 ETH
    assessment.approval_validity = std::chrono::seconds(300);
    
    // Increase risk based on transaction value
    if (request.value_wei > 1000000000000000000ULL) { // > 1 ETH
        assessment.risk_score += 0.3;
        assessment.recommended_level = SecurityLevel::HIGH;
    }
    
    if (request.value_wei > 10000000000000000000ULL) { // > 10 ETH
        assessment.risk_score += 0.4;
        assessment.requires_multi_sig = true;
        assessment.recommended_level = SecurityLevel::CRITICAL;
    }
    
    // MEV operations have higher risk
    if (request.operation_type == "mev") {
        assessment.risk_score += 0.2;
        if (!request.urgent) {
            assessment.requires_manual_approval = true;
        }
    }
    
    return assessment;
}

bool HSMKeyManager::is_transaction_approved(const SigningRequest& request) const {
    RiskAssessment risk = assess_transaction_risk(request);
    
    // Check if value exceeds maximum
    if (request.value_wei > risk.max_approved_value_wei) {
        return false;
    }
    
    // Check if security level is sufficient
    if (request.required_level < risk.recommended_level) {
        return false;
    }
    
    return true;
}

void HSMKeyManager::set_audit_callback(AuditCallback callback) {
    audit_callback_ = std::move(callback);
}

void HSMKeyManager::set_risk_assessment_callback(RiskAssessmentCallback callback) {
    risk_callback_ = std::move(callback);
}

HSMKeyManager::HSMStatus HSMKeyManager::get_hsm_status() const {
    HSMStatus status{};
    status.is_connected = connected_.load();
    status.is_authenticated = status.is_connected;
    status.provider = config_.provider;
    status.firmware_version = "1.0.0"; // Would be queried from actual HSM
    
    {
        std::lock_guard<std::mutex> lock(sessions_mutex_);
        status.active_sessions = active_sessions_.size();
    }
    
    status.available_key_slots = 1000; // Would be queried from actual HSM
    status.cpu_usage_percent = 15.5;    // Mock values
    status.memory_usage_percent = 42.3;
    status.last_health_check = std::chrono::system_clock::now();
    
    return status;
}

bool HSMKeyManager::perform_health_check() {
    if (!connected_.load()) {
        return false;
    }
    
    // Perform basic connectivity test
    // In real implementation, this would ping the HSM
    
    log_audit_event("HEALTH_CHECK", "system", "", "health_check", true, 
                    "HSM health check completed successfully");
    
    return true;
}

void HSMKeyManager::reset_metrics() {
    metrics_.total_signing_requests.store(0);
    metrics_.successful_signings.store(0);
    metrics_.failed_signings.store(0);
    metrics_.multi_sig_requests.store(0);
    metrics_.avg_signing_time_ms.store(0.0);
    metrics_.security_violations.store(0);
    metrics_.key_rotations.store(0);
}

// Private methods implementation

bool HSMKeyManager::connect_to_hsm() {
    // Implementation would be provider-specific
    switch (config_.provider) {
        case HSMProvider::SOFTWARE_HSM:
            // Initialize software HSM (for testing)
            return true;
            
        case HSMProvider::YUBIKEY_HSM2:
            // Connect to YubiHSM 2
            // Would use YubiHSM SDK
            return true;
            
        case HSMProvider::AWS_CLOUDHSM:
            // Connect to AWS CloudHSM
            // Would use AWS SDK
            return true;
            
        default:
            return false;
    }
}

void HSMKeyManager::disconnect_from_hsm() {
    // Provider-specific disconnect logic
}

bool HSMKeyManager::validate_session(const std::string& session_id, SecurityLevel required_level) const {
    std::lock_guard<std::mutex> lock(sessions_mutex_);
    
    auto it = active_sessions_.find(session_id);
    if (it == active_sessions_.end()) {
        return false;
    }
    
    // Check if session has sufficient authorization level
    if (it->second.max_authorized_level < required_level) {
        return false;
    }
    
    // Check session timeout
    auto now = std::chrono::system_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::minutes>(now - it->second.last_activity);
    
    return elapsed <= config_.session_timeout;
}

std::string HSMKeyManager::generate_request_id() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 15);
    
    std::stringstream ss;
    ss << "req_";
    for (int i = 0; i < 16; ++i) {
        ss << std::hex << dis(gen);
    }
    
    return ss.str();
}

std::string HSMKeyManager::generate_session_id() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 15);
    
    std::stringstream ss;
    ss << "ses_";
    for (int i = 0; i < 32; ++i) {
        ss << std::hex << dis(gen);
    }
    
    return ss.str();
}

void HSMKeyManager::log_audit_event(const std::string& event_type, const std::string& operator_id,
                                   const std::string& key_id, const std::string& operation, 
                                   bool success, const std::string& details) {
    AuditLog log_entry;
    log_entry.timestamp = std::chrono::system_clock::now();
    log_entry.event_type = event_type;
    log_entry.operator_id = operator_id;
    log_entry.key_id = key_id;
    log_entry.operation = operation;
    log_entry.success = success;
    log_entry.details = details;
    
    {
        std::lock_guard<std::mutex> lock(audit_mutex_);
        audit_logs_.push_back(log_entry);
        
        // Keep only recent logs (last 10000 entries)
        if (audit_logs_.size() > 10000) {
            audit_logs_.erase(audit_logs_.begin(), audit_logs_.begin() + 1000);
        }
    }
    
    // Call external audit callback if provided
    if (audit_callback_) {
        audit_callback_(event_type, details);
    }
}

bool HSMKeyManager::check_rate_limits(const std::string& key_id) const {
    // Simplified rate limiting implementation
    return true; // Allow all for now
}

void HSMKeyManager::update_signing_metrics(std::chrono::milliseconds timing, bool success) {
    if (success) {
        metrics_.successful_signings.fetch_add(1);
    } else {
        metrics_.failed_signings.fetch_add(1);
    }
    
    // Update average timing (exponential moving average)
    double current_avg = metrics_.avg_signing_time_ms.load();
    double new_avg = (current_avg * 0.9) + (timing.count() * 0.1);
    metrics_.avg_signing_time_ms.store(new_avg);
}

SigningResult HSMKeyManager::perform_hsm_signing(const std::string& key_id, const std::vector<uint8_t>& data) {
    auto start_time = std::chrono::high_resolution_clock::now();
    
    SigningResult result{};
    result.request_id = generate_request_id();
    
    // Mock signing implementation (would be HSM-specific)
    if (hsm_key_exists(key_id)) {
        // Simulate signature generation
        result.signature.resize(64); // ECDSA signature size
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<uint8_t> dis(0, 255);
        
        for (auto& byte : result.signature) {
            byte = dis(gen);
        }
        
        result.success = true;
        
        // Update key usage counter
        {
            std::lock_guard<std::mutex> lock(keys_mutex_);
            auto it = keys_.find(key_id);
            if (it != keys_.end()) {
                it->second.usage_counter++;
                result.key_usage_counter = it->second.usage_counter;
            }
        }
        
    } else {
        result.success = false;
        result.error_message = "Key not found";
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    result.signing_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    result.hsm_session_id = "mock_session_" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
    
    return result;
}

bool HSMKeyManager::hsm_key_exists(const std::string& key_id) const {
    std::lock_guard<std::mutex> lock(keys_mutex_);
    return keys_.find(key_id) != keys_.end();
}

bool HSMKeyManager::validate_pin(const std::string& pin) const {
    // Basic PIN validation
    return pin.length() >= 6 && pin.length() <= 16;
}

bool HSMKeyManager::validate_key_algorithm(const std::string& algorithm) const {
    static const std::vector<std::string> supported_algorithms = {
        "ECDSA_P256", "ECDSA_P384", "ECDSA_P521", "RSA_2048", "RSA_4096"
    };
    
    return std::find(supported_algorithms.begin(), supported_algorithms.end(), algorithm) 
           != supported_algorithms.end();
}

void HSMKeyManager::start_maintenance_thread() {
    if (maintenance_running_.load()) {
        return;
    }
    
    maintenance_running_.store(true);
    maintenance_thread_ = std::thread([this]() {
        maintenance_worker();
    });
}

void HSMKeyManager::stop_maintenance_thread() {
    if (!maintenance_running_.load()) {
        return;
    }
    
    maintenance_running_.store(false);
    
    if (maintenance_thread_.joinable()) {
        maintenance_thread_.join();
    }
}

void HSMKeyManager::maintenance_worker() {
    while (maintenance_running_.load()) {
        // Cleanup expired sessions
        {
            std::lock_guard<std::mutex> lock(sessions_mutex_);
            auto now = std::chrono::system_clock::now();
            
            for (auto it = active_sessions_.begin(); it != active_sessions_.end();) {
                auto elapsed = std::chrono::duration_cast<std::chrono::minutes>(now - it->second.last_activity);
                if (elapsed > config_.session_timeout) {
                    log_audit_event("SESSION_EXPIRED", it->second.operator_id, "", 
                                    "maintenance", true, "Session expired and cleaned up");
                    it = active_sessions_.erase(it);
                } else {
                    ++it;
                }
            }
            metrics_.active_sessions.store(active_sessions_.size());
        }
        
        // Cleanup old completed results
        {
            std::lock_guard<std::mutex> lock(requests_mutex_);
            // Keep results for 1 hour
            // Implementation would clean up old results
        }
        
        // Perform periodic health check
        perform_health_check();
        
        // Sleep for 60 seconds
        std::this_thread::sleep_for(std::chrono::seconds(60));
    }
}

// Factory implementations
std::unique_ptr<HSMKeyManager> HSMFactory::create_software_hsm(const std::string& key_store_path) {
    HSMConfig config{};
    config.provider = HSMProvider::SOFTWARE_HSM;
    config.connection_params = key_store_path;
    config.admin_pin = "admin123";
    config.operator_pin = "operator123";
    
    return std::make_unique<HSMKeyManager>(config);
}

std::unique_ptr<HSMKeyManager> HSMFactory::create_from_config(const HSMConfig& config) {
    return std::make_unique<HSMKeyManager>(config);
}

// Utility functions
namespace hsm_utils {
    std::string key_role_to_string(KeyRole role) {
        switch (role) {
            case KeyRole::TRADING_MASTER: return "trading_master";
            case KeyRole::TRADING_OPERATIONAL: return "trading_operational";
            case KeyRole::MEV_EXECUTION: return "mev_execution";
            case KeyRole::EMERGENCY_RECOVERY: return "emergency_recovery";
            case KeyRole::API_AUTHENTICATION: return "api_authentication";
            case KeyRole::MULTI_SIG_SIGNER: return "multi_sig_signer";
            case KeyRole::READ_ONLY: return "read_only";
            default: return "unknown";
        }
    }
    
    KeyRole string_to_key_role(const std::string& role_str) {
        if (role_str == "trading_master") return KeyRole::TRADING_MASTER;
        if (role_str == "trading_operational") return KeyRole::TRADING_OPERATIONAL;
        if (role_str == "mev_execution") return KeyRole::MEV_EXECUTION;
        if (role_str == "emergency_recovery") return KeyRole::EMERGENCY_RECOVERY;
        if (role_str == "api_authentication") return KeyRole::API_AUTHENTICATION;
        if (role_str == "multi_sig_signer") return KeyRole::MULTI_SIG_SIGNER;
        return KeyRole::READ_ONLY;
    }
    
    std::string security_level_to_string(SecurityLevel level) {
        switch (level) {
            case SecurityLevel::LOW: return "low";
            case SecurityLevel::MEDIUM: return "medium";
            case SecurityLevel::HIGH: return "high";
            case SecurityLevel::CRITICAL: return "critical";
            default: return "unknown";
        }
    }
    
    SecurityLevel string_to_security_level(const std::string& level_str) {
        if (level_str == "low") return SecurityLevel::LOW;
        if (level_str == "medium") return SecurityLevel::MEDIUM;
        if (level_str == "high") return SecurityLevel::HIGH;
        if (level_str == "critical") return SecurityLevel::CRITICAL;
        return SecurityLevel::MEDIUM;
    }
}

} // namespace hfx::ultra
