/**
 * @file terminal_dashboard.hpp
 * @brief Matrix-style terminal dashboard for headless HFT monitoring
 * @version 1.0.0
 * 
 * High-performance terminal interface with real-time updates, ASCII charts,
 * and comprehensive monitoring capabilities for server deployments.
 */

#pragma once

#include "telemetry_engine.hpp"
#include <memory>
#include <string>
#include <vector>
#include <atomic>
#include <thread>
#include <chrono>
#include <functional>

namespace hfx::viz {

/**
 * @brief Terminal color codes and styling
 */
struct TerminalColors {
    // ANSI Color Codes
    static constexpr const char* RESET = "\033[0m";
    static constexpr const char* BOLD = "\033[1m";
    static constexpr const char* DIM = "\033[2m";
    static constexpr const char* UNDERLINE = "\033[4m";
    static constexpr const char* BLINK = "\033[5m";
    static constexpr const char* REVERSE = "\033[7m";
    
    // Foreground Colors
    static constexpr const char* BLACK = "\033[30m";
    static constexpr const char* RED = "\033[31m";
    static constexpr const char* GREEN = "\033[32m";
    static constexpr const char* YELLOW = "\033[33m";
    static constexpr const char* BLUE = "\033[34m";
    static constexpr const char* MAGENTA = "\033[35m";
    static constexpr const char* CYAN = "\033[36m";
    static constexpr const char* WHITE = "\033[37m";
    
    // Bright Colors
    static constexpr const char* BRIGHT_BLACK = "\033[90m";
    static constexpr const char* BRIGHT_RED = "\033[91m";
    static constexpr const char* BRIGHT_GREEN = "\033[92m";
    static constexpr const char* BRIGHT_YELLOW = "\033[93m";
    static constexpr const char* BRIGHT_BLUE = "\033[94m";
    static constexpr const char* BRIGHT_MAGENTA = "\033[95m";
    static constexpr const char* BRIGHT_CYAN = "\033[96m";
    static constexpr const char* BRIGHT_WHITE = "\033[97m";
    
    // Background Colors
    static constexpr const char* BG_BLACK = "\033[40m";
    static constexpr const char* BG_RED = "\033[41m";
    static constexpr const char* BG_GREEN = "\033[42m";
    static constexpr const char* BG_YELLOW = "\033[43m";
    static constexpr const char* BG_BLUE = "\033[44m";
    static constexpr const char* BG_MAGENTA = "\033[45m";
    static constexpr const char* BG_CYAN = "\033[46m";
    static constexpr const char* BG_WHITE = "\033[47m";
    
    // Cursor Control
    static constexpr const char* CLEAR_SCREEN = "\033[2J";
    static constexpr const char* CLEAR_LINE = "\033[2K";
    static constexpr const char* CURSOR_HOME = "\033[H";
    static constexpr const char* CURSOR_UP = "\033[A";
    static constexpr const char* CURSOR_DOWN = "\033[B";
    static constexpr const char* CURSOR_RIGHT = "\033[C";
    static constexpr const char* CURSOR_LEFT = "\033[D";
    static constexpr const char* HIDE_CURSOR = "\033[?25l";
    static constexpr const char* SHOW_CURSOR = "\033[?25h";
    
    // Custom HFT theme colors
    static std::string profit_green() { return BRIGHT_GREEN; }
    static std::string loss_red() { return BRIGHT_RED; }
    static std::string neutral_blue() { return BRIGHT_BLUE; }
    static std::string warning_yellow() { return BRIGHT_YELLOW; }
    static std::string critical_red() { return std::string(RED) + BG_RED; }
    static std::string accent_cyan() { return BRIGHT_CYAN; }
    static std::string matrix_green() { return GREEN; }
};

/**
 * @brief Configuration for terminal dashboard appearance
 */
struct TerminalTheme {
    enum class Style {
        MATRIX,      // Green on black, Matrix-style
        CYBERPUNK,   // Cyan/Purple on black
        HACKER,      // Green/Red on dark
        MINIMAL,     // Simple black and white
        RAINBOW      // Multiple colors
    };
    
    Style style = Style::MATRIX;
    bool use_unicode = true;     // Use Unicode box drawing characters
    bool show_borders = true;    // Show panel borders
    bool use_animations = true;  // Enable text animations
    float update_rate = 10.0f;   // Updates per second
    
    // Chart configuration
    int chart_width = 60;
    int chart_height = 10;
    char chart_fill = '#';
    char chart_empty = '.';
    
    // Progress bar configuration
    char progress_fill = '#';
    char progress_empty = '.';
    int progress_width = 20;
};

/**
 * @brief ASCII art and text formatting utilities
 */
class TerminalArt {
public:
    // Box drawing characters
    struct BoxChars {
        static constexpr const char* TOP_LEFT = "┌";
        static constexpr const char* TOP_RIGHT = "┐";
        static constexpr const char* BOTTOM_LEFT = "└";
        static constexpr const char* BOTTOM_RIGHT = "┘";
        static constexpr const char* HORIZONTAL = "─";
        static constexpr const char* VERTICAL = "│";
        static constexpr const char* CROSS = "┼";
        static constexpr const char* T_DOWN = "┬";
        static constexpr const char* T_UP = "┴";
        static constexpr const char* T_RIGHT = "├";
        static constexpr const char* T_LEFT = "┤";
        
        // Double line variants
        static constexpr const char* DOUBLE_HORIZONTAL = "═";
        static constexpr const char* DOUBLE_VERTICAL = "║";
        static constexpr const char* DOUBLE_TOP_LEFT = "╔";
        static constexpr const char* DOUBLE_TOP_RIGHT = "╗";
        static constexpr const char* DOUBLE_BOTTOM_LEFT = "╚";
        static constexpr const char* DOUBLE_BOTTOM_RIGHT = "╝";
    };
    
    // Generate ASCII art for logo/headers
    static std::string get_logo();
    static std::string get_banner();
    
    // Progress bars and gauges
    static std::string create_progress_bar(float percentage, int width = 20, char fill = '#', char empty = '.');
    static std::string create_gauge(float value, float min_val, float max_val, int width = 30);
    static std::string create_sparkline(const std::vector<float>& data, int width = 40);
    
    // Chart generation
    static std::vector<std::string> create_line_chart(const std::vector<float>& data, int width, int height);
    static std::vector<std::string> create_histogram(const std::vector<float>& data, int width, int height, int bins = 10);
    static std::vector<std::string> create_bar_chart(const std::vector<std::pair<std::string, float>>& data, int width);
    
    // Table formatting
    static std::string create_table_header(const std::vector<std::string>& headers, const std::vector<int>& widths);
    static std::string create_table_row(const std::vector<std::string>& values, const std::vector<int>& widths);
    static std::string create_table_separator(const std::vector<int>& widths);
    
    // Text effects
    static std::string center_text(const std::string& text, int width);
    static std::string pad_right(const std::string& text, int width);
    static std::string pad_left(const std::string& text, int width);
    static std::string truncate(const std::string& text, int max_length);
    
    // Number formatting
    static std::string format_currency(double amount);
    static std::string format_percentage(double percent);
    static std::string format_latency(uint64_t nanoseconds);
    static std::string format_throughput(uint64_t bytes_per_second);
    static std::string format_large_number(uint64_t number);
};

/**
 * @class TerminalDashboard
 * @brief High-performance terminal-based real-time HFT monitoring dashboard
 * 
 * Features:
 * - Real-time metrics display with sub-second updates
 * - ASCII charts and visualizations
 * - Matrix-style animations and effects
 * - Keyboard shortcuts for navigation
 * - Multiple view modes and layouts
 * - Log streaming and alert monitoring
 * - Performance optimized for high-frequency updates
 */
class TerminalDashboard {
public:
    TerminalDashboard(); // Default constructor
    explicit TerminalDashboard(const TerminalTheme& theme);
    ~TerminalDashboard();
    
    // Lifecycle management
    bool initialize();
    void shutdown();
    void run(); // Main event loop
    
    // Data binding
    void set_telemetry_engine(std::shared_ptr<TelemetryEngine> telemetry);
    
    // Display management
    void clear_screen();
    void refresh_display();
    void set_cursor_position(int row, int col);
    
    // View management
    enum class ViewMode {
        OVERVIEW,        // Main dashboard overview
        TRADING,         // Trading-focused view
        RISK,            // Risk management view
        NETWORK,         // Network status view
        PERFORMANCE,     // Performance monitoring
        LOGS,            // Log streaming view
        HELP             // Help and shortcuts
    };
    
    void set_view_mode(ViewMode mode);
    ViewMode get_current_view() const { return current_view_; }
    
    // Interactive features
    void handle_keyboard_input();
    void toggle_pause();
    void cycle_theme();
    void zoom_in();
    void zoom_out();
    void reset_view();
    
    // Logging and alerts
    void add_log_message(const std::string& message, int severity = 0);
    void show_alert(const std::string& message, int severity = 1);
    
    // Configuration
    void set_update_rate(float fps) { theme_.update_rate = fps; }
    void set_theme(const TerminalTheme& theme) { theme_ = theme; }
    
private:
    TerminalTheme theme_;
    std::shared_ptr<TelemetryEngine> telemetry_;
    
    // Terminal state
    ViewMode current_view_ = ViewMode::OVERVIEW;
    bool running_ = false;
    bool paused_ = false;
    std::atomic<bool> should_exit_{false};
    
    // Terminal dimensions
    int terminal_width_ = 80;
    int terminal_height_ = 24;
    
    // Threading
    std::unique_ptr<std::thread> display_thread_;
    std::unique_ptr<std::thread> input_thread_;
    
    // Data buffers for charts
    std::vector<float> pnl_history_;
    std::vector<float> latency_history_;
    std::vector<float> volume_history_;
    std::vector<float> cpu_history_;
    std::vector<float> memory_history_;
    
    // Log and alert buffers
    struct LogEntry {
        std::string message;
        std::chrono::steady_clock::time_point timestamp;
        int severity;
    };
    std::vector<LogEntry> log_buffer_;
    std::vector<LogEntry> alert_buffer_;
    
    static constexpr size_t MAX_LOG_ENTRIES = 1000;
    static constexpr size_t MAX_ALERT_ENTRIES = 100;
    
    // Display functions
    void display_loop();
    void input_loop();
    
    void render_header();
    void render_footer();
    void render_status_bar();
    
    // View rendering functions
    void render_overview();
    void render_trading_view();
    void render_risk_view();
    void render_network_view();
    void render_performance_view();
    void render_logs_view();
    void render_help_view();
    
    // Panel rendering utilities
    void render_metrics_panel(int start_row, int start_col, int width, int height);
    void render_chart_panel(int start_row, int start_col, int width, int height, 
                           const std::vector<float>& data, const std::string& title);
    void render_table_panel(int start_row, int start_col, int width, int height,
                           const std::vector<std::vector<std::string>>& data,
                           const std::vector<std::string>& headers);
    void render_log_panel(int start_row, int start_col, int width, int height);
    
    // Utility functions
    void get_terminal_size();
    void setup_terminal();
    void restore_terminal();
    void move_cursor(int row, int col);
    void print_at(int row, int col, const std::string& text, const std::string& color = "");
    void draw_box(int start_row, int start_col, int width, int height, const std::string& title = "");
    void draw_line(int row, int start_col, int end_col, char character = '-');
    
    // Data update functions
    void update_chart_data();
    void update_metrics_display();
    void process_alerts();
    
    // Animation and effects
    void animate_text(const std::string& text, int row, int col, float duration);
    void show_loading_animation();
    void matrix_rain_effect();
    
    // Keyboard shortcuts
    void show_shortcuts();
    char get_keypress();
    
    // Color and theming
    std::string get_color_for_value(float value, float warn_threshold, float critical_threshold);
    std::string apply_theme_color(const std::string& text, const std::string& element_type);
};

} // namespace hfx::viz
