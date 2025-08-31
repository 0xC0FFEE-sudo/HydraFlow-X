/**
 * @file terminal_dashboard.cpp
 * @brief Implementation of matrix-style terminal dashboard for HFT monitoring
 */

#include "terminal_dashboard.hpp"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <cmath>
#include <thread>
#include <chrono>
#include <atomic>
#include <memory>
#include <string>
#include <vector>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <signal.h>

namespace hfx::viz {

// Static theme definitions (removed create_matrix_theme as not used)

std::string TerminalArt::get_logo() {
    return R"(
    ╔═══════════════════════════════════════╗
    ║        ██╗  ██╗██╗   ██╗██████╗        ║
    ║        ██║  ██║╚██╗ ██╔╝██╔══██╗       ║
    ║        ███████║ ╚████╔╝ ██████╔╝       ║
    ║        ██╔══██║  ╚██╔╝  ██╔══██╗       ║
    ║        ██║  ██║   ██║   ██║  ██║       ║
    ║        ╚═╝  ╚═╝   ╚═╝   ╚═╝  ╚═╝       ║
    ║                                       ║
    ║           HydraFlow-X v1.0.0          ║
    ║      Ultra-Low Latency HFT Engine     ║
    ╚═══════════════════════════════════════╝
    )";
}

std::string TerminalArt::get_banner() {
    return R"(
╔════════════════════════════════════════════════════════════════════════════════╗
║  ██╗  ██╗██╗   ██╗██████╗ ██████╗  █████╗ ███████╗██╗      ██████╗ ██╗    ██╗ ║
║  ██║  ██║╚██╗ ██╔╝██╔══██╗██╔══██╗██╔══██╗██╔════╝██║     ██╔═══██╗██║    ██║ ║
║  ███████║ ╚████╔╝ ██║  ██║██████╔╝███████║█████╗  ██║     ██║   ██║██║ █╗ ██║ ║
║  ██╔══██║  ╚██╔╝  ██║  ██║██╔══██╗██╔══██║██╔══╝  ██║     ██║   ██║██║███╗██║ ║
║  ██║  ██║   ██║   ██████╔╝██║  ██║██║  ██║██║     ███████╗╚██████╔╝╚███╔███╔╝ ║
║  ╚═╝  ╚═╝   ╚═╝   ╚═════╝ ╚═╝  ╚═╝╚═╝  ╚═╝╚═╝     ╚══════╝ ╚═════╝  ╚══╝╚══╝  ║
║                                                                                ║
║                        REAL-TIME HFT COMMAND CENTER                           ║
║                     Sub-Nanosecond DeFi Arbitrage Engine                      ║
╚════════════════════════════════════════════════════════════════════════════════╝
    )";
}

std::string TerminalArt::create_progress_bar(float percentage, int width, char fill, char empty) {
    int filled = static_cast<int>(percentage * width / 100.0f);
    filled = std::clamp(filled, 0, width);
    
    std::string bar;
    for (int i = 0; i < filled; ++i) {
        bar += fill;
    }
    for (int i = filled; i < width; ++i) {
        bar += empty;
    }
    
    return bar;
}

std::string TerminalArt::create_gauge(float value, float min_val, float max_val, int width) {
    float normalized = (value - min_val) / (max_val - min_val);
    normalized = std::clamp(normalized, 0.0f, 1.0f);
    
    std::string gauge = "[";
    gauge += create_progress_bar(normalized * 100.0f, width - 2);
    gauge += "]";
    
    return gauge;
}

std::string TerminalArt::create_sparkline(const std::vector<float>& data, int width) {
    if (data.empty() || width <= 0) return std::string(width, ' ');
    
    // Find min/max values
    float min_val = *std::min_element(data.begin(), data.end());
    float max_val = *std::max_element(data.begin(), data.end());
    
    if (max_val == min_val) {
        return std::string(width, '-'); // Flat line
    }
    
    // Unicode block characters for sparkline
    const char* blocks[] = {"▁", "▂", "▃", "▄", "▅", "▆", "▇", "█"};
    const int num_blocks = 8;
    
    std::string sparkline;
    int data_points = std::min(width, static_cast<int>(data.size()));
    
    for (int i = 0; i < data_points; ++i) {
        int data_idx = data.size() - data_points + i; // Take last data_points values
        float normalized = (data[data_idx] - min_val) / (max_val - min_val);
        int block_idx = std::clamp(static_cast<int>(normalized * num_blocks), 0, num_blocks - 1);
        sparkline += blocks[block_idx];
    }
    
    // Pad with spaces if needed
    while (sparkline.length() < static_cast<size_t>(width)) {
        sparkline += " ";
    }
    
    return sparkline;
}

std::string TerminalArt::format_currency(double amount) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2);
    
    if (std::abs(amount) >= 1000000) {
        oss << (amount / 1000000.0) << "M";
    } else if (std::abs(amount) >= 1000) {
        oss << (amount / 1000.0) << "K";
    } else {
        oss << amount;
    }
    
    return oss.str();
}

std::string TerminalArt::format_percentage(double percent) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2) << percent << "%";
    return oss.str();
}

std::string TerminalArt::format_latency(uint64_t nanoseconds) {
    if (nanoseconds >= 1000000000) { // >= 1 second
        return std::to_string(nanoseconds / 1000000000) + "s";
    } else if (nanoseconds >= 1000000) { // >= 1 millisecond
        return std::to_string(nanoseconds / 1000000) + "ms";
    } else if (nanoseconds >= 1000) { // >= 1 microsecond
        return std::to_string(nanoseconds / 1000) + "μs";
    } else {
        return std::to_string(nanoseconds) + "ns";
    }
}

std::string TerminalArt::center_text(const std::string& text, int width) {
    int padding = (width - static_cast<int>(text.length())) / 2;
    padding = std::max(0, padding);
    return std::string(padding, ' ') + text + std::string(width - padding - text.length(), ' ');
}

TerminalDashboard::TerminalDashboard() 
    : theme_({}), current_view_(ViewMode::OVERVIEW), running_(false), paused_(false), 
      should_exit_(false), terminal_width_(80), terminal_height_(24) {
    // Reserve space for data buffers
    pnl_history_.reserve(1000);
    latency_history_.reserve(1000);
    volume_history_.reserve(1000);
    cpu_history_.reserve(1000);
    memory_history_.reserve(1000);
}

TerminalDashboard::TerminalDashboard(const TerminalTheme& theme) 
    : theme_(theme), current_view_(ViewMode::OVERVIEW), running_(false), paused_(false), 
      should_exit_(false), terminal_width_(80), terminal_height_(24) {
    // Reserve space for data buffers
    pnl_history_.reserve(1000);
    latency_history_.reserve(1000);
    volume_history_.reserve(1000);
    cpu_history_.reserve(1000);
    memory_history_.reserve(1000);
}

TerminalDashboard::~TerminalDashboard() {
    shutdown();
}

bool TerminalDashboard::initialize() {
    // Get terminal dimensions
    get_terminal_size();
    
    // Setup terminal for raw mode
    setup_terminal();
    
    std::cout << TerminalColors::CLEAR_SCREEN << TerminalColors::CURSOR_HOME;
    std::cout << TerminalColors::HIDE_CURSOR;
    
    // Display banner
    std::cout << TerminalColors::BRIGHT_CYAN << TerminalArt::get_banner() << TerminalColors::RESET << std::endl;
    
    running_ = true;
    
    return true;
}

void TerminalDashboard::shutdown() {
    if (!running_) return;
    
    should_exit_.store(true);
    running_ = false;
    
    // Join threads
    if (display_thread_ && display_thread_->joinable()) {
        display_thread_->join();
    }
    if (input_thread_ && input_thread_->joinable()) {
        input_thread_->join();
    }
    
    // Restore terminal
    restore_terminal();
    
    std::cout << TerminalColors::SHOW_CURSOR;
    std::cout << TerminalColors::CLEAR_SCREEN << TerminalColors::CURSOR_HOME;
    std::cout << TerminalColors::RESET;
    
    std::cout << "\n" << TerminalColors::BRIGHT_GREEN 
              << "Thank you for using HydraFlow-X!" 
              << TerminalColors::RESET << std::endl;
}

void TerminalDashboard::run() {
    if (!running_) {
        if (!initialize()) {
            return;
        }
    }
    
    // Start threads
    display_thread_ = std::make_unique<std::thread>(&TerminalDashboard::display_loop, this);
    input_thread_ = std::make_unique<std::thread>(&TerminalDashboard::input_loop, this);
    
    std::cout << TerminalColors::BRIGHT_GREEN 
              << "HydraFlow-X Terminal Dashboard Running" 
              << TerminalColors::RESET << std::endl;
    std::cout << TerminalColors::BRIGHT_YELLOW 
              << "Press 'q' to quit, 'h' for help, 't' to cycle themes" 
              << TerminalColors::RESET << std::endl;
    
    // Wait for threads to complete
    if (display_thread_->joinable()) {
        display_thread_->join();
    }
    if (input_thread_->joinable()) {
        input_thread_->join();
    }
}

void TerminalDashboard::set_telemetry_engine(std::shared_ptr<TelemetryEngine> telemetry) {
    telemetry_ = telemetry;
}

void TerminalDashboard::display_loop() {
    const auto frame_duration = std::chrono::microseconds(
        static_cast<int64_t>(1000000.0f / theme_.update_rate));
    
    while (running_ && !should_exit_.load()) {
        const auto frame_start = std::chrono::high_resolution_clock::now();
        
        if (!paused_) {
            // Update data buffers
            update_chart_data();
            
            // Clear screen and render current view
            std::cout << TerminalColors::CURSOR_HOME;
            
            switch (current_view_) {
                case ViewMode::OVERVIEW:
                    render_overview();
                    break;
                case ViewMode::TRADING:
                    render_trading_view();
                    break;
                case ViewMode::RISK:
                    render_risk_view();
                    break;
                case ViewMode::NETWORK:
                    render_network_view();
                    break;
                case ViewMode::PERFORMANCE:
                    render_performance_view();
                    break;
                case ViewMode::LOGS:
                    render_logs_view();
                    break;
                case ViewMode::HELP:
                    render_help_view();
                    break;
            }
            
            render_status_bar();
        }
        
        // Calculate frame timing (simplified)
        const auto frame_end = std::chrono::high_resolution_clock::now();
        [[maybe_unused]] const auto frame_time_us = std::chrono::duration_cast<std::chrono::microseconds>(
            frame_end - frame_start).count();
        
        // Sleep until next frame
        const auto elapsed = std::chrono::high_resolution_clock::now() - frame_start;
        const auto sleep_duration = frame_duration - elapsed;
        if (sleep_duration > std::chrono::nanoseconds::zero()) {
            std::this_thread::sleep_for(sleep_duration);
        }
    }
}

void TerminalDashboard::input_loop() {
    while (running_ && !should_exit_.load()) {
        char key = get_keypress();
        
        switch (key) {
            case 'q':
            case 'Q':
                should_exit_.store(true);
                break;
            case 'p':
            case 'P':
                toggle_pause();
                break;
            case 't':
            case 'T':
                cycle_theme();
                break;
            case 'h':
            case 'H':
                set_view_mode(ViewMode::HELP);
                break;
            case '1':
                set_view_mode(ViewMode::OVERVIEW);
                break;
            case '2':
                set_view_mode(ViewMode::TRADING);
                break;
            case '3':
                set_view_mode(ViewMode::RISK);
                break;
            case '4':
                set_view_mode(ViewMode::NETWORK);
                break;
            case '5':
                set_view_mode(ViewMode::PERFORMANCE);
                break;
            case '6':
                set_view_mode(ViewMode::LOGS);
                break;
            case 27: // ESC key
                should_exit_.store(true);
                break;
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}

void TerminalDashboard::render_overview() {
    // Header
    std::cout << TerminalColors::BRIGHT_CYAN << TerminalColors::BOLD;
    std::cout << TerminalArt::center_text("═══ HYDRAFLOW-X OVERVIEW ═══", terminal_width_) << std::endl;
    std::cout << TerminalColors::RESET;
    
    if (!telemetry_) {
        std::cout << TerminalColors::BRIGHT_RED 
                  << TerminalArt::center_text("⚠️  No telemetry data available", terminal_width_) 
                  << TerminalColors::RESET << std::endl;
        return;
    }
    
    auto snapshot = telemetry_->get_snapshot();
    
    // Key metrics row
    std::cout << TerminalColors::BRIGHT_WHITE << "┌─ Key Metrics ─────────────────────────────────────────────────────┐" << std::endl;
    
    std::cout << "│ " << TerminalColors::BRIGHT_GREEN << "P&L: " 
              << TerminalColors::BRIGHT_WHITE << TerminalArt::format_currency(snapshot.metrics.trading.total_pnl_usd.load())
              << TerminalColors::BRIGHT_WHITE << " │ " 
              << TerminalColors::BRIGHT_CYAN << "Latency: " 
              << TerminalColors::BRIGHT_WHITE << TerminalArt::format_latency(snapshot.metrics.timing.order_execution_latency_ns.load())
              << TerminalColors::BRIGHT_WHITE << " │ "
              << TerminalColors::BRIGHT_YELLOW << "Trades: "
              << TerminalColors::BRIGHT_WHITE << snapshot.metrics.trading.total_trades.load()
              << " │" << std::endl;
    
    std::cout << TerminalColors::BRIGHT_WHITE << "└───────────────────────────────────────────────────────────────────┘" << std::endl;
    
    // P&L Chart
    if (!pnl_history_.empty()) {
        std::cout << std::endl << TerminalColors::BRIGHT_GREEN << "P&L Trend: ";
        std::cout << TerminalColors::BRIGHT_WHITE << TerminalArt::create_sparkline(pnl_history_, 60) << std::endl;
    }
    
    // Latency Chart
    if (!latency_history_.empty()) {
        std::cout << TerminalColors::BRIGHT_CYAN << "Latency:   ";
        std::cout << TerminalColors::BRIGHT_WHITE << TerminalArt::create_sparkline(latency_history_, 60) << std::endl;
    }
    
    // Status indicators
    std::cout << std::endl;
    std::cout << TerminalColors::BRIGHT_WHITE << "Status: ";
    
    if (snapshot.metrics.trading.total_trades.load() > 0) {
        std::cout << TerminalColors::BRIGHT_GREEN << "● ACTIVE ";
    } else {
        std::cout << TerminalColors::BRIGHT_YELLOW << "● IDLE ";
    }
    
    std::cout << TerminalColors::BRIGHT_WHITE << "│ Win Rate: ";
    
    auto total_trades = snapshot.metrics.trading.total_trades.load();
    auto successful_trades = snapshot.metrics.trading.successful_arbitrages.load();
    if (total_trades > 0) {
        double win_rate = (double)successful_trades / total_trades * 100.0;
        if (win_rate >= 60) {
            std::cout << TerminalColors::BRIGHT_GREEN;
        } else if (win_rate >= 40) {
            std::cout << TerminalColors::BRIGHT_YELLOW;
        } else {
            std::cout << TerminalColors::BRIGHT_RED;
        }
        std::cout << TerminalArt::format_percentage(win_rate);
    } else {
        std::cout << TerminalColors::BRIGHT_WHITE << "N/A";
    }
    
    std::cout << TerminalColors::RESET << std::endl;
}

void TerminalDashboard::render_status_bar() {
    // Move to bottom of screen
    std::cout << "\033[" << terminal_height_ << ";1H";
    
    std::cout << TerminalColors::BG_BLUE << TerminalColors::BRIGHT_WHITE;
    
    // View mode indicator
    std::string view_name;
    switch (current_view_) {
        case ViewMode::OVERVIEW: view_name = "OVERVIEW"; break;
        case ViewMode::TRADING: view_name = "TRADING"; break;
        case ViewMode::RISK: view_name = "RISK"; break;
        case ViewMode::NETWORK: view_name = "NETWORK"; break;
        case ViewMode::PERFORMANCE: view_name = "PERFORMANCE"; break;
        case ViewMode::LOGS: view_name = "LOGS"; break;
        case ViewMode::HELP: view_name = "HELP"; break;
    }
    
    std::ostringstream status;
    status << " " << view_name << " │ ";
    
    if (paused_) {
        status << "PAUSED │ ";
    } else {
        status << "LIVE │ ";
    }
    
    status << "FPS: " << std::fixed << std::setprecision(1) << theme_.update_rate << " │ ";
    status << "Mode: LIVE │ ";
    status << "q:quit h:help t:theme p:pause │ ";
    
    std::string status_str = status.str();
    if (status_str.length() < static_cast<size_t>(terminal_width_)) {
        status_str += std::string(terminal_width_ - status_str.length(), ' ');
    }
    
    std::cout << status_str << TerminalColors::RESET << std::endl;
}

void TerminalDashboard::get_terminal_size() {
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    terminal_width_ = w.ws_col;
    terminal_height_ = w.ws_row;
    
    if (terminal_width_ <= 0) terminal_width_ = 80;
    if (terminal_height_ <= 0) terminal_height_ = 24;
}

char TerminalDashboard::get_keypress() {
    char ch = 0;
    if (read(STDIN_FILENO, &ch, 1) == 1) {
        return ch;
    }
    return 0;
}

void TerminalDashboard::setup_terminal() {
    struct termios term;
    tcgetattr(STDIN_FILENO, &term);
    term.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &term);
}

void TerminalDashboard::restore_terminal() {
    struct termios term;
    tcgetattr(STDIN_FILENO, &term);
    term.c_lflag |= (ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &term);
}

void TerminalDashboard::update_chart_data() {
    if (!telemetry_) return;
    
    auto snapshot = telemetry_->get_snapshot();
    
    // Update data buffers
    pnl_history_.push_back(static_cast<float>(snapshot.metrics.trading.total_pnl_usd.load()));
    latency_history_.push_back(static_cast<float>(snapshot.metrics.timing.order_execution_latency_ns.load() / 1000.0f)); // Convert to microseconds
    
    // Keep buffer size reasonable
    const size_t max_size = 1000;
    if (pnl_history_.size() > max_size) {
        pnl_history_.erase(pnl_history_.begin(), pnl_history_.begin() + (pnl_history_.size() - max_size));
    }
    if (latency_history_.size() > max_size) {
        latency_history_.erase(latency_history_.begin(), latency_history_.begin() + (latency_history_.size() - max_size));
    }
}

void TerminalDashboard::toggle_pause() {
    paused_ = !paused_;
    add_log_message(paused_ ? "Dashboard paused" : "Dashboard resumed", 0);
}

void TerminalDashboard::cycle_theme() {
    // Cycle through different color themes
    switch (theme_.style) {
        case TerminalTheme::Style::MATRIX:
            theme_.style = TerminalTheme::Style::CYBERPUNK;
            add_log_message("Theme: Cyberpunk", 0);
            break;
        case TerminalTheme::Style::CYBERPUNK:
            theme_.style = TerminalTheme::Style::HACKER;
            add_log_message("Theme: Hacker", 0);
            break;
        case TerminalTheme::Style::HACKER:
            theme_.style = TerminalTheme::Style::MINIMAL;
            add_log_message("Theme: Minimal", 0);
            break;
        default:
            theme_.style = TerminalTheme::Style::MATRIX;
            add_log_message("Theme: Matrix", 0);
            break;
    }
}

void TerminalDashboard::add_log_message(const std::string& message, int severity) {
    LogEntry entry;
    entry.message = message;
    entry.severity = severity;
    entry.timestamp = std::chrono::steady_clock::now();
    
    log_buffer_.push_back(entry);
    
    // Keep buffer size reasonable
    if (log_buffer_.size() > MAX_LOG_ENTRIES) {
        log_buffer_.erase(log_buffer_.begin());
    }
}

void TerminalDashboard::render_trading_view() {
    std::cout << TerminalColors::BRIGHT_GREEN << TerminalColors::BOLD;
    std::cout << TerminalArt::center_text("═══ TRADING OVERVIEW ═══", terminal_width_) << std::endl;
    std::cout << TerminalColors::RESET;
    std::cout << TerminalColors::BRIGHT_YELLOW << "Trading view - Coming soon!" << TerminalColors::RESET << std::endl;
}

void TerminalDashboard::render_risk_view() {
    std::cout << TerminalColors::BRIGHT_RED << TerminalColors::BOLD;
    std::cout << TerminalArt::center_text("═══ RISK MANAGEMENT ═══", terminal_width_) << std::endl;
    std::cout << TerminalColors::RESET;
    std::cout << TerminalColors::BRIGHT_YELLOW << "Risk view - Coming soon!" << TerminalColors::RESET << std::endl;
}

void TerminalDashboard::render_network_view() {
    std::cout << TerminalColors::BRIGHT_BLUE << TerminalColors::BOLD;
    std::cout << TerminalArt::center_text("═══ NETWORK STATUS ═══", terminal_width_) << std::endl;
    std::cout << TerminalColors::RESET;
    std::cout << TerminalColors::BRIGHT_YELLOW << "Network view - Coming soon!" << TerminalColors::RESET << std::endl;
}

void TerminalDashboard::render_performance_view() {
    std::cout << TerminalColors::BRIGHT_MAGENTA << TerminalColors::BOLD;
    std::cout << TerminalArt::center_text("═══ PERFORMANCE MONITORING ═══", terminal_width_) << std::endl;
    std::cout << TerminalColors::RESET;
    std::cout << TerminalColors::BRIGHT_YELLOW << "Performance view - Coming soon!" << TerminalColors::RESET << std::endl;
}

void TerminalDashboard::render_logs_view() {
    std::cout << TerminalColors::BRIGHT_WHITE << TerminalColors::BOLD;
    std::cout << TerminalArt::center_text("═══ LOG STREAM ═══", terminal_width_) << std::endl;
    std::cout << TerminalColors::RESET;
    
    // Display recent log messages
    int display_count = std::min(static_cast<int>(log_buffer_.size()), terminal_height_ - 5);
    for (int i = log_buffer_.size() - display_count; i < static_cast<int>(log_buffer_.size()); ++i) {
        const auto& entry = log_buffer_[i];
        
        std::string color;
        switch (entry.severity) {
            case 0: color = TerminalColors::BRIGHT_WHITE; break;  // Info
            case 1: color = TerminalColors::BRIGHT_YELLOW; break; // Warning
            case 2: color = TerminalColors::BRIGHT_RED; break;    // Error
            default: color = TerminalColors::BRIGHT_WHITE; break;
        }
        
        std::cout << color << entry.message << TerminalColors::RESET << std::endl;
    }
}

void TerminalDashboard::render_help_view() {
    std::cout << TerminalColors::BRIGHT_CYAN << TerminalColors::BOLD;
    std::cout << TerminalArt::center_text("═══ HELP & SHORTCUTS ═══", terminal_width_) << std::endl;
    std::cout << TerminalColors::RESET << std::endl;
    
    std::cout << TerminalColors::BRIGHT_WHITE << "Navigation:" << TerminalColors::RESET << std::endl;
    std::cout << "  " << TerminalColors::BRIGHT_YELLOW << "1" << TerminalColors::RESET << " - Overview" << std::endl;
    std::cout << "  " << TerminalColors::BRIGHT_YELLOW << "2" << TerminalColors::RESET << " - Trading" << std::endl;
    std::cout << "  " << TerminalColors::BRIGHT_YELLOW << "3" << TerminalColors::RESET << " - Risk Management" << std::endl;
    std::cout << "  " << TerminalColors::BRIGHT_YELLOW << "4" << TerminalColors::RESET << " - Network Status" << std::endl;
    std::cout << "  " << TerminalColors::BRIGHT_YELLOW << "5" << TerminalColors::RESET << " - Performance" << std::endl;
    std::cout << "  " << TerminalColors::BRIGHT_YELLOW << "6" << TerminalColors::RESET << " - Logs" << std::endl;
    std::cout << std::endl;
    
    std::cout << TerminalColors::BRIGHT_WHITE << "Controls:" << TerminalColors::RESET << std::endl;
    std::cout << "  " << TerminalColors::BRIGHT_YELLOW << "p" << TerminalColors::RESET << " - Pause/Resume updates" << std::endl;
    std::cout << "  " << TerminalColors::BRIGHT_YELLOW << "t" << TerminalColors::RESET << " - Cycle themes" << std::endl;
    std::cout << "  " << TerminalColors::BRIGHT_YELLOW << "h" << TerminalColors::RESET << " - Show this help" << std::endl;
    std::cout << "  " << TerminalColors::BRIGHT_YELLOW << "q" << TerminalColors::RESET << " - Quit dashboard" << std::endl;
    std::cout << "  " << TerminalColors::BRIGHT_YELLOW << "ESC" << TerminalColors::RESET << " - Emergency exit" << std::endl;
    std::cout << std::endl;
    
    std::cout << TerminalColors::BRIGHT_GREEN << "Press any view key (1-6) to return to dashboard..." << TerminalColors::RESET << std::endl;
}

void TerminalDashboard::set_view_mode(ViewMode mode) {
    current_view_ = mode;
}

} // namespace hfx::viz
