#include <limits>
#include <string>
#include <unordered_set>

#include <stdint.h>

#include "asio.hpp"
#include "base/thread/thread.h"

namespace net {

class PingClient {
public:
  class Observer{
  public:
    virtual void OnRTTUpdate(const bool timeout,  const uint64_t sequence_number_, int ttl) = 0;
    virtual void OnPacketLossUpdate(const uint64_t sequence_number_, const double loss) = 0;
  };

  PingClient(
      const std::string &destination, const int interval = 10 /*s*/);
  ~PingClient();

  void Start();
  void Stop();

  void AddObserver(Observer* observer) {
    notify_thread_.PostTask([=]() {
      observer_list_.insert(observer);
    });
  }

  bool HasObserver(Observer* observer) const {
    return observer_list_.find(observer) != observer_list_.end();
  }

  void RemoveObserver(Observer* observer) {
    notify_thread_.PostTask([=]() {
      if(HasObserver(observer)) {
        observer_list_.erase(observer);
      }
    });
  }

private:
  void StartSend();
  void StartReceive();

  unsigned short GetIdentifier();

  void HandleTimeout();
  void HandleReceive(std::size_t length);

  void NotifyRTTUpdate(const bool timeout,  const uint64_t sequence_number_, int rtt);
  void NotifyPacketLossUpdate(const uint64_t sequence_number_, const double loss);

  asio::io_context io_context_;
  asio::io_service::work io_work_;
  asio::ip::icmp::resolver resolver_;
  asio::ip::icmp::endpoint destination_;
  asio::ip::icmp::socket socket_;
  asio::steady_timer timer_;
  std::chrono::steady_clock::time_point time_sent_;
  uint64_t sequence_number_;
  size_t num_replies_;
  size_t num_replies_ok_;
  asio::streambuf reply_buffer_;

  int interval_;
  uint64_t first_sequence_number_;

  base::Thread work_thread_;
  base::Thread notify_thread_;

  std::unordered_set<Observer*> observer_list_;
};

} // namespace net
