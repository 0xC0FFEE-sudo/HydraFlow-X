/**
 * @file test_security_manager.cpp
 * @brief Unit tests for SecurityManager
 */

#include "core-backend/hfx-ultra/include/testing_framework.hpp"
#include "core-backend/hfx-ultra/include/security_manager.hpp"
#include <memory>

using namespace hfx::testing;
using namespace hfx::ultra;

// Test suite for SecurityManager
class SecurityManagerTestSuite : public TestSuite {
public:
    SecurityManagerTestSuite() : TestSuite("SecurityManager") {
        AddTest(std::make_shared<SessionCreationTest>());
        AddTest(std::make_shared<APIKeyManagementTest>());
        AddTest(std::make_shared<RateLimitingTest>());
        AddTest(std::make_shared<InputValidationTest>());
        AddTest(std::make_shared<PerformanceTest>());
    }
    
    void SetUpSuite() override {
        std::cout << "🔒 Setting up SecurityManager test suite" << std::endl;
    }
    
    void TearDownSuite() override {
        std::cout << "🔒 Tearing down SecurityManager test suite" << std::endl;
    }

private:
    // Session creation and management tests
    class SessionCreationTest : public TestCase {
    public:
        SessionCreationTest() : TestCase("SessionCreation", TestCategory::UNIT, TestPriority::HIGH) {
            AddTag("security");
            AddTag("session");
        }
        
        void SetUp() override {
            config_.session_timeout = std::chrono::minutes(5);
            config_.enable_audit_logging = false; // Disable for unit tests
            security_manager_ = std::make_unique<SecurityManager>(config_);
            security_manager_->initialize();
            security_manager_->start();
        }
        
        void TearDown() override {
            security_manager_->stop();
            security_manager_.reset();
        }
        
        void Run() override {
            // Test successful session creation
            std::string session_id = security_manager_->create_session(
                "test_user", "192.168.1.100", "HydraFlow-Test/1.0", 
                AuthMethod::API_KEY, SecurityLevel::AUTHENTICATED
            );
            
            HFX_ASSERT_FALSE(session_id.empty());
            HFX_ASSERT_TRUE(session_id.starts_with("sess_"));
            
            // Test session validation
            HFX_ASSERT_TRUE(security_manager_->validate_session(session_id));
            
            // Test session extension
            HFX_ASSERT_TRUE(security_manager_->extend_session(session_id));
            
            // Test session termination
            HFX_ASSERT_TRUE(security_manager_->terminate_session(session_id));
            HFX_ASSERT_FALSE(security_manager_->validate_session(session_id));
            
            // Test invalid session ID
            HFX_ASSERT_FALSE(security_manager_->validate_session("invalid_session"));
        }
        
    private:
        SecurityConfig config_;
        std::unique_ptr<SecurityManager> security_manager_;
    };
    
    // API key management tests
    class APIKeyManagementTest : public TestCase {
    public:
        APIKeyManagementTest() : TestCase("APIKeyManagement", TestCategory::UNIT, TestPriority::HIGH) {
            AddTag("security");
            AddTag("api_key");
        }
        
        void SetUp() override {
            config_.enable_audit_logging = false;
            security_manager_ = std::make_unique<SecurityManager>(config_);
            security_manager_->initialize();
            security_manager_->start();
        }
        
        void TearDown() override {
            security_manager_->stop();
            security_manager_.reset();
        }
        
        void Run() override {
            auto expires_at = std::chrono::system_clock::now() + std::chrono::hours(24);
            std::unordered_set<std::string> permissions = {"trades:create", "orders:read"};
            
            // Test API key creation
            std::string api_key = security_manager_->create_api_key(
                "test_user", "Test API Key", SecurityLevel::AUTHORIZED, 
                permissions, expires_at
            );
            
            HFX_ASSERT_FALSE(api_key.empty());
            HFX_ASSERT_TRUE(api_key.starts_with("hfx_"));
            HFX_ASSERT_GE(api_key.length(), 44); // "hfx_" + 40 hex chars
            
            // Test API key validation
            HFX_ASSERT_TRUE(security_manager_->validate_api_key(api_key, "192.168.1.100"));
            
            // Test invalid API key
            HFX_ASSERT_FALSE(security_manager_->validate_api_key("invalid_key"));
            HFX_ASSERT_FALSE(security_manager_->validate_api_key("hfx_invalid"));
        }
        
    private:
        SecurityConfig config_;
        std::unique_ptr<SecurityManager> security_manager_;
    };
    
    // Rate limiting tests
    class RateLimitingTest : public TestCase {
    public:
        RateLimitingTest() : TestCase("RateLimiting", TestCategory::UNIT, TestPriority::MEDIUM) {
            AddTag("security");
            AddTag("rate_limit");
        }
        
        void SetUp() override {
            config_.enable_rate_limiting = true;
            config_.per_user_requests_per_minute = 5; // Very low for testing
            config_.enable_audit_logging = false;
            security_manager_ = std::make_unique<SecurityManager>(config_);
            security_manager_->initialize();
            security_manager_->start();
        }
        
        void TearDown() override {
            security_manager_->stop();
            security_manager_.reset();
        }
        
        void Run() override {
            std::string identifier = "test_user";
            std::string endpoint = "/api/test";
            
            // Test normal rate limiting
            for (int i = 0; i < 5; ++i) {
                HFX_ASSERT_TRUE(security_manager_->check_rate_limit(identifier, endpoint));
            }
            
            // This should exceed the rate limit
            HFX_ASSERT_FALSE(security_manager_->check_rate_limit(identifier, endpoint));
            
            // Test rate limit reset
            security_manager_->reset_rate_limit(identifier);
            HFX_ASSERT_TRUE(security_manager_->check_rate_limit(identifier, endpoint));
        }
        
    private:
        SecurityConfig config_;
        std::unique_ptr<SecurityManager> security_manager_;
    };
    
    // Input validation tests
    class InputValidationTest : public TestCase {
    public:
        InputValidationTest() : TestCase("InputValidation", TestCategory::UNIT, TestPriority::MEDIUM) {
            AddTag("security");
            AddTag("validation");
        }
        
        void SetUp() override {
            config_.enable_audit_logging = false;
            security_manager_ = std::make_unique<SecurityManager>(config_);
            security_manager_->initialize();
        }
        
        void TearDown() override {
            security_manager_.reset();
        }
        
        void Run() override {
            // Test email validation
            HFX_ASSERT_TRUE(security_manager_->is_valid_email("user@example.com"));
            HFX_ASSERT_TRUE(security_manager_->is_valid_email("test.user+tag@domain.co.uk"));
            HFX_ASSERT_FALSE(security_manager_->is_valid_email("invalid-email"));
            HFX_ASSERT_FALSE(security_manager_->is_valid_email("@domain.com"));
            HFX_ASSERT_FALSE(security_manager_->is_valid_email("user@"));
            
            // Test IP address validation
            HFX_ASSERT_TRUE(security_manager_->is_valid_ip_address("192.168.1.1"));
            HFX_ASSERT_TRUE(security_manager_->is_valid_ip_address("10.0.0.1"));
            HFX_ASSERT_TRUE(security_manager_->is_valid_ip_address("127.0.0.1"));
            HFX_ASSERT_FALSE(security_manager_->is_valid_ip_address("256.1.1.1"));
            HFX_ASSERT_FALSE(security_manager_->is_valid_ip_address("192.168.1"));
            HFX_ASSERT_FALSE(security_manager_->is_valid_ip_address("not.an.ip.address"));
            
            // Test filename validation
            HFX_ASSERT_TRUE(security_manager_->is_safe_filename("document.pdf"));
            HFX_ASSERT_TRUE(security_manager_->is_safe_filename("image.jpg"));
            HFX_ASSERT_FALSE(security_manager_->is_safe_filename("../../../etc/passwd"));
            HFX_ASSERT_FALSE(security_manager_->is_safe_filename("C:\\Windows\\System32\\cmd.exe"));
            HFX_ASSERT_FALSE(security_manager_->is_safe_filename("file/with/path.txt"));
            
            // Test input sanitization
            std::string malicious_input = "<script>alert('xss')</script>";
            std::string sanitized = security_manager_->sanitize_input(malicious_input);
            HFX_ASSERT_FALSE(sanitized.find("<script>") != std::string::npos);
            HFX_ASSERT_FALSE(sanitized.find("</script>") != std::string::npos);
            
            // Test regex validation
            HFX_ASSERT_TRUE(security_manager_->validate_input("abc123", "[a-z0-9]+"));
            HFX_ASSERT_FALSE(security_manager_->validate_input("ABC123", "[a-z0-9]+"));
            HFX_ASSERT_TRUE(security_manager_->validate_input("user123", "user[0-9]+"));
        }
        
    private:
        SecurityConfig config_;
        std::unique_ptr<SecurityManager> security_manager_;
    };
    
    // Performance benchmark test
    class PerformanceTest : public TestCase {
    public:
        PerformanceTest() : TestCase("SecurityPerformance", TestCategory::PERFORMANCE, TestPriority::LOW) {
            AddTag("security");
            AddTag("performance");
            SetTimeout(std::chrono::seconds(30));
        }
        
        void SetUp() override {
            config_.enable_audit_logging = false; // Disable for performance testing
            config_.enable_rate_limiting = false; // Disable for clean measurements
            security_manager_ = std::make_unique<SecurityManager>(config_);
            security_manager_->initialize();
            security_manager_->start();
        }
        
        void TearDown() override {
            security_manager_->stop();
            security_manager_.reset();
        }
        
        void Run() override {
            const int iterations = 1000;
            
            // Benchmark session creation
            HFX_MEASURE_LATENCY("session_creation", {
                for (int i = 0; i < iterations; ++i) {
                    std::string session_id = security_manager_->create_session(
                        "user_" + std::to_string(i), "192.168.1.100", "Test-Client/1.0",
                        AuthMethod::API_KEY, SecurityLevel::AUTHENTICATED
                    );
                    
                    // Clean up immediately to avoid memory issues
                    security_manager_->terminate_session(session_id);
                }
            });
            
            // Benchmark API key validation
            auto expires_at = std::chrono::system_clock::now() + std::chrono::hours(1);
            std::string api_key = security_manager_->create_api_key(
                "benchmark_user", "Benchmark Key", SecurityLevel::AUTHENTICATED,
                {"test:read"}, expires_at
            );
            
            HFX_MEASURE_LATENCY("api_key_validation", {
                for (int i = 0; i < iterations; ++i) {
                    security_manager_->validate_api_key(api_key, "192.168.1.100");
                }
            });
            
            // Benchmark password hashing
            HFX_MEASURE_LATENCY("password_hashing", {
                for (int i = 0; i < 100; ++i) { // Fewer iterations for expensive operation
                    security_manager_->hash_password("test_password_" + std::to_string(i));
                }
            });
            
            // Benchmark input validation
            HFX_MEASURE_LATENCY("input_validation", {
                for (int i = 0; i < iterations; ++i) {
                    security_manager_->is_valid_email("test" + std::to_string(i) + "@example.com");
                    security_manager_->is_valid_ip_address("192.168.1." + std::to_string(i % 255));
                }
            });
        }
        
    private:
        SecurityConfig config_;
        std::unique_ptr<SecurityManager> security_manager_;
    };
};

// Test runner example
int main() {
    auto test_runner = TestRunnerFactory::CreateUnitTestRunner();
    
    // Configure test runner
    test_runner->SetVerboseMode(true);
    test_runner->SetOutputFormat("console");
    
    // Set up event callbacks
    test_runner->SetTestStartCallback([](const TestContext& context) {
        std::cout << "🚀 Starting test: " << context.test_name << std::endl;
    });
    
    test_runner->SetTestEndCallback([](const TestResult& result) {
        std::string status_emoji = (result.status == TestStatus::PASSED) ? "✅" : "❌";
        std::cout << status_emoji << " Test " << result.test_name 
                  << " completed in " << result.execution_time.count() / 1000000.0 << "ms" << std::endl;
        
        if (result.status != TestStatus::PASSED) {
            std::cout << "   Error: " << result.error_message << std::endl;
        }
    });
    
    // Register test suite
    auto suite = std::make_shared<SecurityManagerTestSuite>();
    test_runner->RegisterTestSuite(suite);
    
    // Run tests
    test_runner->RunAllTests();
    
    // Print statistics
    auto stats = test_runner->GetStatistics();
    std::cout << "\n📊 Test Summary:" << std::endl;
    std::cout << "   Total: " << stats.total_tests << std::endl;
    std::cout << "   Passed: " << stats.passed_tests << std::endl;
    std::cout << "   Failed: " << stats.failed_tests << std::endl;
    std::cout << "   Success Rate: " << stats.success_rate << "%" << std::endl;
    
    return (stats.failed_tests == 0 && stats.error_tests == 0) ? 0 : 1;
}
