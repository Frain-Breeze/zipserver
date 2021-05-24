#define ASIO_STANDALONE
#include <asio.hpp>
#include <iostream>
#include <fstream>
#include "ftp_server.hpp"


int main(int argc, char* argv[]) {
	SetConsoleOutputCP(65001);
	printf("usage: .exe\n");

	if (argc == 1) {

		int port = 2121;
		std::unordered_map<std::string, fs::path> root_points;

		std::ifstream settings("ftp_settings.txt");
		char buffer[2048];
		while (settings.getline(buffer, 2048)) {
			std::string bufstr = buffer;
			size_t found = bufstr.find_first_of(' ');
			if (found != std::string::npos) {
				if (bufstr.substr(0, found) == "PORT") {
					sscanf(bufstr.substr(found, std::string::npos).c_str(), "%d", &port);
				}
				else if (bufstr.substr(0, found) == "FOLDER") {
					std::string first, second;
					std::string todo = bufstr.substr(found, std::string::npos);
					size_t offset = 0;
					bool instring = false;
					bool insecond = false;
					while (offset != todo.length()) {
						if (todo[offset] == '\"') {
							if (instring) { instring = false; insecond = true; }
							else { instring = true; }
						}
						else if (instring && insecond) { second += todo[offset]; }
						else if (instring && !insecond) { first += todo[offset]; }
						offset++;
					}
					root_points.insert_or_assign(first, fs::u8path(second));
				}
			}
		}

		asio::io_context io_context;
		tcp::endpoint endp(tcp::v4(), port);
		Server ser(io_context, endp, root_points);
		printf("server started on port %d\n", port);
		io_context.run();
	}
	else {
		printf("not enough arguments, or too many! we need 0, we have %d\n", argc - 1);
		std::cin.ignore();
	}
}