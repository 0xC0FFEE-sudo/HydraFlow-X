/**
 * @file sentiment_analyzer.cpp
 * @brief Complete sentiment analysis implementation
 */

#include "../include/sentiment_analyzer.hpp"
#include "hfx-log/include/logger.hpp"
#include <algorithm>
#include <regex>
#include <sstream>
#include <numeric>
#include <unordered_set>

namespace hfx {
namespace sentiment {

// Utility functions
namespace {
    std::vector<std::string> tokenize_text(const std::string& text) {
        std::vector<std::string> tokens;
        std::regex word_regex("[a-zA-Z]+");
        std::sregex_iterator iter(text.begin(), text.end(), word_regex);
        std::sregex_iterator end;
        
        for (; iter != end; ++iter) {
            std::string token = iter->str();
            std::transform(token.begin(), token.end(), token.begin(), ::tolower);
            tokens.push_back(token);
        }
        
        return tokens;
    }
    
    std::string normalize_text(const std::string& text) {
        std::string normalized = text;
        std::transform(normalized.begin(), normalized.end(), normalized.begin(), ::tolower);
        
        // Remove extra whitespace
        std::regex ws_regex("\\s+");
        normalized = std::regex_replace(normalized, ws_regex, " ");
        
        // Remove punctuation except for important sentiment indicators
        std::regex punct_regex("[^a-zA-Z0-9\\s!?]");
        normalized = std::regex_replace(normalized, punct_regex, " ");
        
        return normalized;
    }
}

// SentimentLexicon implementation
SentimentLexicon::SentimentLexicon() {
    initialize_word_scores();
    initialize_emotion_lexicon();
}

void SentimentLexicon::initialize_word_scores() {
    // Positive words with scores
    positive_words_ = {
        {"amazing", 1.5}, {"awesome", 1.4}, {"excellent", 1.3}, {"fantastic", 1.4}, 
        {"great", 1.0}, {"good", 0.8}, {"love", 1.2}, {"like", 0.6}, {"wonderful", 1.3},
        {"perfect", 1.4}, {"best", 1.2}, {"incredible", 1.4}, {"outstanding", 1.3},
        {"superb", 1.3}, {"brilliant", 1.2}, {"marvelous", 1.3}, {"terrific", 1.2},
        {"splendid", 1.1}, {"magnificent", 1.3}, {"phenomenal", 1.4}, {"exceptional", 1.3},
        
        // Crypto/Trading specific positive words
        {"moon", 1.5}, {"bullish", 1.3}, {"pump", 1.2}, {"gains", 1.1}, {"profit", 1.0},
        {"hodl", 0.8}, {"diamond", 1.0}, {"rocket", 1.2}, {"surge", 1.1}, {"breakout", 1.2},
        {"rally", 1.1}, {"momentum", 0.9}, {"uptrend", 1.0}, {"adoption", 0.9}, 
        {"innovation", 0.8}, {"partnership", 0.8}, {"upgrade", 0.7}, {"milestone", 0.8}
    };
    
    // Negative words with scores
    negative_words_ = {
        {"terrible", -1.5}, {"awful", -1.4}, {"horrible", -1.4}, {"bad", -0.8}, 
        {"worst", -1.3}, {"hate", -1.2}, {"dislike", -0.7}, {"poor", -0.9}, 
        {"disappointing", -1.1}, {"pathetic", -1.3}, {"disgusting", -1.4}, 
        {"appalling", -1.4}, {"dreadful", -1.3}, {"atrocious", -1.5}, {"abysmal", -1.4},
        
        // Crypto/Trading specific negative words
        {"crash", -1.4}, {"dump", -1.3}, {"bearish", -1.2}, {"loss", -1.0}, {"losses", -1.0},
        {"rekt", -1.3}, {"fud", -1.1}, {"scam", -1.5}, {"rug", -1.5}, {"rugpull", -1.5},
        {"panic", -1.2}, {"sell", -0.7}, {"selling", -0.8}, {"dip", -0.6}, {"decline", -0.8},
        {"correction", -0.6}, {"volatility", -0.5}, {"uncertain", -0.7}, {"risk", -0.6}
    };
    
    // Build combined word scores map
    for (const auto& pair : positive_words_) {
        word_scores_[pair.first] = pair.second;
    }
    for (const auto& pair : negative_words_) {
        word_scores_[pair.first] = pair.second;
    }
}

void SentimentLexicon::initialize_emotion_lexicon() {
    // Emotion categories for more nuanced analysis
    emotion_words_["fear"] = {
        {"scared", -1.0}, {"afraid", -0.9}, {"terrified", -1.3}, {"worried", -0.8},
        {"anxious", -0.9}, {"panic", -1.2}, {"fearful", -1.0}, {"nervous", -0.7}
    };
    
    emotion_words_["anger"] = {
        {"angry", -1.1}, {"furious", -1.4}, {"mad", -1.0}, {"outraged", -1.3},
        {"irritated", -0.8}, {"frustrated", -0.9}, {"annoyed", -0.7}, {"livid", -1.3}
    };
    
    emotion_words_["joy"] = {
        {"happy", 1.0}, {"excited", 1.2}, {"thrilled", 1.3}, {"delighted", 1.1},
        {"ecstatic", 1.4}, {"cheerful", 0.9}, {"elated", 1.2}, {"overjoyed", 1.3}
    };
    
    emotion_words_["surprise"] = {
        {"surprised", 0.3}, {"shocked", -0.2}, {"amazed", 1.0}, {"astonished", 0.5},
        {"stunned", 0.0}, {"bewildered", -0.3}, {"confused", -0.4}, {"unexpected", 0.1}
    };
    
    emotion_words_["trust"] = {
        {"confident", 0.9}, {"certain", 0.8}, {"sure", 0.7}, {"convinced", 0.8},
        {"believers", 0.6}, {"faith", 0.7}, {"trust", 0.8}, {"reliable", 0.8}
    };
}

double SentimentLexicon::get_word_score(const std::string& word) const {
    auto it = word_scores_.find(word);
    return (it != word_scores_.end()) ? it->second : 0.0;
}

std::unordered_map<std::string, double> SentimentLexicon::get_emotion_scores(const std::string& word) const {
    std::unordered_map<std::string, double> scores;
    
    for (const auto& emotion_category : emotion_words_) {
        auto it = emotion_category.second.find(word);
        if (it != emotion_category.second.end()) {
            scores[emotion_category.first] = it->second;
        }
    }
    
    return scores;
}

// RuleBasedSentimentAnalyzer implementation
RuleBasedSentimentAnalyzer::RuleBasedSentimentAnalyzer() 
    : lexicon_(std::make_unique<SentimentLexicon>()) {
    
    // Initialize negation words
    negation_words_ = {
        "not", "no", "never", "nothing", "nobody", "nowhere", "neither", "nor",
        "none", "hardly", "scarcely", "barely", "don't", "doesn't", "didn't",
        "won't", "wouldn't", "can't", "cannot", "couldn't", "shouldn't", "mustn't"
    };
    
    // Initialize intensifier words
    intensifier_words_ = {
        {"very", 1.5}, {"extremely", 2.0}, {"incredibly", 1.8}, {"really", 1.3},
        {"quite", 1.2}, {"rather", 1.1}, {"pretty", 1.1}, {"totally", 1.6},
        {"absolutely", 1.8}, {"completely", 1.7}, {"utterly", 1.9}, {"highly", 1.4},
        {"super", 1.5}, {"ultra", 1.6}, {"mega", 1.7}, {"insanely", 1.9}
    };
    
    // Add sentiment rules (regex patterns)
    add_sentiment_rule("\\b(to the moon|moon|🚀|💎|hodl)\\b", SentimentType::STRONGLY_POSITIVE, 1.5);
    add_sentiment_rule("\\b(rug pull|scam|dump|crash)\\b", SentimentType::STRONGLY_NEGATIVE, -1.5);
    add_sentiment_rule("\\b(fud|fear|panic sell)\\b", SentimentType::NEGATIVE, -1.0);
    add_sentiment_rule("\\b(bullish|pump|gains|profit)\\b", SentimentType::POSITIVE, 1.0);
}

SentimentScore RuleBasedSentimentAnalyzer::analyze_text(const std::string& text) const {
    std::string normalized = normalize_text(text);
    std::vector<std::string> tokens = tokenize_text(normalized);
    return analyze_tokens(tokens);
}

SentimentScore RuleBasedSentimentAnalyzer::analyze_tokens(const std::vector<std::string>& tokens) const {
    SentimentScore score;
    
    if (tokens.empty()) {
        return score;
    }
    
    // Calculate base sentiment score
    double base_score = calculate_sentiment_score(tokens);
    
    // Apply negation handling
    double negation_adjusted_score = apply_negation_handling(tokens, base_score);
    
    // Apply intensifier handling
    double final_score = apply_intensifier_handling(tokens, negation_adjusted_score);
    
    // Calculate emotion scores
    for (const std::string& token : tokens) {
        auto emotion_scores = lexicon_->get_emotion_scores(token);
        for (const auto& emotion : emotion_scores) {
            score.emotion_scores[emotion.first] += emotion.second;
        }
    }
    
    // Normalize emotion scores
    for (auto& emotion : score.emotion_scores) {
        emotion.second /= tokens.size();
    }
    
    // Set final sentiment values
    score.compound_score = std::clamp(final_score, -1.0, 1.0);
    score.sentiment = classify_sentiment(score.compound_score);
    score.confidence = calculate_confidence(score.compound_score, tokens.size());
    
    return score;
}

double RuleBasedSentimentAnalyzer::calculate_sentiment_score(const std::vector<std::string>& tokens) const {
    double total_score = 0.0;
    int word_count = 0;
    
    for (const std::string& token : tokens) {
        double word_score = lexicon_->get_word_score(token);
        if (word_score != 0.0) {
            total_score += word_score;
            word_count++;
        }
    }
    
    // Apply pattern-based rules
    std::string text = "";
    for (const auto& token : tokens) {
        text += token + " ";
    }
    
    for (const auto& rule : sentiment_rules_) {
        std::regex pattern(std::get<0>(rule));
        if (std::regex_search(text, pattern)) {
            total_score += std::get<2>(rule);
            word_count++;
        }
    }
    
    return word_count > 0 ? total_score / word_count : 0.0;
}

double RuleBasedSentimentAnalyzer::apply_negation_handling(const std::vector<std::string>& tokens, double base_score) const {
    // Simple negation handling: look for negation words within 3 words of sentiment words
    bool negation_found = false;
    
    for (size_t i = 0; i < tokens.size(); ++i) {
        if (negation_words_.count(tokens[i])) {
            // Check next 3 words for sentiment words
            for (size_t j = i + 1; j < std::min(i + 4, tokens.size()); ++j) {
                if (lexicon_->get_word_score(tokens[j]) != 0.0) {
                    negation_found = true;
                    break;
                }
            }
        }
    }
    
    return negation_found ? -base_score * 0.8 : base_score;
}

double RuleBasedSentimentAnalyzer::apply_intensifier_handling(const std::vector<std::string>& tokens, double base_score) const {
    double intensifier_multiplier = 1.0;
    
    for (const std::string& token : tokens) {
        auto it = intensifier_words_.find(token);
        if (it != intensifier_words_.end()) {
            intensifier_multiplier *= it->second;
        }
    }
    
    return base_score * std::min(intensifier_multiplier, 3.0); // Cap at 3x intensification
}

double RuleBasedSentimentAnalyzer::calculate_confidence(double score, size_t token_count) const {
    // Higher confidence for stronger sentiment and more tokens
    double strength_confidence = std::min(std::abs(score), 1.0);
    double length_confidence = std::min(token_count / 20.0, 1.0); // Max confidence at 20 tokens
    
    return (strength_confidence + length_confidence) / 2.0;
}

SentimentType RuleBasedSentimentAnalyzer::classify_sentiment(double score) const {
    if (score >= 0.6) return SentimentType::STRONGLY_POSITIVE;
    else if (score >= 0.2) return SentimentType::POSITIVE;
    else if (score <= -0.6) return SentimentType::STRONGLY_NEGATIVE;
    else if (score <= -0.2) return SentimentType::NEGATIVE;
    else return SentimentType::NEUTRAL;
}

void RuleBasedSentimentAnalyzer::add_sentiment_rule(const std::string& pattern, SentimentType sentiment, double weight) {
    sentiment_rules_.push_back(std::make_tuple(pattern, sentiment, weight));
}

void RuleBasedSentimentAnalyzer::add_negation_word(const std::string& word) {
    negation_words_.insert(word);
}

void RuleBasedSentimentAnalyzer::add_intensifier_word(const std::string& word, double multiplier) {
    intensifier_words_[word] = multiplier;
}

// MLBasedSentimentAnalyzer implementation (stub for now)
MLBasedSentimentAnalyzer::MLBasedSentimentAnalyzer() : model_loaded_(false) {
    // In production, this would initialize ML models (TensorFlow, PyTorch, ONNX, etc.)
    HFX_LOG_INFO("[MLBasedSentimentAnalyzer] Initializing ML sentiment analyzer (stub)");
}

SentimentScore MLBasedSentimentAnalyzer::analyze_text(const std::string& text) const {
    // Placeholder implementation - would use actual ML model in production
    SentimentScore score;
    score.sentiment = SentimentType::NEUTRAL;
    score.confidence = 0.5;
    score.compound_score = 0.0;
    
    if (!model_loaded_) {
        HFX_LOG_WARN("[MLBasedSentimentAnalyzer] ML model not loaded, returning neutral sentiment");
        return score;
    }
    
    // Would perform actual ML inference here
    // For now, return neutral sentiment
    return score;
}

void MLBasedSentimentAnalyzer::train_model(const std::vector<std::pair<std::string, SentimentType>>& training_data) {
    HFX_LOG_INFO("[MLBasedSentimentAnalyzer] Training ML model with " + std::to_string(training_data.size()) + " samples");
    // Placeholder for model training
}

void MLBasedSentimentAnalyzer::load_model(const std::string& model_path) {
    HFX_LOG_INFO("[MLBasedSentimentAnalyzer] Loading ML model from: " + model_path);
    // Placeholder for model loading
    model_loaded_ = true;
}

void MLBasedSentimentAnalyzer::save_model(const std::string& model_path) const {
    HFX_LOG_INFO("[MLBasedSentimentAnalyzer] Saving ML model to: " + model_path);
    // Placeholder for model saving
}

// Main SentimentAnalyzer implementation
class SentimentAnalyzer::AnalyzerImpl {
public:
    std::unique_ptr<RuleBasedSentimentAnalyzer> rule_based_analyzer_;
    std::unique_ptr<MLBasedSentimentAnalyzer> ml_analyzer_;
    SentimentConfig config_;
    std::mutex analysis_mutex_;
    AnalyzerStats stats_;
    
    AnalyzerImpl() {
        rule_based_analyzer_ = std::make_unique<RuleBasedSentimentAnalyzer>();
        ml_analyzer_ = std::make_unique<MLBasedSentimentAnalyzer>();
        stats_.start_time = std::chrono::system_clock::now();
    }
};

SentimentAnalyzer::SentimentAnalyzer(const SentimentConfig& config)
    : pimpl_(std::make_unique<AnalyzerImpl>()) {
    pimpl_->config_ = config;
    HFX_LOG_INFO("[SentimentAnalyzer] Initialized with hybrid analysis approach");
}

SentimentAnalyzer::~SentimentAnalyzer() = default;

TextAnalysisResult SentimentAnalyzer::analyze_text(const std::string& text, const std::string& text_id) {
    std::lock_guard<std::mutex> lock(pimpl_->analysis_mutex_);
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    TextAnalysisResult result;
    result.text_id = text_id.empty() ? std::to_string(std::hash<std::string>{}(text)) : text_id;
    result.original_text = text;
    result.analyzed_at = std::chrono::system_clock::now();
    
    if (text.empty()) {
        pimpl_->stats_.failed_analyses.fetch_add(1);
        return result;
    }
    
    // Use rule-based analyzer (primary)
    SentimentScore rule_score = pimpl_->rule_based_analyzer_->analyze_text(text);
    
    // Use ML analyzer if available (secondary)
    SentimentScore ml_score = pimpl_->ml_analyzer_->analyze_text(text);
    
    // Combine scores based on configuration
    result.sentiment_score = combine_sentiment_scores(rule_score, ml_score);
    
    // Extract keywords and entities (simplified)
    result.keywords = extract_keywords(text);
    result.entities = extract_entities(text);
    
    // Update statistics
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    
    pimpl_->stats_.total_analyses.fetch_add(1);
    pimpl_->stats_.successful_analyses.fetch_add(1);
    pimpl_->stats_.avg_analysis_time_us.store(duration.count());
    pimpl_->stats_.last_analysis_time = std::chrono::system_clock::now();
    
    return result;
}

MarketSentimentData SentimentAnalyzer::analyze_market_text(const std::string& text, 
                                                         const std::string& symbol, 
                                                         const std::string& source) {
    TextAnalysisResult text_result = analyze_text(text);
    
    MarketSentimentData market_data;
    market_data.symbol = symbol;
    market_data.source = source;
    market_data.sentiment_score = text_result.sentiment_score;
    market_data.mention_count = 1;
    market_data.sentiment_volume = std::abs(text_result.sentiment_score.compound_score);
    market_data.timestamp = text_result.analyzed_at;
    market_data.top_keywords = text_result.keywords;
    
    return market_data;
}

SentimentScore SentimentAnalyzer::combine_sentiment_scores(const SentimentScore& rule_score, 
                                                         const SentimentScore& ml_score) {
    SentimentScore combined;
    
    // Weight the scores based on configuration
    double rule_weight = pimpl_->config_.rule_based_weight;
    double ml_weight = pimpl_->config_.ml_weight;
    double total_weight = rule_weight + ml_weight;
    
    if (total_weight > 0) {
        rule_weight /= total_weight;
        ml_weight /= total_weight;
    }
    
    // Combine compound scores
    combined.compound_score = (rule_score.compound_score * rule_weight) + 
                             (ml_score.compound_score * ml_weight);
    
    // Combine confidences
    combined.confidence = (rule_score.confidence * rule_weight) + 
                         (ml_score.confidence * ml_weight);
    
    // Use rule-based classification as primary
    combined.sentiment = rule_score.sentiment;
    
    // Combine emotion scores
    for (const auto& emotion : rule_score.emotion_scores) {
        combined.emotion_scores[emotion.first] = emotion.second * rule_weight;
    }
    for (const auto& emotion : ml_score.emotion_scores) {
        combined.emotion_scores[emotion.first] += emotion.second * ml_weight;
    }
    
    return combined;
}

std::vector<std::string> SentimentAnalyzer::extract_keywords(const std::string& text) {
    std::vector<std::string> keywords;
    
    // Extract crypto/finance related keywords
    std::regex crypto_keywords("\\b(bitcoin|btc|ethereum|eth|solana|sol|doge|shiba|inu|pump|dump|moon|hodl|fud|fomo|defi|nft|dao|yield|staking|mining|trading|crypto|blockchain|altcoin|memecoin)\\b");
    
    std::sregex_iterator iter(text.begin(), text.end(), crypto_keywords);
    std::sregex_iterator end;
    
    std::unordered_set<std::string> unique_keywords;
    for (; iter != end; ++iter) {
        std::string keyword = iter->str();
        std::transform(keyword.begin(), keyword.end(), keyword.begin(), ::tolower);
        unique_keywords.insert(keyword);
    }
    
    keywords.assign(unique_keywords.begin(), unique_keywords.end());
    std::sort(keywords.begin(), keywords.end());
    
    return keywords;
}

std::vector<std::string> SentimentAnalyzer::extract_entities(const std::string& text) {
    std::vector<std::string> entities;
    
    // Extract mentions (@username)
    std::regex mention_regex("@[a-zA-Z0-9_]+");
    std::sregex_iterator iter(text.begin(), text.end(), mention_regex);
    std::sregex_iterator end;
    
    for (; iter != end; ++iter) {
        entities.push_back(iter->str());
    }
    
    // Extract hashtags
    std::regex hashtag_regex("#[a-zA-Z0-9_]+");
    iter = std::sregex_iterator(text.begin(), text.end(), hashtag_regex);
    
    for (; iter != end; ++iter) {
        entities.push_back(iter->str());
    }
    
    return entities;
}

const SentimentAnalyzer::AnalyzerStats& SentimentAnalyzer::get_stats() const {
    return pimpl_->stats_;
}

void SentimentAnalyzer::reset_stats() {
    pimpl_->stats_ = AnalyzerStats{};
    pimpl_->stats_.start_time = std::chrono::system_clock::now();
}

void SentimentAnalyzer::update_config(const SentimentConfig& config) {
    std::lock_guard<std::mutex> lock(pimpl_->analysis_mutex_);
    pimpl_->config_ = config;
    HFX_LOG_INFO("[SentimentAnalyzer] Configuration updated");
}

const SentimentConfig& SentimentAnalyzer::get_config() const {
    return pimpl_->config_;
}

std::vector<MarketSentimentData> SentimentAnalyzer::batch_analyze_market_texts(
    const std::vector<std::tuple<std::string, std::string, std::string>>& text_data) {
    
    std::vector<MarketSentimentData> results;
    results.reserve(text_data.size());
    
    for (const auto& data : text_data) {
        const std::string& text = std::get<0>(data);
        const std::string& symbol = std::get<1>(data);
        const std::string& source = std::get<2>(data);
        
        results.push_back(analyze_market_text(text, symbol, source));
    }
    
    return results;
}

} // namespace sentiment
} // namespace hfx