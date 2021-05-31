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

		if (fs::exists("ftp_settings.txt") && fs::is_regular_file("ftp_settings.txt")) {
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
		}
		else {
			FILE* sett = fopen("ftp_settings.txt", "wb");
			if (sett) {
				std::string def = "PORT 2121\nFOLDER \"server_directory\" \"" + fs::current_path().u8string() + "\"\n";
				fwrite(def.c_str(), 1, strlen(def.c_str()), sett);
				fclose(sett);
				printf("unable to find an existing settings file, creating one with default settings.\n");
			}
			else {
				printf("unable to find an existing settings file, and can't open a new one. starting server from current folder instead\n");
			}

			root_points.insert_or_assign("server_directory", fs::current_path().u8string());
		}

		asio::io_context io_context;
		tcp::endpoint endp(tcp::v4(), port);
		Server ser(io_context, endp, root_points);
		printf("server started on port %d\n", port);
		io_context.run();
	}
	else {
		printf("too many arguments! we don't want any\n");
		std::cin.ignore();
	}
}