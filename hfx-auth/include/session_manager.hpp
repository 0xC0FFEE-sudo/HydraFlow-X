/**
 * @file session_manager.hpp
 * @brief Session management system for user sessions
 * @version 1.0.0
 */

#pragma once

#include "auth_manager.hpp"
#include <unordered_map>
#include <mutex>
#include <vector>
#include <optional>
#include <string>
#include <chrono>
#include <thread>
#include <atomic>

namespace hfx {
namespace auth {

struct SessionConfig {
    std::chrono::minutes default_timeout = std::chrono::minutes(30);
    std::chrono::minutes cleanup_interval = std::chrono::minutes(5);
    std::chrono::minutes extended_timeout = std::chrono::hours(8);
    size_t max_concurrent_sessions = 10;
    bool enable_session_renewal = true;
    bool track_activity = true;
};

/**
 * @brief Session Manager for handling user sessions
 */
class SessionManager {
public:
    explicit SessionManager(const SessionConfig& config = SessionConfig{});
    ~SessionManager();

    // Session lifecycle
    std::string create_session(const std::string& user_id, const std::string& ip_address,
                              const std::string& user_agent, bool extended = false);
    bool validate_session(const std::string& session_id);
    bool refresh_session(const std::string& session_id);
    bool invalidate_session(const std::string& session_id);
    bool invalidate_user_sessions(const std::string& user_id);
    bool invalidate_all_sessions();

    // Session information
    std::optional<Session> get_session(const std::string& session_id);
    std::vector<Session> get_user_sessions(const std::string& user_id);
    std::vector<Session> get_all_sessions();
    
    // Activity tracking
    bool update_activity(const std::string& session_id);
    bool is_session_expired(const std::string& session_id);
    std::chrono::system_clock::time_point get_session_expiry(const std::string& session_id);

    // Session management
    void cleanup_expired_sessions();
    void extend_session_timeout(const std::string& session_id, std::chrono::minutes extra_time);
    
    // Statistics
    struct SessionStats {
        size_t total_sessions{0};
        size_t active_sessions{0};
        size_t expired_sessions{0};
        size_t sessions_by_user[10]{0}; // Top 10 users by session count
        std::chrono::system_clock::time_point last_cleanup;
        double avg_session_duration_minutes{0.0};
    };
    
    SessionStats get_session_stats() const;

    // Configuration
    void update_config(const SessionConfig& config);
    const SessionConfig& get_config() const;

    // Concurrent session limits
    bool check_session_limit(const std::string& user_id);
    size_t get_user_session_count(const std::string& user_id);

private:
    SessionConfig config_;
    mutable std::mutex sessions_mutex_;
    std::unordered_map<std::string, Session> sessions_;                  // session_id -> Session
    std::unordered_multimap<std::string, std::string> user_sessions_;    // user_id -> session_id
    
    // Background cleanup
    std::atomic<bool> cleanup_running_{false};
    std::thread cleanup_thread_;
    
    // Statistics
    mutable SessionStats stats_;
    
    // Helper methods
    std::string generate_session_id();
    bool is_session_valid(const Session& session) const;
    void remove_session_from_indexes(const std::string& session_id);
    void start_cleanup_thread();
    void stop_cleanup_thread();
    void cleanup_thread_worker();
    void update_statistics();
};

} // namespace auth
} // namespace hfx
