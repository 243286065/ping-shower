#pragma once

#include <atomic>
#include <map>
#include <mutex>
#include <string>

#include "net/ping/ping_client.h"

namespace display
{

class MathPlotDisplayer : public net::PingClient::Observer {

public:
  MathPlotDisplayer();
  ~MathPlotDisplayer();

  void Refresh();

private:
  void OnRTTUpdate(const bool timeout,  const uint64_t sequence_number_, int ttl) override;
  void OnPacketLossUpdate(const uint64_t sequence_number_, const double loss) override;
  void OnStop() override;

  std::vector<uint64_t> rtt_index_;
  std::vector<uint64_t> rtt_value_;

  std::vector<double> loss_index_;
  std::vector<double> loss_value_;

  std::atomic_bool running_;
  std::mutex mutex_;
};
  
} // namespace display
