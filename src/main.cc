#include "base/log/logging.h"
#include "net/ping/ping_client.h"

#include "base/thread/message_loop.h"

int main(int argc, char* argv[]) {
	logging::SetMinLogLevel(logging::LogSeverity::LOG_INFO);

	if(argc != 2 ) {
		LOG(ERROR) << "Usage: ping_shower ip";
		return -1;
	}

	base::MessageLoop main_loop;

	net::PingClient client(argv[1]);
	client.Start();

	main_loop.RunLoop();

	return 0;
}