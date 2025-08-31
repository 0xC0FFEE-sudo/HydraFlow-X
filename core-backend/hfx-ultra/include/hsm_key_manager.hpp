#pragma once

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <atomic>
#include <mutex>
#include <thread>
#include <chrono>
#include <functional>
#include <array>

namespace hfx::ultra {

// HSM provider types
enum class HSMProvider {
    YUBIKEY_HSM2,       // YubiHSM 2
    AWS_CLOUDHSM,       // AWS CloudHSM
    AZURE_DEDICATED,    // Azure Dedicated HSM
    THALES_NETWORK,     // Thales Network HSM
    SAFENET_LUNA,       // SafeNet Luna HSM
    SOFTWARE_HSM        // Software HSM for testing
};

// Key usage roles for strict separation
enum class KeyRole {
    TRADING_MASTER,     // Master trading key (cold storage)
    TRADING_OPERATIONAL,// Hot trading keys
    MEV_EXECUTION,      // MEV-specific execution
    EMERGENCY_RECOVERY, // Emergency recovery keys
    API_AUTHENTICATION, // API authentication
    MULTI_SIG_SIGNER,   // Multi-signature participant
    READ_ONLY          // Read-only access
};

// Security levels for different operations
enum class SecurityLevel {
    LOW,               // Basic operations
    MEDIUM,            // Standard trading
    HIGH,              // Large value transactions
    CRITICAL           // Emergency operations
};

// Key metadata and status
struct KeyInfo {
    std::string key_id;
    std::string label;
    KeyRole role;
    SecurityLevel security_level;
    std::string algorithm;          // e.g., "ECDSA_P256", "RSA_2048"
    std::chrono::system_clock::time_point created_at;
    std::chrono::system_clock::time_point expires_at;
    uint64_t usage_counter;
    bool is_active;
    bool requires_multi_auth;
    std::vector<std::string> authorized_operations;
    std::array<uint8_t, 32> public_key_hash;
};

// Transaction signing request
struct SigningRequest {
    std::string request_id;
    std::string key_id;
    std::vector<uint8_t> data_to_sign;
    std::string operation_type;     // "trade", "mev", "emergency"
    SecurityLevel required_level;
    uint64_t value_wei;            // Transaction value for risk assessment
    std::chrono::system_clock::time_point timestamp;
    std::vector<std::string> approvers; // For multi-auth
    bool urgent;                   // For time-sensitive MEV operations
};

// Signing result
struct SigningResult {
    std::string request_id;
    bool success;
    std::vector<uint8_t> signature;
    std::string error_message;
    std::chrono::milliseconds signing_time;
    std::string hsm_session_id;
    uint32_t key_usage_counter;
};

// Multi-signature configuration
struct MultiSigConfig {
    uint32_t required_signatures;
    uint32_t total_signers;
    std::vector<std::string> signer_key_ids;
    std::chrono::seconds approval_timeout{300}; // 5 minutes default
    bool allow_emergency_bypass;
    uint64_t emergency_threshold_wei;
};

// HSM session management
struct HSMSession {
    std::string session_id;
    HSMProvider provider;
    std::string connection_string;
    std::chrono::system_clock::time_point created_at;
    std::chrono::system_clock::time_point last_activity;
    std::atomic<bool> is_authenticated{false};
    SecurityLevel max_authorized_level;
    std::string operator_id;
    
    // Custom copy constructor and assignment operator
    HSMSession() = default;
    HSMSession(const HSMSession& other) 
        : session_id(other.session_id), provider(other.provider), 
          connection_string(other.connection_string), created_at(other.created_at),
          last_activity(other.last_activity), max_authorized_level(other.max_authorized_level),
          operator_id(other.operator_id), is_authenticated(other.is_authenticated.load()) {}
    HSMSession& operator=(const HSMSession& other) {
        if (this != &other) {
            session_id = other.session_id;
            provider = other.provider;
            connection_string = other.connection_string;
            created_at = other.created_at;
            last_activity = other.last_activity;
            max_authorized_level = other.max_authorized_level;
            operator_id = other.operator_id;
            is_authenticated.store(other.is_authenticated.load());
        }
        return *this;
    }
};

// Risk assessment for transactions
struct RiskAssessment {
    double risk_score;             // 0.0 = safe, 1.0 = maximum risk
    SecurityLevel recommended_level;
    bool requires_multi_sig;
    bool requires_manual_approval;
    std::vector<std::string> risk_factors;
    uint64_t max_approved_value_wei;
    std::chrono::seconds approval_validity;
};

// HSM configuration
struct HSMConfig {
    HSMProvider provider;
    std::string connection_params;
    std::string admin_pin;
    std::string operator_pin;
    
    // Security settings
    uint32_t max_failed_attempts = 3;
    std::chrono::minutes session_timeout{30};
    bool enable_audit_logging = true;
    bool require_dual_auth = true;
    
    // Performance settings
    uint32_t connection_pool_size = 5;
    std::chrono::milliseconds signing_timeout{1000};
    bool enable_key_caching = false; // Security vs performance trade-off
    
    // Key management
    std::chrono::days key_rotation_interval{90};
    bool auto_backup_keys = true;
    std::string backup_location;
    
    // Rate limiting
    uint32_t max_signings_per_minute = 100;
    uint32_t max_signings_per_hour = 5000;
};

// Advanced HSM-based key manager with role separation
class HSMKeyManager {
public:
    using AuditCallback = std::function<void(const std::string& event, const std::string& details)>;
    using RiskAssessmentCallback = std::function<RiskAssessment(const SigningRequest&)>;
    
    explicit HSMKeyManager(const HSMConfig& config);
    ~HSMKeyManager();
    
    // HSM connection and session management
    bool initialize();
    bool shutdown();
    bool is_connected() const { return connected_.load(); }
    
    std::string create_session(const std::string& operator_id, const std::string& pin);
    bool authenticate_session(const std::string& session_id, SecurityLevel max_level);
    void close_session(const std::string& session_id);
    
    // Key lifecycle management
    std::string generate_key(KeyRole role, const std::string& label, 
                           SecurityLevel level, const std::string& algorithm = "ECDSA_P256");
    bool import_key(const std::string& key_data, KeyRole role, const std::string& label);
    bool delete_key(const std::string& key_id, const std::string& admin_session_id);
    std::vector<KeyInfo> list_keys(KeyRole role = KeyRole::READ_ONLY) const;
    
    // Key rotation and backup
    std::string rotate_key(const std::string& old_key_id, const std::string& admin_session_id);
    bool backup_key(const std::string& key_id, const std::string& backup_location);
    bool restore_key(const std::string& backup_data, const std::string& admin_session_id);
    
    // Transaction signing (core functionality)
    std::string submit_signing_request(const SigningRequest& request);
    SigningResult get_signing_result(const std::string& request_id);
    bool cancel_signing_request(const std::string& request_id);
    
    // Fast signing for time-critical MEV operations (< 5ms target)
    SigningResult fast_sign(const std::string& key_id, const std::vector<uint8_t>& data,
                          const std::string& session_id);
    
    // Multi-signature operations
    bool configure_multi_sig(const std::string& policy_id, const MultiSigConfig& config);
    std::string create_multi_sig_request(const SigningRequest& request, const std::string& policy_id);
    bool approve_multi_sig_request(const std::string& request_id, const std::string& signer_key_id,
                                 const std::string& session_id);
    SigningResult finalize_multi_sig(const std::string& request_id);
    
    // Role-based access control
    bool assign_key_role(const std::string& key_id, KeyRole role, const std::string& admin_session_id);
    bool authorize_operation(const std::string& key_id, const std::string& operation,
                           SecurityLevel level) const;
    std::vector<std::string> get_authorized_operations(const std::string& key_id) const;
    
    // Risk management and compliance
    void set_risk_assessment_callback(RiskAssessmentCallback callback);
    RiskAssessment assess_transaction_risk(const SigningRequest& request) const;
    bool is_transaction_approved(const SigningRequest& request) const;
    
    // Emergency procedures
    bool emergency_key_disable(const std::string& key_id, const std::string& reason);
    bool emergency_session_terminate(const std::string& operator_id, const std::string& reason);
    std::vector<std::string> get_emergency_recovery_keys() const;
    
    // Audit and monitoring
    void set_audit_callback(AuditCallback callback);
    struct AuditLog {
        std::chrono::system_clock::time_point timestamp;
        std::string event_type;
        std::string operator_id;
        std::string key_id;
        std::string operation;
        bool success;
        std::string details;
    };
    std::vector<AuditLog> get_audit_logs(std::chrono::hours lookback = std::chrono::hours(24)) const;
    
    // Performance metrics
    struct Metrics {
        std::atomic<uint64_t> total_signing_requests{0};
        std::atomic<uint64_t> successful_signings{0};
        std::atomic<uint64_t> failed_signings{0};
        std::atomic<uint64_t> multi_sig_requests{0};
        std::atomic<double> avg_signing_time_ms{0.0};
        std::atomic<uint64_t> active_sessions{0};
        std::atomic<uint64_t> security_violations{0};
        std::atomic<uint64_t> key_rotations{0};
    };
    
    const Metrics& get_metrics() const { return metrics_; }
    void reset_metrics();
    
    // HSM health and status
    struct HSMStatus {
        bool is_connected;
        bool is_authenticated;
        HSMProvider provider;
        std::string firmware_version;
        uint32_t active_sessions;
        uint32_t available_key_slots;
        double cpu_usage_percent;
        double memory_usage_percent;
        std::chrono::system_clock::time_point last_health_check;
    };
    
    HSMStatus get_hsm_status() const;
    bool perform_health_check();
    
private:
    HSMConfig config_;
    std::atomic<bool> connected_{false};
    mutable Metrics metrics_;
    
    // Session management
    std::unordered_map<std::string, HSMSession> active_sessions_;
    mutable std::mutex sessions_mutex_;
    
    // Key storage and metadata
    std::unordered_map<std::string, KeyInfo> keys_;
    mutable std::mutex keys_mutex_;
    
    // Signing request tracking
    std::unordered_map<std::string, SigningRequest> pending_requests_;
    std::unordered_map<std::string, SigningResult> completed_results_;
    mutable std::mutex requests_mutex_;
    
    // Multi-signature policies
    std::unordered_map<std::string, MultiSigConfig> multi_sig_policies_;
    std::unordered_map<std::string, std::vector<std::string>> multi_sig_approvals_;
    mutable std::mutex multi_sig_mutex_;
    
    // Callbacks
    AuditCallback audit_callback_;
    RiskAssessmentCallback risk_callback_;
    
    // Audit logging
    std::vector<AuditLog> audit_logs_;
    mutable std::mutex audit_mutex_;
    
    // Rate limiting
    std::unordered_map<std::string, std::atomic<uint32_t>> signing_rates_;
    mutable std::mutex rate_limit_mutex_;
    
    // Internal methods
    bool connect_to_hsm();
    void disconnect_from_hsm();
    bool validate_session(const std::string& session_id, SecurityLevel required_level) const;
    bool validate_key_usage(const std::string& key_id, const std::string& operation) const;
    
    std::string generate_request_id();
    std::string generate_session_id();
    
    void log_audit_event(const std::string& event_type, const std::string& operator_id,
                        const std::string& key_id, const std::string& operation, 
                        bool success, const std::string& details = "");
    
    bool check_rate_limits(const std::string& key_id) const;
    void update_signing_metrics(std::chrono::milliseconds timing, bool success);
    
    // HSM-specific implementations (would be provider-specific)
    SigningResult perform_hsm_signing(const std::string& key_id, const std::vector<uint8_t>& data);
    bool hsm_key_exists(const std::string& key_id) const;
    std::vector<uint8_t> get_public_key(const std::string& key_id) const;
    
    // Security validation
    bool validate_pin(const std::string& pin) const;
    bool validate_key_algorithm(const std::string& algorithm) const;
    SecurityLevel calculate_required_security_level(uint64_t value_wei) const;
    
    // Background maintenance
    void start_maintenance_thread();
    void stop_maintenance_thread();
    void maintenance_worker();
    std::atomic<bool> maintenance_running_{false};
    std::thread maintenance_thread_;
};

// Factory for creating HSM managers for different providers
class HSMFactory {
public:
    static std::unique_ptr<HSMKeyManager> create_yubikey_hsm(const std::string& device_path);
    static std::unique_ptr<HSMKeyManager> create_aws_cloudhsm(const std::string& cluster_id);
    static std::unique_ptr<HSMKeyManager> create_azure_hsm(const std::string& vault_url);
    static std::unique_ptr<HSMKeyManager> create_software_hsm(const std::string& key_store_path);
    static std::unique_ptr<HSMKeyManager> create_from_config(const HSMConfig& config);
    
    // HSM discovery and validation
    static std::vector<HSMProvider> discover_available_hsms();
    static bool validate_hsm_connectivity(HSMProvider provider, const std::string& connection_params);
};

// Utility functions for key management
namespace hsm_utils {
    std::string key_role_to_string(KeyRole role);
    KeyRole string_to_key_role(const std::string& role_str);
    std::string security_level_to_string(SecurityLevel level);
    SecurityLevel string_to_security_level(const std::string& level_str);
    
    // Key derivation utilities
    std::string derive_key_label(KeyRole role, const std::string& base_name);
    std::vector<uint8_t> derive_key_material(const std::string& master_key, const std::string& context);
    
    // Validation utilities
    bool validate_key_id_format(const std::string& key_id);
    bool validate_session_id_format(const std::string& session_id);
    bool is_key_role_compatible(KeyRole role, const std::string& operation);
}

} // namespace hfx::ultra
