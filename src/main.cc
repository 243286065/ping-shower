#include "base/log/logging.h"
#include "base/thread/message_loop.h"
#include "display/matplot_displayer.h"
#include "net/ping/ping_client.h"

#if defined(OS_LINUX)
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

#include <memory>

static bool stop_flag = false;
static std::unique_ptr<display::MathPlotDisplayer> player = nullptr;
static std::unique_ptr<net::PingClient> client = nullptr;

static void StopHandler(int signo) {
  if (stop_flag)
    return;

  stop_flag = true;
  if(client) {
    client->Stop();
    client.reset();
  }
};

int main(int argc, char *argv[]) {
  logging::SetMinLogLevel(logging::LogSeverity::LOG_INFO);
#if defined(OS_LINUX)
  signal(SIGINT, StopHandler);
  signal(SIGTERM, StopHandler);
#endif

  if (argc != 2) {
    LOG(ERROR) << "Usage: ping_shower ip";
    return -1;
  }

  player.reset(new display::MathPlotDisplayer());
  client.reset(new net::PingClient(argv[1]));

  client->AddObserver(player.get());
  client->Start();

  while(!stop_flag) {
    player->Refresh();
    std::this_thread::sleep_for(std::chrono::seconds(1));
  }

  return 0;
}