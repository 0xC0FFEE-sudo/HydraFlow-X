#pragma once

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <atomic>
#include <mutex>
#include <memory>
#include <chrono>
#include <vector>
#include <deque>
#include <functional>

namespace hfx {
namespace security {

// DDoS attack types
enum class DDoSAttackType {
    SYN_FLOOD = 0,
    UDP_FLOOD,
    HTTP_FLOOD,
    SLOWLORIS,
    CONNECTION_FLOOD,
    REQUEST_FLOOD,
    BOTNET_ATTACK,
    DNS_AMPLIFICATION,
    NTP_AMPLIFICATION,
    MEMCACHED_AMPLIFICATION,
    UNKNOWN
};

// DDoS detection method
enum class DDoSDetectionMethod {
    TRAFFIC_ANALYSIS = 0,
    CONNECTION_ANALYSIS,
    REQUEST_PATTERN_ANALYSIS,
    BEHAVIOR_ANALYSIS,
    STATISTICAL_ANALYSIS,
    MACHINE_LEARNING
};

// DDoS protection action
enum class DDoSProtectionAction {
    ALLOW = 0,
    BLOCK,
    RATE_LIMIT,
    CAPTCHA,
    REDIRECT,
    LOG_ONLY,
    ALERT
};

// DDoS alert
struct DDoSAlert {
    std::string alert_id;
    DDoSAttackType attack_type;
    std::string source_ip;
    std::string description;
    double severity_score;
    std::chrono::system_clock::time_point detected_at;
    std::unordered_map<std::string, std::string> metadata;

    DDoSAlert() : severity_score(0.0) {}
};

// DDoS protection rule
struct DDoSProtectionRule {
    std::string name;
    DDoSAttackType attack_type;
    DDoSDetectionMethod detection_method;
    double threshold;
    std::chrono::seconds monitoring_window;
    DDoSProtectionAction action;
    bool enabled;
    int priority;

    DDoSProtectionRule() : threshold(100.0),
                          monitoring_window(std::chrono::seconds(60)),
                          action(DDoSProtectionAction::RATE_LIMIT),
                          enabled(true),
                          priority(1) {}
};

// DDoS protection configuration
struct DDoSProtectionConfig {
    bool enabled;
    int max_connections_per_ip;
    int max_requests_per_second;
    int max_requests_per_minute;
    int suspicious_request_threshold;
    double anomaly_score_threshold;
    std::chrono::seconds monitoring_window;
    std::chrono::seconds block_duration;
    bool enable_auto_mitigation;
    bool enable_machine_learning;
    std::vector<DDoSProtectionRule> custom_rules;
    std::vector<std::string> trusted_ips;
    std::vector<std::string> trusted_user_agents;

    DDoSProtectionConfig() : enabled(true),
                            max_connections_per_ip(10),
                            max_requests_per_second(100),
                            max_requests_per_minute(1000),
                            suspicious_request_threshold(50),
                            anomaly_score_threshold(0.8),
                            monitoring_window(std::chrono::seconds(60)),
                            block_duration(std::chrono::minutes(15)),
                            enable_auto_mitigation(true),
                            enable_machine_learning(false) {}
};

// Traffic pattern analysis
struct TrafficPattern {
    std::string ip_address;
    std::chrono::system_clock::time_point first_seen;
    std::chrono::system_clock::time_point last_seen;
    int total_requests;
    int requests_per_second;
    int requests_per_minute;
    int connection_count;
    double anomaly_score;
    std::unordered_map<std::string, int> endpoint_counts;
    std::unordered_map<std::string, int> user_agent_counts;
    std::deque<std::chrono::system_clock::time_point> request_timestamps;

    TrafficPattern() : total_requests(0), requests_per_second(0),
                      requests_per_minute(0), connection_count(0),
                      anomaly_score(0.0) {}
};

// DDoS protection class
class DDoSProtection {
public:
    explicit DDoSProtection(const DDoSProtectionConfig& config);
    ~DDoSProtection();

    // Traffic analysis
    DDoSProtectionAction analyze_traffic(const std::string& ip_address,
                                       const std::string& endpoint,
                                       const std::string& user_agent,
                                       const std::string& method);

    // Attack detection
    DDoSAttackType detect_attack_type(const TrafficPattern& pattern) const;
    double calculate_anomaly_score(const TrafficPattern& pattern) const;

    // Protection actions
    void block_ip(const std::string& ip_address, std::chrono::seconds duration);
    void unblock_ip(const std::string& ip_address);
    void rate_limit_ip(const std::string& ip_address, int max_requests_per_minute);
    bool is_ip_blocked(const std::string& ip_address) const;

    // Rule management
    void add_protection_rule(const DDoSProtectionRule& rule);
    void remove_protection_rule(const std::string& rule_name);
    void enable_protection_rule(const std::string& rule_name);
    void disable_protection_rule(const std::string& rule_name);

    // Trusted entities
    void add_trusted_ip(const std::string& ip_address);
    void remove_trusted_ip(const std::string& ip_address);
    void add_trusted_user_agent(const std::string& user_agent);
    void remove_trusted_user_agent(const std::string& user_agent);
    bool is_trusted_ip(const std::string& ip_address) const;
    bool is_trusted_user_agent(const std::string& user_agent) const;

    // Statistics and monitoring
    struct DDoSStats {
        std::atomic<uint64_t> total_requests_analyzed{0};
        std::atomic<uint64_t> suspicious_requests{0};
        std::atomic<uint64_t> blocked_requests{0};
        std::atomic<uint64_t> attacks_detected{0};
        std::atomic<uint64_t> ips_blocked{0};
        std::atomic<uint64_t> false_positives{0};
        std::atomic<size_t> currently_blocked_ips{0};
        std::atomic<size_t> monitored_ips{0};
        std::chrono::system_clock::time_point last_attack_detected;
    };

    DDoSStats get_stats() const;
    void reset_stats();

    // Alert management
    std::vector<DDoSAlert> get_recent_alerts(std::chrono::minutes window = std::chrono::minutes(10)) const;
    void acknowledge_alert(const std::string& alert_id);

    // Configuration
    void update_config(const DDoSProtectionConfig& config);
    DDoSProtectionConfig get_config() const;

    // Cleanup and maintenance
    void cleanup_expired_blocks();
    void cleanup_old_traffic_data();
    void update_traffic_patterns();

    // Machine learning integration (future)
    void train_anomaly_detector(const std::vector<TrafficPattern>& training_data);
    void update_anomaly_model();

private:
    DDoSProtectionConfig config_;
    mutable std::mutex config_mutex_;

    // Traffic monitoring
    std::unordered_map<std::string, TrafficPattern> traffic_patterns_;
    std::unordered_map<std::string, std::chrono::system_clock::time_point> blocked_ips_;
    std::unordered_map<std::string, int> rate_limited_ips_;

    // Trusted entities
    std::unordered_set<std::string> trusted_ips_;
    std::unordered_set<std::string> trusted_user_agents_;

    // Protection rules
    std::unordered_map<std::string, DDoSProtectionRule> protection_rules_;

    // Alerts
    std::vector<DDoSAlert> recent_alerts_;

    // Statistics
    mutable DDoSStats stats_;

    // Mutexes
    mutable std::mutex traffic_mutex_;
    mutable std::mutex blocks_mutex_;
    mutable std::mutex rules_mutex_;
    mutable std::mutex alerts_mutex_;

    // Internal methods
    void update_traffic_pattern(const std::string& ip_address, const std::string& endpoint,
                               const std::string& user_agent, const std::string& method);
    TrafficPattern& get_or_create_traffic_pattern(const std::string& ip_address);
    bool is_attack_pattern(const TrafficPattern& pattern) const;
    DDoSProtectionAction determine_protection_action(const TrafficPattern& pattern,
                                                    DDoSAttackType attack_type) const;
    void generate_alert(DDoSAttackType attack_type, const std::string& ip_address,
                       double severity_score, const std::string& description);
    void apply_protection_action(DDoSProtectionAction action, const std::string& ip_address,
                                DDoSAttackType attack_type);
    const DDoSProtectionRule* get_matching_rule(DDoSAttackType attack_type) const;

    // Statistical analysis
    double calculate_entropy(const std::unordered_map<std::string, int>& distribution) const;
    double calculate_burstiness(const std::deque<std::chrono::system_clock::time_point>& timestamps) const;
    bool detect_suspicious_pattern(const TrafficPattern& pattern) const;

    // Cleanup helpers
    void cleanup_expired_entries(std::unordered_map<std::string, std::chrono::system_clock::time_point>& map);
    void cleanup_old_timestamps(std::deque<std::chrono::system_clock::time_point>& timestamps,
                               std::chrono::seconds max_age);
};

// Utility functions
std::string ddos_attack_type_to_string(DDoSAttackType attack_type);
DDoSAttackType string_to_ddos_attack_type(const std::string& attack_type_str);
std::string ddos_protection_action_to_string(DDoSProtectionAction action);
DDoSProtectionAction string_to_ddos_protection_action(const std::string& action_str);
std::string ddos_detection_method_to_string(DDoSDetectionMethod method);
DDoSDetectionMethod string_to_ddos_detection_method(const std::string& method_str);

// Traffic analysis utilities
double calculate_request_frequency(const std::deque<std::chrono::system_clock::time_point>& timestamps,
                                 std::chrono::seconds window);
bool is_suspicious_user_agent(const std::string& user_agent);
bool is_suspicious_endpoint_pattern(const std::string& endpoint);

} // namespace security
} // namespace hfx
