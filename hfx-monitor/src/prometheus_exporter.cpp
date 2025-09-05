/**
 * @file prometheus_exporter.cpp
 * @brief Prometheus Exporter Implementation (Stub)
 */

#include "../include/prometheus_exporter.hpp"

namespace hfx {
namespace monitor {

PrometheusExporter::PrometheusExporter() {}
PrometheusExporter::~PrometheusExporter() {}

void PrometheusExporter::export_metric(const std::string& name, double value) {}
void PrometheusExporter::export_histogram(const std::string& name, const std::vector<double>& values) {}
void PrometheusExporter::export_counter(const std::string& name, uint64_t value) {}
std::string PrometheusExporter::get_prometheus_format() const { return ""; }

} // namespace monitor
} // namespace hfx
