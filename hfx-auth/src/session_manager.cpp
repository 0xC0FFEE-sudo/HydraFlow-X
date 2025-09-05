/**
 * @file session_manager.cpp
 * @brief Session manager implementation
 */

#include "../include/session_manager.hpp"
#include "../../hfx-log/include/simple_logger.hpp"
#include <random>
#include <algorithm>
#include <thread>

namespace hfx {
namespace auth {

SessionManager::SessionManager(const SessionConfig& config) : config_(config) {
    stats_.last_cleanup = std::chrono::system_clock::now();
    start_cleanup_thread();
    HFX_LOG_INFO("[SessionManager] Initialized with " + std::to_string(config_.default_timeout.count()) + " minute timeout");
}

SessionManager::~SessionManager() {
    stop_cleanup_thread();
}

std::string SessionManager::create_session(const std::string& user_id, const std::string& ip_address,
                                         const std::string& user_agent, bool extended) {
    std::lock_guard<std::mutex> lock(sessions_mutex_);
    
    // Check concurrent session limit
    if (!check_session_limit(user_id)) {
        HFX_LOG_WARN("[SessionManager] Session limit exceeded for user: " + user_id);
        return "";
    }
    
    Session session;
    session.session_id = generate_session_id();
    session.user_id = user_id;
    session.ip_address = ip_address;
    session.user_agent = user_agent;
    session.created_at = std::chrono::system_clock::now();
    session.last_activity = session.created_at;
    session.is_active = true;
    
    // Set expiry time
    auto timeout = extended ? config_.extended_timeout : config_.default_timeout;
    session.expires_at = session.created_at + timeout;
    
    // Store session
    sessions_[session.session_id] = session;
    user_sessions_.emplace(user_id, session.session_id);
    
    // Update statistics
    stats_.total_sessions++;
    stats_.active_sessions++;
    
    HFX_LOG_INFO("[SessionManager] Created session for user: " + user_id + " from IP: " + ip_address);
    return session.session_id;
}

bool SessionManager::validate_session(const std::string& session_id) {
    std::lock_guard<std::mutex> lock(sessions_mutex_);
    
    auto it = sessions_.find(session_id);
    if (it == sessions_.end()) {
        return false;
    }
    
    return is_session_valid(it->second);
}

bool SessionManager::refresh_session(const std::string& session_id) {
    if (!config_.enable_session_renewal) {
        return false;
    }
    
    std::lock_guard<std::mutex> lock(sessions_mutex_);
    
    auto it = sessions_.find(session_id);
    if (it == sessions_.end() || !is_session_valid(it->second)) {
        return false;
    }
    
    // Extend session expiry
    it->second.expires_at = std::chrono::system_clock::now() + config_.default_timeout;
    it->second.last_activity = std::chrono::system_clock::now();
    
    HFX_LOG_DEBUG("[SessionManager] Refreshed session: " + session_id);
    return true;
}

bool SessionManager::invalidate_session(const std::string& session_id) {
    std::lock_guard<std::mutex> lock(sessions_mutex_);
    
    auto it = sessions_.find(session_id);
    if (it == sessions_.end()) {
        return false;
    }
    
    std::string user_id = it->second.user_id;
    
    // Remove from user sessions index
    remove_session_from_indexes(session_id);
    
    // Remove session
    sessions_.erase(it);
    
    if (stats_.active_sessions > 0) {
        stats_.active_sessions--;
    }
    
    HFX_LOG_INFO("[SessionManager] Invalidated session: " + session_id + " for user: " + user_id);
    return true;
}

bool SessionManager::invalidate_user_sessions(const std::string& user_id) {
    std::lock_guard<std::mutex> lock(sessions_mutex_);
    
    // Find all sessions for user
    std::vector<std::string> sessions_to_remove;
    auto range = user_sessions_.equal_range(user_id);
    for (auto it = range.first; it != range.second; ++it) {
        sessions_to_remove.push_back(it->second);
    }
    
    // Remove sessions
    for (const auto& session_id : sessions_to_remove) {
        auto session_it = sessions_.find(session_id);
        if (session_it != sessions_.end()) {
            sessions_.erase(session_it);
            if (stats_.active_sessions > 0) {
                stats_.active_sessions--;
            }
        }
    }
    
    // Remove from user sessions index
    user_sessions_.erase(user_id);
    
    HFX_LOG_INFO("[SessionManager] Invalidated " + std::to_string(sessions_to_remove.size()) + " sessions for user: " + user_id);
    return !sessions_to_remove.empty();
}

bool SessionManager::invalidate_all_sessions() {
    std::lock_guard<std::mutex> lock(sessions_mutex_);
    
    size_t count = sessions_.size();
    sessions_.clear();
    user_sessions_.clear();
    stats_.active_sessions = 0;
    
    HFX_LOG_WARN("[SessionManager] Invalidated all " + std::to_string(count) + " sessions");
    return true;
}

std::optional<Session> SessionManager::get_session(const std::string& session_id) {
    std::lock_guard<std::mutex> lock(sessions_mutex_);
    
    auto it = sessions_.find(session_id);
    if (it != sessions_.end() && is_session_valid(it->second)) {
        return it->second;
    }
    return std::nullopt;
}

std::vector<Session> SessionManager::get_user_sessions(const std::string& user_id) {
    std::lock_guard<std::mutex> lock(sessions_mutex_);
    
    std::vector<Session> result;
    auto range = user_sessions_.equal_range(user_id);
    
    for (auto it = range.first; it != range.second; ++it) {
        auto session_it = sessions_.find(it->second);
        if (session_it != sessions_.end() && is_session_valid(session_it->second)) {
            result.push_back(session_it->second);
        }
    }
    
    return result;
}

std::vector<Session> SessionManager::get_all_sessions() {
    std::lock_guard<std::mutex> lock(sessions_mutex_);
    
    std::vector<Session> result;
    for (const auto& pair : sessions_) {
        if (is_session_valid(pair.second)) {
            result.push_back(pair.second);
        }
    }
    
    return result;
}

bool SessionManager::update_activity(const std::string& session_id) {
    if (!config_.track_activity) {
        return false;
    }
    
    std::lock_guard<std::mutex> lock(sessions_mutex_);
    
    auto it = sessions_.find(session_id);
    if (it == sessions_.end() || !is_session_valid(it->second)) {
        return false;
    }
    
    it->second.last_activity = std::chrono::system_clock::now();
    return true;
}

bool SessionManager::is_session_expired(const std::string& session_id) {
    std::lock_guard<std::mutex> lock(sessions_mutex_);
    
    auto it = sessions_.find(session_id);
    if (it == sessions_.end()) {
        return true;
    }
    
    return !is_session_valid(it->second);
}

std::chrono::system_clock::time_point SessionManager::get_session_expiry(const std::string& session_id) {
    std::lock_guard<std::mutex> lock(sessions_mutex_);
    
    auto it = sessions_.find(session_id);
    if (it != sessions_.end()) {
        return it->second.expires_at;
    }
    
    return std::chrono::system_clock::time_point{};
}

void SessionManager::cleanup_expired_sessions() {
    std::lock_guard<std::mutex> lock(sessions_mutex_);
    
    auto now = std::chrono::system_clock::now();
    std::vector<std::string> expired_sessions;
    
    // Find expired sessions
    for (const auto& pair : sessions_) {
        if (!is_session_valid(pair.second)) {
            expired_sessions.push_back(pair.first);
        }
    }
    
    // Remove expired sessions
    for (const auto& session_id : expired_sessions) {
        auto it = sessions_.find(session_id);
        if (it != sessions_.end()) {
            remove_session_from_indexes(session_id);
            sessions_.erase(it);
            if (stats_.active_sessions > 0) {
                stats_.active_sessions--;
            }
            stats_.expired_sessions++;
        }
    }
    
    stats_.last_cleanup = now;
    
    if (!expired_sessions.empty()) {
        HFX_LOG_DEBUG("[SessionManager] Cleaned up " + std::to_string(expired_sessions.size()) + " expired sessions");
    }
}

void SessionManager::extend_session_timeout(const std::string& session_id, std::chrono::minutes extra_time) {
    std::lock_guard<std::mutex> lock(sessions_mutex_);
    
    auto it = sessions_.find(session_id);
    if (it != sessions_.end() && is_session_valid(it->second)) {
        it->second.expires_at += extra_time;
        HFX_LOG_DEBUG("[SessionManager] Extended session timeout by " + std::to_string(extra_time.count()) + " minutes: " + session_id);
    }
}

SessionManager::SessionStats SessionManager::get_session_stats() const {
    std::lock_guard<std::mutex> lock(sessions_mutex_);
    return stats_;
}

void SessionManager::update_config(const SessionConfig& config) {
    std::lock_guard<std::mutex> lock(sessions_mutex_);
    config_ = config;
    HFX_LOG_INFO("[SessionManager] Configuration updated");
}

const SessionConfig& SessionManager::get_config() const {
    std::lock_guard<std::mutex> lock(sessions_mutex_);
    return config_;
}

bool SessionManager::check_session_limit(const std::string& user_id) {
    auto count = get_user_session_count(user_id);
    return count < config_.max_concurrent_sessions;
}

size_t SessionManager::get_user_session_count(const std::string& user_id) {
    // Called from within locked context
    auto range = user_sessions_.equal_range(user_id);
    size_t count = 0;
    
    for (auto it = range.first; it != range.second; ++it) {
        auto session_it = sessions_.find(it->second);
        if (session_it != sessions_.end() && is_session_valid(session_it->second)) {
            count++;
        }
    }
    
    return count;
}

// Private helper methods
std::string SessionManager::generate_session_id() {
    const std::string chars = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dist(0, chars.size() - 1);
    
    std::string session_id = "sess_";
    session_id.reserve(36);
    
    for (int i = 0; i < 32; ++i) {
        session_id += chars[dist(gen)];
    }
    
    return session_id;
}

bool SessionManager::is_session_valid(const Session& session) const {
    if (!session.is_active) {
        return false;
    }
    
    auto now = std::chrono::system_clock::now();
    return now < session.expires_at;
}

void SessionManager::remove_session_from_indexes(const std::string& session_id) {
    auto session_it = sessions_.find(session_id);
    if (session_it != sessions_.end()) {
        const std::string& user_id = session_it->second.user_id;
        
        // Remove from user sessions multimap
        auto range = user_sessions_.equal_range(user_id);
        for (auto it = range.first; it != range.second; ++it) {
            if (it->second == session_id) {
                user_sessions_.erase(it);
                break;
            }
        }
    }
}

void SessionManager::start_cleanup_thread() {
    cleanup_running_ = true;
    cleanup_thread_ = std::thread(&SessionManager::cleanup_thread_worker, this);
}

void SessionManager::stop_cleanup_thread() {
    cleanup_running_ = false;
    if (cleanup_thread_.joinable()) {
        cleanup_thread_.join();
    }
}

void SessionManager::cleanup_thread_worker() {
    HFX_LOG_DEBUG("[SessionManager] Cleanup thread started");
    
    while (cleanup_running_) {
        try {
            cleanup_expired_sessions();
            update_statistics();
            
            // Sleep for cleanup interval
            std::this_thread::sleep_for(config_.cleanup_interval);
        } catch (const std::exception& e) {
            HFX_LOG_ERROR("[SessionManager] Cleanup thread error: " + std::string(e.what()));
        }
    }
    
    HFX_LOG_DEBUG("[SessionManager] Cleanup thread stopped");
}

void SessionManager::update_statistics() {
    std::lock_guard<std::mutex> lock(sessions_mutex_);
    
    stats_.total_sessions = sessions_.size();
    
    size_t active_count = 0;
    double total_duration = 0.0;
    auto now = std::chrono::system_clock::now();
    
    for (const auto& pair : sessions_) {
        if (is_session_valid(pair.second)) {
            active_count++;
            auto duration = std::chrono::duration_cast<std::chrono::minutes>(
                now - pair.second.created_at);
            total_duration += duration.count();
        }
    }
    
    stats_.active_sessions = active_count;
    stats_.avg_session_duration_minutes = active_count > 0 ? total_duration / active_count : 0.0;
}

} // namespace auth
} // namespace hfx
