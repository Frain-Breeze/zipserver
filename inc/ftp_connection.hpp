#pragma once

#include <asio.hpp>
#include <string>

using asio::ip::tcp;

class Ftpconn {
public:
	Ftpconn() : socket(ioserv){}
	~Ftpconn() { this->close(); }


	void connect(const std::string& hostname, const int port) {
		try {
			//resolve host name
			tcp::resolver resolv(this->ioserv);
			tcp::resolver::query query(hostname, std::to_string(port));

			//connect to host
			asio::connect(this->socket, resolv.resolve(query));
		}
		catch (std::system_error& e) {
			printf("err: %s\n", e.what());
		}
	}


	std::string readLine() {
		std::string ret;
		asio::streambuf responsebuf;
		try {
			asio::read_until(this->socket, responsebuf, "\n");
			std::istream responseStream(&responsebuf);
			std::getline(responseStream, ret);
		}
		catch (std::system_error& e) {
			printf("err: %s\n", e.what());
		}

		return ret;
	}
	void close() { this->socket.close(); }



private:
	asio::io_context ioserv;
	tcp::socket socket;
};