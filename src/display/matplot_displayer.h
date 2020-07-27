#pragma once

#include <atomic>
#include <map>
#include <string>

#include "net/ping/ping_client.h"

namespace display
{

class MathPlotDisplayer : public net::PingClient::Observer {

public:
  MathPlotDisplayer();
  ~MathPlotDisplayer();

  void Close();

  void OnRTTUpdate(const bool timeout,  const uint64_t sequence_number_, int ttl) override;
  void OnPacketLossUpdate(const uint64_t sequence_number_, const double loss) override;

private:
  void Refresh();

  std::vector<uint64_t> rtt_index_;
  std::vector<uint64_t> rtt_value_;

  std::vector<double> loss_index_;
  std::vector<double> loss_value_;

  std::atomic_bool running_;
};
  
} // namespace display
