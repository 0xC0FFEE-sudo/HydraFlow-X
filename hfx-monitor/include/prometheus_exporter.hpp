#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace hfx {
namespace monitor {

class PrometheusExporter {
public:
    PrometheusExporter();
    ~PrometheusExporter();

    void export_metric(const std::string& name, double value);
    void export_histogram(const std::string& name, const std::vector<double>& values);
    void export_counter(const std::string& name, uint64_t value);
    std::string get_prometheus_format() const;
};

} // namespace monitor
} // namespace hfx
