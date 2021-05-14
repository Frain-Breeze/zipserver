#pragma once
#include <asio.hpp>
#include <iostream>
#include <deque>
#include <vector>
#include <stdint.h>
#include <filesystem>
#include <codecvt>
#include <fstream>
#include <chrono>
#include <thread>
#include <list>

#include <Windows.h>

#include <bitextractor.hpp>
#include <bitarchiveinfo.hpp>
#include <bitexception.hpp>
using namespace bit7z;

namespace fs = std::filesystem;

std::string list_complete(fs::path& path) {
	fs::path curr = "";
	fs::path before_curr = "";

	const Bit7zLibrary lib{ L"7z.dll" };

	std::string towrite;

	for (const auto& e : path) {
		curr /= e;
		std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
		const std::wstring wide_curr_fname = converter.from_bytes(e.u8string());
		const std::wstring wide_curr_path = converter.from_bytes(curr.u8string());

		const auto extt = e.extension().u8string();

		if (extt == ".7z" || extt == ".rar" || extt == ".zip" ) {
			try {
				BitArchiveInfo arc{ lib, wide_curr_path, BitFormat::Auto };


				for (const auto& a : arc.items()) {

					fs::path fname = a.name();

					const auto ext = fname.extension().u8string();
					//we only go one archive layer deep, so we don't need to patch it here

					if (!a.isDir()) {
						towrite += "Type=file;Size=" + std::to_string(a.size()) + ";Modify=17070101010101.000;Perm=r; ";

						for (const auto& b : fs::path(a.path())) {
							towrite += "__";
							towrite += b.u8string();
						}


						towrite += "\r\n";
					}
					//else if (a.isDir()) {
					//	towrite += "Type=dir;Modify=17070101010101.000;Perm=el; ";
					//}

					



					//towrite += fname.u8string();
				}

				
			}
			catch(const BitException& e){
				std::cout << "taihen in archive open: " << e.what() << "\n";
			}
			std::cout << towrite << "\n";
			return towrite;
			
		}


	}
	//if we get here, it's not an archive we support
	try {
		for (const auto& e : fs::directory_iterator(path)) {
			const auto ext = e.path().extension().u8string();
			if (ext == ".zip" || ext == ".rar" || ext == ".7z") {
				towrite += "Type=dir;Modify=17070101010101.000;Perm=el; ";
			}
			else if (e.is_regular_file()) {
				towrite += "Type=file;Size=" + std::to_string(e.file_size()) + ";Modify=17070101010101.000;Perm=r; ";
			}
			else if (e.is_directory()) {
				towrite += "Type=dir;Modify=17070101010101.000;Perm=el; ";
			}

			towrite += e.path().filename().u8string();
			towrite += "\r\n";
		}
	}
	catch (fs::filesystem_error& e) {
		std::cout << "taihen: " << e.what() << "\n";
	}


	return towrite;
}

int64_t get_filesize_complete(fs::path& path) {
	fs::path curr = "";
	fs::path before_curr = "";

	const Bit7zLibrary lib{ L"7z.dll" };

	for (const auto& e : path) {
		curr /= e;
		std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
		const std::wstring wide_curr_fname = converter.from_bytes(e.u8string());
		const std::wstring wide_old_curr_path = converter.from_bytes(before_curr.u8string());

		const auto extt = before_curr.extension().u8string();

		if (extt == ".zip" || extt == ".rar" || extt == ".7z") {
			BitArchiveInfo arc{ lib, wide_old_curr_path, BitFormat::Auto };

			for (const auto& a : arc.items()) {
				std::string path;
				for (const auto& b : fs::path(a.path())) {
					path += "__";
					path += b.u8string();
				}
				//path += fs::path(a.name()).u8string();

				if (path == e.u8string()) {
					return a.size();
				}
			}
		}
		before_curr /= e;
	}
	//if we get here, it must be a normal file

	return fs::file_size(path);
}
void read_file_complete(std::vector<uint8_t>& data, fs::path& path, int64_t goto_offset) {
	fs::path curr = "";
	fs::path before_curr = "";

	data.clear();

	const Bit7zLibrary lib{ L"7z.dll" };

	for (const auto& e : path) {
		curr /= e;
		std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
		const std::wstring wide_curr_fname = converter.from_bytes(e.u8string());
		const std::wstring wide_old_curr_path = converter.from_bytes(before_curr.u8string());

		const auto extt = before_curr.extension().u8string();

		if (extt == ".zip" || extt == ".rar" || extt == ".7z") {
			BitArchiveInfo arc{ lib, wide_old_curr_path, BitFormat::Auto };

			for (const auto& a : arc.items()) {

				std::string path;
				for (const auto& b : fs::path(a.path())) {
					path += "__";
					path += b.u8string();
				}
				//path += fs::path(a.name()).u8string();

				if (path == e.u8string()) {
					BitExtractor extractor{ lib, BitFormat::Auto };
					try {
						extractor.extract(wide_old_curr_path, data, a.index());
					}
					catch (BitException& e) {
						std::cout << "taihen in file extract: " << e.what() << "\n";
					}
					return;
				}
			}

		}
		before_curr /= e;
	}
	//if we get here, it must be a normal file

	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
	const std::wstring wide_curr_path = converter.from_bytes(path.u8string());

	try {
		std::ifstream ifile(wide_curr_path, std::ios::binary | std::ios::ate);
		if (!ifile.bad()) {
			std::streamoff fsize = ifile.tellg();
			ifile.seekg(0, std::ios::beg);
			if (goto_offset != -1) {
				fsize -= goto_offset;
				ifile.seekg(goto_offset, std::ios::beg);
			}
			data.resize(fsize);
			ifile.read((char*)data.data(), fsize);
			ifile.close();
			return;
		}
	}
	catch (std::exception& e) {
		std::cout << "taihen in file read: " << e.what() << "\n";
	}
}

enum class FTPCode : uint16_t {
	COMMAND_NOT_IMPLEMENTED = 202,
	READY = 220, //used directly after establishing connection
	NEED_PASSWORD = 331, //used after getting username
	GREETING = 230, //user logged in
	UNKNOWN_COMMAND = 501,
	PATHNAME_CREATED = 257, //for pwd return
	OKAY = 200,
	ENTERING_PASV_MODE = 227,
	ACTION_NOT_TAKEN_UNAVAILABLE = 550, // used for denying AUTH requests, cuz apparently "unknown command" is not acceptable?
	CLOSING_DATA_CONNECTION = 226,
	GOING_TO_OPEN_DATA_CONNECTION = 150,
	FILE_STATUS = 213,
	SYSTEM_STATUS = 211,
	SYNTAX_ERROR_IN_COMMAND = 501,
};

using asio::ip::tcp;

typedef std::deque<std::string> message_queue;

class user {
public:
	virtual ~user() {}
	virtual void deliver(const std::string& msg) = 0;
};

typedef std::shared_ptr<user> user_ptr;

void assembleResponse(std::string& ret_msg, FTPCode code, const std::string& message) {
	ret_msg = std::to_string((int)code);
	ret_msg += " ";
	ret_msg += message;
	ret_msg += "\r\n";
}

std::string assembleResponse(FTPCode code, const std::string& message) {
	std::string ret;
	assembleResponse(ret, code, message);
	return ret;
}



class session : public user, public std::enable_shared_from_this<session> {
public:
	session(tcp::socket socket, asio::io_context& context, const std::string& root_path) : _context(context), _socket(std::move(socket)), _data_acceptor(context), disk_root_path(root_path) {}

	~session() {
		std::cout << "oh no session got destructed\n";
	}

	void start() {
		std::cout << "started session with " << _socket.remote_endpoint() << "\n";

		deliver(assembleResponse(FTPCode::READY, "server ready"));

		do_read();
	}

	void deliver(const std::string& msg) {
		bool writing = !_queue.empty();
		_queue.push_back(msg);
		if (!writing) {
			//do_write();
			if (msg[0] == '1') { SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 7); }
			else if (msg[0] == '2') { SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 10); }
			else if (msg[0] == '3') { SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 11); }
			else if (msg[0] == '4') { SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 6); }
			else if (msg[0] == '5') { SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 4); }
			else if (msg[0] == '6') { SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 5); }
			std::cout << "local -> " << _socket.remote_endpoint() << ": " << msg;
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 15);
			do_write();
		}
	}

	void do_write() {
		auto self(shared_from_this());
		asio::async_write(_socket, asio::buffer(_queue.front().data(), _queue.front().size()),
			[this, self](std::error_code ec, size_t len) {
				if (!ec) {
					_queue.pop_front();
					if (!_queue.empty()) {
						do_write();
					}
				}
				else {
					std::cout << "error in command write: " << ec.message() << "\n";
				}
			});
	}

	void deliver_data(tcp::socket data_socket, const std::string& data) {
		std::vector<uint8_t> dat;
		dat.resize(data.size());
		memcpy(dat.data(), data.data(), data.size());
		deliver_data(std::move(data_socket), dat);
	}

	void deliver_data(tcp::socket data_socket, std::vector<uint8_t> data) {
		bool writing = !_data_queue.empty();
		_data_queue.push_back({ std::move(data_socket), std::move(data) });
		if (!writing) {
			do_data_write();
		}
	}

	void do_data_write() {
		auto self(shared_from_this());

		//getting a headache from the async not working, so this'll have to do
		try {
			/*asio::write(_data_queue.front().first, asio::buffer(_data_queue.front().second.data(), _data_queue.front().second.size()));
			_data_queue.pop_front();
			deliver(assembleResponse(FTPCode::CLOSING_DATA_CONNECTION, "closing data connection, transmission succeeded"));
			if (!_data_queue.empty()) {
				do_data_write();
			}*/

			asio::async_write(_data_queue.front().first, asio::buffer(_data_queue.front().second.data(), _data_queue.front().second.size()),
				[this, self](std::error_code ec, size_t len) {
					if (!ec) {
						//_data_queue.front().first.close();
						_data_queue.pop_front();
						//std::cout << "did data write\n";
						deliver(assembleResponse(FTPCode::CLOSING_DATA_CONNECTION, "closing data connection, transmission succeeded"));

						if (!_data_queue.empty()) {
							do_data_write();
						}
					}
					else {
						_alive = false;
						_data_queue.clear();
						std::cout << "error in data write: " << ec.message() << "\n";
					}
				});
		}
		catch (std::system_error& e) {
			std::cout << "taihen: " << e.what() << "\n";
		}


		/*asio::async_write(_data_queue.front().first, asio::buffer(_data_queue.front().second->data(), _data_queue.front().second->size()),
			[this, self](std::error_code ec, size_t len) {
				if (!ec) {
					_data_queue.pop_front();
					//std::cout << "did data write\n";
					deliver(assembleResponse(FTPCode::CLOSING_DATA_CONNECTION, "closing data connection, transmission succeeded"));
					if (!_data_queue.empty()) {
						do_data_write();
					}
				}
				else {
					std::cout << "error: " << ec.message() << "\n";
				}
			});*/
	}


	void do_read() {
		auto self(shared_from_this());
		asio::async_read_until(_socket, _response, "\r\n",
			[this, self](std::error_code ec, size_t len) {
				if (!ec) {
					std::istream respstream(&_response);
					std::string strstr;
					std::getline(respstream, strstr);

					SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 9);
					std::cout << _socket.remote_endpoint() << " -> local: " << strstr << "\n";
					SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 15);

					if (strstr.size() >= 4) {
						if (std::strstr(strstr.c_str(), "FEAT")) {
							deliver("211- extensions supported:\r\n UTF8\r\n MLSD\r\n211 end.\r\n");
						}
						else if (std::strstr(strstr.c_str(), "OPTS")) {
							char bbuf[4096];
							if (sscanf(strstr.c_str(), "OPTS %[^\r]", bbuf) != 1) {
								std::strcpy(bbuf, "");
							}
							if (bbuf == (std::string)"UTF8") {
								//TODO:: actually do shit
							}
							deliver(assembleResponse(FTPCode::OKAY, "good"));
						}
						else if (std::strstr(strstr.c_str(), "USER")) {
							deliver(assembleResponse(FTPCode::GREETING, "user logged in (don't care about password)"));
						}
						else if (std::strstr(strstr.c_str(), "PASS")) {
							deliver(assembleResponse(FTPCode::GREETING, "got password, user logged in"));
						}
						else if (std::strstr(strstr.c_str(), "AUTH")) {
							deliver(assembleResponse(FTPCode::ACTION_NOT_TAKEN_UNAVAILABLE, "we don't do that here"));
						}
						else if (std::strstr(strstr.c_str(), "REST")) {
							if (sscanf(strstr.c_str(), "REST %lld", &rest) != 1) {
								rest = -1;
								deliver(assembleResponse(FTPCode::SYNTAX_ERROR_IN_COMMAND, "could not parse offset"));
							}
							else {
								deliver(assembleResponse(FTPCode::OKAY, "set file pos"));
							}

						}
						else if (std::strstr(strstr.c_str(), "SIZE")) {
							fs::path assembled = fs::u8path(disk_root_path.u8string());
							assembled /= fs::u8path(virtual_curr_path.u8string());
							char bbuf[4096];
							if (sscanf(strstr.c_str(), "SIZE %[^\r]", bbuf) != 1) {
								std::strcpy(bbuf, "");
							}
							assembled /= fs::u8path(bbuf);
							//std::cout << "file thing: " << assembled << "\n";

							if (fs::is_regular_file(assembled)) {
								deliver(assembleResponse(FTPCode::FILE_STATUS, std::to_string(fs::file_size(assembled))));
							}
							else {
								try {
									deliver(assembleResponse(FTPCode::FILE_STATUS, std::to_string(get_filesize_complete(assembled))));
								}
								catch (fs::filesystem_error& e) {
									deliver(assembleResponse(FTPCode::ACTION_NOT_TAKEN_UNAVAILABLE, "can't get filesize of file"));
								}
							}
						}
						else if (std::strstr(strstr.c_str(), "PWD")) {
							if (virtual_curr_path.u8string() == "") {
								deliver(assembleResponse(FTPCode::PATHNAME_CREATED, "\"/\""));
							}
							else {
								deliver(assembleResponse(FTPCode::PATHNAME_CREATED, (std::string)"\"/" + virtual_curr_path.u8string() + "\""));
							}
						}
						else if (std::strstr(strstr.c_str(), "CWD")) {

							bool isRelative = true;

							char bbuf[4096];
							if (sscanf(strstr.c_str(), "CWD %[^\r]", bbuf) == 1) {
								if (bbuf[0] == '/') {
									std::strcpy(bbuf, &bbuf[1]);
									isRelative = false;
								}
							}
							else {
								std::strcpy(bbuf, "");
							}
							//std::cout << "section : " << bbuf << "\n";

							fs::path assembled_path = fs::u8path(disk_root_path.u8string());
							if (isRelative) { assembled_path /= virtual_curr_path; }
							assembled_path /= fs::u8path(bbuf);

							//is_valid_fake_directory(assembled_path);

							if (fs::is_directory(assembled_path) || true) {
								//std::cout << "folder " << fs::u8path(bbuf).u8string() << " is real, we will enter it";
								if (isRelative) {
									virtual_curr_path /= fs::u8path(bbuf);
								}
								else {
									virtual_curr_path = fs::u8path(bbuf);
								}
							}
							deliver(assembleResponse(FTPCode::OKAY, virtual_curr_path.u8string()));
						}
						else if (std::strstr(strstr.c_str(), "TYPE")) {
							deliver(assembleResponse(FTPCode::OKAY, "todo: actually switch type"));
						}
						else if (std::strstr(strstr.c_str(), "CDUP")) {
							virtual_curr_path = virtual_curr_path.parent_path();
							deliver(assembleResponse(FTPCode::OKAY, "todo: check if there is a parent path"));
						}
						else if (std::strstr(strstr.c_str(), "QUIT")) {
							_socket.close();
							_data_queue.clear();
							_alive = false;
							return;
							//deliver(assembleResponse(FTPCode::OKAY, "todo: check if there is a parent path"));
						}
						else if (std::strstr(strstr.c_str(), "ABOR")) {
							deliver(assembleResponse(FTPCode::OKAY, "we don't actually abort shit since we can't"));
							for (int i = 0; i < _data_queue.size(); i++) {
								_data_queue[i].second.resize(0);
							}
						}
						else if (std::strstr(strstr.c_str(), "SYST")) {
							deliver(assembleResponse(FTPCode::OKAY, "UNIX Type: L8"));
						}
						else if (std::strstr(strstr.c_str(), "MLSD") || std::strstr(strstr.c_str(), "LIST")) {

							fs::path diskroot = disk_root_path;
							fs::path virtpath = virtual_curr_path;

							deliver(assembleResponse(FTPCode::GOING_TO_OPEN_DATA_CONNECTION, "gonna open data conn"));

							auto self(shared_from_this());
							_data_acceptor.async_accept(
								[this, self, diskroot, virtpath](std::error_code ec, tcp::socket socket) {
									fs::path path_to_list = diskroot;
									path_to_list /= virtpath;
									//std::cout << "path thing root: " << diskroot.u8string() << " virtual: " << virtpath.u8string() << " combined: "<< path_to_list.u8string() << "\n";
									std::string towrite;

									towrite = list_complete(path_to_list);

									/*for (const auto& e : fs::directory_iterator(path_to_list)) {


									}
									//std::cout << "did write of directory listing: " << fs::u8path(towrite).u8string() << "\n";*/
									deliver_data(std::move(socket), towrite);
								}
							);
						}
						else if (std::strstr(strstr.c_str(), "RETR")) {

							char bbuf[4096];
							if (sscanf(strstr.c_str(), "RETR %[^\r]", bbuf) != 1) {
								std::strcpy(bbuf, "");
							}
							if (bbuf[0] == '/') {
								strcpy(bbuf, &bbuf[1]);
							}

							//std::cout << "file to get: " << bbuf << "\n";

							fs::path diskroot = disk_root_path;
							fs::path virtpath = virtual_curr_path;

							deliver(assembleResponse(FTPCode::GOING_TO_OPEN_DATA_CONNECTION, "gonna open data conn to send file"));

							auto self(shared_from_this());
							_data_acceptor.async_accept(
								[this, self, diskroot, virtpath, bbuf](std::error_code ec, tcp::socket socket) {
									fs::path path_to_get = diskroot;
									path_to_get /= virtpath;
									path_to_get /= fs::u8path(bbuf);
									//std::cout << "path thing: " << diskroot.u8string() << " " << virtpath.u8string() << " " << path_to_get.u8string() << "\n";

									std::u16string wide = std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t>{}.from_bytes(path_to_get.u8string());

									std::vector<uint8_t> towrite;
									read_file_complete(towrite, path_to_get, rest);
									rest = -1;

									deliver_data(std::move(socket), std::move(towrite));

									/*std::ifstream ifile(wide, std::ios::binary | std::ios::ate);
									if (!ifile.bad()) {
										std::streamoff fsize = ifile.tellg();
										ifile.seekg(0, std::ios::beg);
										towrite.resize(fsize);
										ifile.read((char*)towrite.data(), fsize);
										ifile.close();

										deliver_data(socket, towrite);
									}
									else {
										std::cout << "file read failed!\n";
									}*/

								}
							);
						}
						else if (std::strstr(strstr.c_str(), "PASV")) {

							if (_data_acceptor.is_open()) {
								_data_acceptor.close();
								//std::cout << "closed data acceptor\n";
							}

							tcp::endpoint endp(tcp::v4(), 0);
							_data_acceptor.open(endp.protocol());
							_data_acceptor.bind(endp);
							_data_acceptor.listen(asio::socket_base::max_listen_connections);

							auto ip_bytes = _socket.local_endpoint().address().to_v4().to_bytes();
							auto port = _data_acceptor.local_endpoint().port();

							std::stringstream stream;
							stream << "Entering passive mode (";
							for (size_t i = 0; i < 4; i++)
							{
								stream << static_cast<int>(ip_bytes[i]) << ",";
							}
							stream << ((port >> 8) & 0xff) << "," << (port & 0xff) << ")";



							deliver(assembleResponse(FTPCode::ENTERING_PASV_MODE, stream.str()));

						}
						else {
							deliver(assembleResponse(FTPCode::UNKNOWN_COMMAND, "bad command >:"));
						}
					}
					else {
						deliver(assembleResponse(FTPCode::COMMAND_NOT_IMPLEMENTED, "wher command? >:"));
					}


				}

				do_read();
			});
	}

	bool alive() {
		return _alive;
	}

private:
	std::deque<std::pair<tcp::socket, std::vector<uint8_t>>> _data_queue;
	tcp::socket _socket;
	std::string _message;
	message_queue _queue;
	asio::streambuf _response;
	tcp::acceptor _data_acceptor;
	asio::io_context& _context;

	bool _alive = true;

	int64_t rest = -1;

	fs::path disk_root_path = fs::u8path(u8"X:/");
	fs::path virtual_curr_path = fs::u8path(u8"");
};

class server {
public:
	server(asio::io_context& context, const tcp::endpoint& endpoint, const std::string& root_path) : _context(context), _acceptor(context, endpoint), _root_path(root_path) {
		do_accept();
	}

	void do_garbage() {
		std::list<std::shared_ptr<session>>::iterator i = _sessions.begin();
		while (i != _sessions.end())
		{
			if (!(*i)->alive())
			{
				i = _sessions.erase(i);
			}
			else
			{
				i++;
			}
		}
	}

	void do_accept() {
		_acceptor.async_accept(
			[this](std::error_code ec, tcp::socket socket) {
				if (!ec) {
					_sessions.emplace_front(std::make_shared<session>(std::move(socket), _context, _root_path))->start();
				}

				do_garbage();

				do_accept();
			}
		);
	}

private:
	std::string _root_path = "";
	std::list<std::shared_ptr<session>> _sessions;
	asio::io_context& _context;
	tcp::acceptor _acceptor;
};