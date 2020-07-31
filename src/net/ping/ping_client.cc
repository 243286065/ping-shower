#include "ping_client.h"

#include "base/log/logging.h"
#include "net/icmp/icmp_header.h"
#include "net/ipv4/ipv4_header.h"

#include <functional>

namespace net {

const int kDefaultTimeoutThread = 2000; // ,s

PingClient::PingClient(const std::string &destination, const int interval)
    : interval_(interval), io_work_(io_context_), resolver_(io_context_),
      socket_(io_context_, asio::ip::icmp::v4()), timer_(io_context_),
      sequence_number_(0), num_replies_(0), num_replies_timeout_(0),
      first_sequence_number_(sequence_number_ + 1), running_(false) {
  destination_ =
      *resolver_.resolve(asio::ip::icmp::v4(), destination, "").begin();
  work_thread_.Start();
  notify_thread_.Start();
}

PingClient::~PingClient() {}

void PingClient::Start() {
  work_thread_.PostTask([this]() {
    running_ = true;
    StartSend();
    StartReceive();
    io_context_.run();
  });
}

void PingClient::Stop() {
  running_ = false;

  if(socket_.is_open()) {
      socket_.cancel();
      socket_.close();
    }

  timer_.cancel();
  io_context_.stop();

  work_thread_.Stop();
  notify_thread_.Stop();
}

void PingClient::StartSend() {
  if(!running_)
    return;

  std::string body("\"Hello!\" from ping-shower ping.");
  // Create an ICMP header for an echo request.
  icmp_header echo_request;
  echo_request.type(icmp_header::echo_request);
  echo_request.code(0);
  echo_request.identifier(GetIdentifier());
  echo_request.sequence_number(++sequence_number_);
  compute_checksum(echo_request, body.begin(), body.end());

  // Encode the request packet.
  asio::streambuf request_buffer;
  std::ostream os(&request_buffer);
  os << echo_request << body;

  // Send the request.
  time_sent_ = asio::steady_timer::clock_type::now();
  socket_.send_to(request_buffer.data(), destination_);

  // Wait up to 3 seconds for a reply.
  num_replies_ = 0;
  timer_.expires_at(time_sent_ +
                    std::chrono::milliseconds(kDefaultTimeoutThread));
  timer_.async_wait(std::bind(&PingClient::HandleTimeout, this, sequence_number_));
}

void PingClient::StartReceive() {
  if(!running_)
    return;

  // Discard any data already in the buffer.
  reply_buffer_.consume(reply_buffer_.size());

  // Wait for a reply. We prepare the buffer to receive up to 64KB.
  socket_.async_receive(
      reply_buffer_.prepare(65536),
      std::bind(&PingClient::HandleReceive, this, std::placeholders::_2));
}

unsigned short PingClient::GetIdentifier() {
#if defined(ASIO_WINDOWS)
  return static_cast<unsigned short>(::GetCurrentProcessId());
#else
  return static_cast<unsigned short>(::getpid());
#endif
}

void PingClient::HandleTimeout(const uint64_t sequence_number) {
  if (num_replies_ == 0) {
    LOG(INFO) << "Request timed out";
    num_replies_timeout_++;
    notify_thread_.PostTask([=]() {
      NotifyRTTUpdate(true, sequence_number, kDefaultTimeoutThread);
    });
  }

  // Requests must be sent no less than one second apart.
  timer_.expires_at(time_sent_ + std::chrono::seconds(1));
  timer_.async_wait(std::bind(&PingClient::StartSend, this));
}

void PingClient::HandleReceive(std::size_t length) {
  // The actual number of bytes received is committed to the buffer so that we
  // can extract it using a std::istream object.
  reply_buffer_.commit(length);

  // Decode the reply packet.
  std::istream is(&reply_buffer_);
  ipv4_header ipv4_hdr;
  icmp_header icmp_hdr;
  is >> ipv4_hdr >> icmp_hdr;

  // We can receive all ICMP packets received by the host, so we need to
  // filter out only the echo replies that match the our identifier and
  // expected sequence number.
  if (is && icmp_hdr.type() == icmp_header::echo_reply &&
      icmp_hdr.identifier() == GetIdentifier() &&
      icmp_hdr.sequence_number() == sequence_number_) {
    // If this is the first reply, interrupt the five second timeout.
    if (num_replies_++ == 0)
      timer_.cancel();

    // Print out some information about the reply packet.
    std::chrono::steady_clock::time_point now =
        std::chrono::steady_clock::now();
    std::chrono::steady_clock::duration elapsed = now - time_sent_;
    auto rtt =
        std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();
    LOG(INFO) << length - ipv4_hdr.header_length() << " bytes from "
              << ipv4_hdr.source_address()
              << ": icmp_seq=" << icmp_hdr.sequence_number()
              << ", ttl=" << ipv4_hdr.time_to_live() << ", time=" << rtt;

    notify_thread_.PostTask(
        [&]() { NotifyRTTUpdate(false, sequence_number_, rtt); });
  }

  StartReceive();
}

void PingClient::NotifyRTTUpdate(const bool timeout,
                                 const uint64_t sequence_number, int rtt) {
  for (auto &observer : observer_list_) {
    if (observer) {
      rtt = std::min<int>(kDefaultTimeoutThread, rtt);
      observer->OnRTTUpdate(timeout, sequence_number, rtt);
    }
  }

  bool last_sequence_number = (sequence_number % interval_ == 0);
  if (last_sequence_number) {
    double loss = (double)num_replies_timeout_ / interval_;
    NotifyPacketLossUpdate(first_sequence_number_, loss);
  }
}

void PingClient::NotifyPacketLossUpdate(const uint64_t sequence_number,
                                        const double loss) {
  LOG(INFO) << loss * 100 << "% packet loss";
  num_replies_timeout_ = 0;
  first_sequence_number_ = first_sequence_number_  + interval_;

  for (auto &observer : observer_list_) {
    if (observer) {
      observer->OnPacketLossUpdate(sequence_number, loss);
    }
  }
}

void PingClient::NotifyStop() {
  for (auto &observer : observer_list_) {
    if (observer) {
      observer->OnStop();
    }
  }
}

} // namespace net
