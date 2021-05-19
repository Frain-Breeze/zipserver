#define ASIO_STANDALONE
#include <asio.hpp>
#include <iostream>
#include "ftp_server.hpp"


int main(int argc, char* argv[]) {
	SetConsoleOutputCP(65001);
	printf("usage: .exe [port] \"[path to root]\"\n");

	if (argc == 3) {
		int port = -1;
		if (sscanf(argv[1], "%d", &port) != 1) {
			printf("couldn't parse port parameter!\n");
			return -1;
		}

		asio::io_context io_context;
		tcp::endpoint endp(tcp::v4(), port);
		Server ser(io_context, endp, argv[2]);
		printf("server started on port %d with root folder %s\n", port, argv[2]);
		io_context.run();
	}
	else {
		printf("not enough arguments, or too many! we need 2, we have %d\n", argc - 1);
		std::cin.ignore();
	}
}