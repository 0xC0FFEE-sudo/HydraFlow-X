#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <atomic>
#include <mutex>
#include <memory>
#include <chrono>
#include <functional>
#include <thread>

namespace hfx {
namespace sentiment {

// Sentiment classification
enum class SentimentType {
    STRONGLY_NEGATIVE = -2,
    NEGATIVE = -1,
    NEUTRAL = 0,
    POSITIVE = 1,
    STRONGLY_POSITIVE = 2
};

// Sentiment score structure
struct SentimentScore {
    SentimentType sentiment;
    double confidence;
    double compound_score; // Overall sentiment intensity (-1 to 1)
    std::unordered_map<std::string, double> emotion_scores;

    SentimentScore() : sentiment(SentimentType::NEUTRAL), confidence(0.0), compound_score(0.0) {}
};

// Text analysis result
struct TextAnalysisResult {
    std::string text_id;
    std::string original_text;
    std::string language;
    SentimentScore sentiment_score;
    std::vector<std::string> keywords;
    std::vector<std::string> entities;
    std::chrono::system_clock::time_point analyzed_at;
    std::unordered_map<std::string, std::string> metadata;

    TextAnalysisResult() : language("en") {}
};

// Market sentiment data
struct MarketSentimentData {
    std::string symbol;
    std::string source; // news, twitter, reddit, etc.
    SentimentScore sentiment_score;
    int mention_count;
    double sentiment_volume; // weighted sentiment score
    std::chrono::system_clock::time_point timestamp;
    std::vector<std::string> top_keywords;
    std::unordered_map<std::string, std::string> metadata;

    MarketSentimentData() : mention_count(0), sentiment_volume(0.0) {}
};

// Sentiment signal for trading
struct SentimentSignal {
    std::string symbol;
    SentimentType sentiment;
    double signal_strength; // 0-1 scale
    std::string reason;
    std::vector<std::string> supporting_evidence;
    std::chrono::system_clock::time_point generated_at;
    double confidence;

    SentimentSignal() : signal_strength(0.0), confidence(0.0) {}
};

// Sentiment analysis configuration
struct SentimentAnalysisConfig {
    bool enabled;
    bool enable_real_time_analysis;
    bool enable_historical_analysis;
    bool enable_social_media_monitoring;
    bool enable_news_monitoring;
    int analysis_interval_seconds;
    int max_text_length;
    std::vector<std::string> supported_languages;
    std::vector<std::string> monitored_symbols;
    std::vector<std::string> sentiment_sources;
    double sentiment_threshold; // Minimum confidence for signals
    int max_keywords_per_text;
    bool enable_emotion_analysis;
    std::unordered_map<std::string, double> source_weights;

    SentimentAnalysisConfig() : enabled(true),
                               enable_real_time_analysis(true),
                               enable_historical_analysis(false),
                               enable_social_media_monitoring(true),
                               enable_news_monitoring(true),
                               analysis_interval_seconds(300),
                               max_text_length(10000),
                               sentiment_threshold(0.6),
                               max_keywords_per_text(10),
                               enable_emotion_analysis(false) {
        supported_languages = {"en", "es", "fr", "de", "zh", "ja"};
        monitored_symbols = {"BTC", "ETH", "SOL", "ADA", "DOT", "LINK"};
        sentiment_sources = {"twitter", "reddit", "news", "telegram", "discord"};
    }
};

// Text preprocessing utilities
class TextPreprocessor {
public:
    TextPreprocessor();
    ~TextPreprocessor() = default;

    std::string preprocess_text(const std::string& text) const;
    std::vector<std::string> extract_keywords(const std::string& text, int max_keywords = 10) const;
    std::vector<std::string> extract_entities(const std::string& text) const;
    std::string detect_language(const std::string& text) const;
    std::string remove_noise(const std::string& text) const;
    std::vector<std::string> tokenize(const std::string& text) const;

private:
    std::unordered_set<std::string> stop_words_;
    std::vector<std::string> noise_patterns_;

    void initialize_stop_words();
    void initialize_noise_patterns();
    std::string normalize_text(const std::string& text) const;
};

// Sentiment lexicon (dictionary-based approach)
class SentimentLexicon {
public:
    SentimentLexicon();
    ~SentimentLexicon() = default;

    void load_lexicon(const std::string& file_path);
    void add_word(const std::string& word, double score, SentimentType sentiment);
    double get_word_score(const std::string& word) const;
    SentimentType get_word_sentiment(const std::string& word) const;
    bool has_word(const std::string& word) const;

    // Emotion analysis
    std::unordered_map<std::string, double> analyze_emotions(const std::vector<std::string>& words) const;

private:
    std::unordered_map<std::string, std::pair<double, SentimentType>> lexicon_;
    std::unordered_map<std::string, std::unordered_map<std::string, double>> emotion_lexicon_;

    void initialize_default_lexicon();
    void initialize_emotion_lexicon();
};

// Rule-based sentiment analyzer
class RuleBasedSentimentAnalyzer {
public:
    RuleBasedSentimentAnalyzer();
    ~RuleBasedSentimentAnalyzer() = default;

    SentimentScore analyze_text(const std::string& text) const;
    SentimentScore analyze_tokens(const std::vector<std::string>& tokens) const;

    void add_sentiment_rule(const std::string& pattern, SentimentType sentiment, double weight);
    void add_negation_word(const std::string& word);
    void add_intensifier_word(const std::string& word, double multiplier);
    SentimentType classify_sentiment(double score) const;

private:
    std::unique_ptr<SentimentLexicon> lexicon_;
    std::unordered_set<std::string> negation_words_;
    std::unordered_map<std::string, double> intensifier_words_;
    std::vector<std::tuple<std::string, SentimentType, double>> sentiment_rules_;

    double calculate_sentiment_score(const std::vector<std::string>& tokens) const;
    double apply_negation_handling(const std::vector<std::string>& tokens, double base_score) const;
    double apply_intensifier_handling(const std::vector<std::string>& tokens, double base_score) const;
    double calculate_confidence(double score, size_t token_count) const;
};

// Machine learning-based sentiment analyzer (placeholder for future ML integration)
class MLBasedSentimentAnalyzer {
public:
    MLBasedSentimentAnalyzer();
    ~MLBasedSentimentAnalyzer() = default;

    SentimentScore analyze_text(const std::string& text) const;
    void train_model(const std::vector<std::pair<std::string, SentimentType>>& training_data);
    void load_model(const std::string& model_path);
    void save_model(const std::string& model_path) const;

private:
    bool model_loaded_;
    // Placeholder for ML model integration
    // In production, this would use TensorFlow, PyTorch, or similar
};

// Main sentiment analyzer
class SentimentAnalyzer {
public:
    explicit SentimentAnalyzer(const SentimentAnalysisConfig& config);
    ~SentimentAnalyzer();

    // Text analysis
    TextAnalysisResult analyze_text(const std::string& text,
                                  const std::string& text_id = "",
                                  const std::unordered_map<std::string, std::string>& metadata = {});

    // Batch analysis
    std::vector<TextAnalysisResult> analyze_batch(const std::vector<std::string>& texts,
                                                const std::vector<std::string>& text_ids = {});

    // Market sentiment analysis
    MarketSentimentData analyze_market_sentiment(const std::string& symbol,
                                               const std::vector<std::string>& texts,
                                               const std::string& source);

    // Sentiment signal generation
    SentimentSignal generate_sentiment_signal(const MarketSentimentData& sentiment_data,
                                            const std::string& symbol);

    // Real-time monitoring
    void start_real_time_monitoring();
    void stop_real_time_monitoring();
    bool is_monitoring() const;

    // Configuration
    void update_config(const SentimentAnalysisConfig& config);
    SentimentAnalysisConfig get_config() const;

    // Statistics and monitoring
    struct SentimentStats {
        std::atomic<uint64_t> total_texts_analyzed{0};
        std::atomic<uint64_t> positive_sentiments{0};
        std::atomic<uint64_t> negative_sentiments{0};
        std::atomic<uint64_t> neutral_sentiments{0};
        std::atomic<uint64_t> signals_generated{0};
        std::atomic<uint64_t> analysis_errors{0};
        std::chrono::system_clock::time_point last_analysis;
    };

    const SentimentStats& get_stats() const;
    void reset_stats();

    // Signal callbacks
    using SentimentSignalCallback = std::function<void(const SentimentSignal&)>;
    void register_signal_callback(SentimentSignalCallback callback);

private:
    SentimentAnalysisConfig config_;
    mutable std::mutex config_mutex_;

    std::unique_ptr<TextPreprocessor> text_preprocessor_;
    std::unique_ptr<RuleBasedSentimentAnalyzer> rule_analyzer_;
    std::unique_ptr<MLBasedSentimentAnalyzer> ml_analyzer_;

    // Monitoring
    std::atomic<bool> monitoring_active_;
    std::vector<std::thread> monitoring_threads_;

    // Statistics
    mutable SentimentStats stats_;

    // Callbacks
    std::vector<SentimentSignalCallback> signal_callbacks_;
    mutable std::mutex callbacks_mutex_;

    // Internal methods
    SentimentScore combine_sentiment_scores(const SentimentScore& rule_score,
                                          const SentimentScore& ml_score) const;
    double calculate_sentiment_volume(const std::vector<SentimentScore>& scores) const;
    std::vector<std::string> extract_top_keywords(const std::vector<TextAnalysisResult>& results,
                                                int max_keywords) const;
    void notify_signal_callbacks(const SentimentSignal& signal);
    void update_stats(const TextAnalysisResult& result);
    void monitoring_worker();
};

// Utility functions
std::string sentiment_type_to_string(SentimentType sentiment);
SentimentType string_to_sentiment_type(const std::string& sentiment_str);
std::string format_sentiment_score(const SentimentScore& score);
bool is_strong_sentiment(SentimentType sentiment);
double normalize_sentiment_score(double score);

} // namespace sentiment
} // namespace hfx
