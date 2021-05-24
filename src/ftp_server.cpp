#include "ftp_server.hpp"
#include "ftp_response.hpp"
#include "ftp_command_retr.hpp"

#include "util_file_time.hpp"
#include "ftp_path_resolver.hpp"

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

#include <map>
#include <unordered_map>

#include <Windows.h>

#include <bitextractor.hpp>
#include <bitarchiveinfo.hpp>
#include <bitexception.hpp>
using namespace bit7z;

namespace fs = std::filesystem;

std::string list_complete(const fs::path& path, bool metadata, bool unix) {
	fs::path curr = "";
	fs::path before_curr = "";

	const Bit7zLibrary lib{ L"7z.dll" };

	std::string towrite;

	for (const auto& e : path) {
		curr /= e;
		std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
		std::wstring wide_curr_fname;
		std::wstring wide_curr_path;

		try {
			wide_curr_fname = converter.from_bytes(e.u8string());
			wide_curr_path = converter.from_bytes(curr.u8string());
		}
		catch(std::system_error& e){
			std::cout << "error in list_complete: " << e.what() << "\n";
		}

		const auto extt = e.extension().u8string();

		if (extt == ".7z" || extt == ".rar" || extt == ".zip") {
			try {
				BitArchiveInfo arc{ lib, wide_curr_path, BitFormat::Auto };


				for (const auto& a : arc.items()) {

					fs::path fname = a.name();

					const auto ext = fname.extension().u8string();
					//we only go one archive layer deep, so we don't need to patch it here

					if (!a.isDir()) {
						if (metadata) {
							const auto modify_time = a.getProperty(BitProperty::MTime);
							auto ft = modify_time.filetime;
							ULARGE_INTEGER ull; ull.LowPart = ft.dwLowDateTime;
							ull.HighPart = ft.dwHighDateTime;
							time_t tim = ull.QuadPart / 10000000ULL - 11644473600ULL;
							if (unix) {
								towrite += "-r-------- 1 loli loli " + std::to_string(a.size()) + " " + util_file::write_time_unix(localtime(&tim)) + " ";
							}
							else {
								const std::string modify = util_file::write_time_mlsd(localtime(&tim));
								towrite += "Type=file;Size=" + std::to_string(a.size()) + ";";
								towrite += modify;
								towrite += ";Perm=r; ";
							}
						}

						for (const auto& b : fs::path(a.path())) {
							towrite += "__";
							towrite += b.u8string();
						}


						towrite += "\r\n";
					}
				}
			}
			catch (const BitException& e) {
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
			if (metadata) {
				if (unix) {
					const std::string modify = util_file::write_time_unix(util_file::last_modify(e.path()));

					if (ext == ".zip" || ext == ".rar" || ext == ".7z" || e.is_directory()) {
						towrite += "dr-------- 1 loli loli 0 " + modify + " ";
					}
					else if (e.is_regular_file()) {
						towrite += "-r-------- 1 loli loli " + std::to_string(e.file_size()) + " " + modify + " ";
					}
				}
				else {
					const std::string modify = util_file::write_time_mlsd(util_file::last_modify(e.path()));

					if (ext == ".zip" || ext == ".rar" || ext == ".7z" || e.is_directory()) {
						towrite += "Type=dir;";
						towrite += modify;
						towrite +=";Perm=el; ";
					}
					else if (e.is_regular_file()) {
						towrite += "Type=file;Size=" + std::to_string(e.file_size()) + ";" + modify + ";Perm=r; ";
					}
				}
			}
			towrite += e.path().filename().u8string();
			towrite += "\r\n";
		}
	}
	catch (fs::filesystem_error& e) {
		std::cout << "taihen: " << e.what() << "\n";
	}

	std::cout << towrite << "\n";
	return towrite;
}

int64_t get_filesize_complete(fs::path& path) {
	fs::path curr = "";
	fs::path before_curr = "";

	const Bit7zLibrary lib{ L"7z.dll" };

	for (const auto& e : path) {
		curr /= e;
		std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
		std::wstring wide_curr_fname;
		std::wstring wide_old_curr_path;

		try {
			wide_curr_fname = converter.from_bytes(e.u8string());
			wide_old_curr_path = converter.from_bytes(before_curr.u8string());
		}
		catch (std::system_error& e) {
			std::cout << "error in get_filesize_complete: " << e.what() << "\n";
		}

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
		std::wstring wide_curr_fname;
		std::wstring wide_old_curr_path;

		try {
			wide_curr_fname = converter.from_bytes(e.u8string());
			wide_old_curr_path = converter.from_bytes(before_curr.u8string());
		}
		catch (std::system_error& e) {
			std::cout << "error in read_file_complete: " << e.what() << "\n";
		}

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



Session::Session(tcp::socket socket, asio::io_context& context, std::unordered_map<std::string, fs::path>& points) : _context(context), _socket(std::move(socket)), _data_acceptor(context), root_points(points) {}

Session::~Session() {
	std::cout << "oh no session got destructed\n";
}

void Session::start() {
	std::cout << "started session with " << _socket.remote_endpoint() << "\n";

	deliver(assembleResponse(FTPCode::POS_COMPLETE_READY_NEW_USER, "server ready"));

	do_read();
}

void Session::deliver(const std::string& msg) {
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

void Session::do_write() {
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

void Session::deliver_data(tcp::socket data_socket, const std::string& data) {
	std::vector<uint8_t> dat;
	dat.resize(data.size());
	memcpy(dat.data(), data.data(), data.size());
	deliver_data(std::move(data_socket), dat);
}

void Session::deliver_data(tcp::socket data_socket, std::vector<uint8_t> data) {
	bool writing = !_data_queue.empty();
	_data_queue.push_back({ std::move(data_socket), std::move(data) });
	if (!writing) {
		do_data_write();
	}
}

void Session::do_data_write() {
	auto self(shared_from_this());

	try {

		asio::async_write(_data_queue.front().first, asio::buffer(_data_queue.front().second.data(), _data_queue.front().second.size()),
			[this, self](std::error_code ec, size_t len) {
				if (!ec) {
					//_data_queue.front().first.close();
					_data_queue.pop_front();
					//std::cout << "did data write\n";
					deliver(assembleResponse(FTPCode::POS_COMPLETE_CLOSING_DATA_CONNECTION, "closing data connection, transmission succeeded"));

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
}

void Session::comm_rest(const std::string& input) {
	if (sscanf(input.c_str(), "%lld", &rest) == 1) {
		deliver(assembleResponse(FTPCode::POS_MID_PENDING_FURTHER_INFO, (std::string)"set offset to " + std::to_string(rest)));
	}
	else {
		deliver(assembleResponse(FTPCode::NEG_PERM_SYNTAX_ERROR, "couldn't parse REST command!"));
	}
}
void Session::comm_user(const std::string& input) {
	deliver(assembleResponse(FTPCode::POS_COMPLETE_LOGGED_IN, (std::string)"user logged in with username: " + input));
}
void Session::comm_pwd(const std::string& input) {
	deliver(assembleResponse(FTPCode::POS_COMPLETE_NAME_CREATED, (std::string)"\"/" + virtual_curr_path.u8string() + "\""));
}
void Session::comm_type(const std::string& input) {
	deliver(assembleResponse(FTPCode::POS_COMPLETE_SUCCESS, "we didn't actually change mode but it's always in binary anyways"));
}
void Session::comm_syst(const std::string& input) {
	deliver(assembleResponse(FTPCode::POS_COMPLETE_NAME_SYSTEM_TYPE, "UNIX Type: L8"));
}
void Session::comm_quit(const std::string& input) {
	_alive = false;
	_data_queue.clear();
	deliver(assembleResponse(FTPCode::POS_COMPLETE_CLOSING_CONTROL_CONNECTION, "set session to inactive"));
	_socket.close();
}
void Session::comm_abor(const std::string& input) {
	_data_queue.clear();
	deliver(assembleResponse(FTPCode::POS_COMPLETE_CLOSING_DATA_CONNECTION, "set session to inactive"));
}
void Session::comm_feat(const std::string& input) {
	deliver("211- extensions supported:\r\n UTF8\r\n MLSD\r\n211 end.\r\n");
}
void Session::comm_opts(const std::string& input) {
	deliver(assembleResponse(FTPCode::POS_COMPLETE_SUCCESS, "didn't actually check the option but it should be okay"));
}
void Session::comm_noop(const std::string& input) {
	deliver(assembleResponse(FTPCode::POS_COMPLETE_SUCCESS, "succesfully did absolutely nothing"));
}
void Session::comm_size(const std::string& input) {
	//fs::path assembled = fs::u8path(disk_root_path.u8string());
	//assembled /= fs::u8path(virtual_curr_path.u8string());
	//assembled /= fs::u8path(input);
	//std::cout << "file thing: " << assembled << "\n";
	fs::path assembled = assemble_path(curr_root_point, fs::u8path(virtual_curr_path.u8string()));
	assembled /= fs::u8path(input);

	if (fs::is_regular_file(assembled)) {
		deliver(assembleResponse(FTPCode::POS_COMPLETE_FILE_STATUS, std::to_string(fs::file_size(assembled))));
	}
	else {
		try {
			deliver(assembleResponse(FTPCode::POS_COMPLETE_FILE_STATUS, std::to_string(get_filesize_complete(assembled))));
		}
		catch (fs::filesystem_error& e) {
			deliver(assembleResponse(FTPCode::NEG_PERM_ACTION_NOT_TAKEN_UNAVAILABLE, "can't get filesize of file"));
		}
	}
}
void Session::comm_nlst(const std::string& input) {
	
	printf("badddd\n\n\n\n\n\n\n\n\n\n\n\n\n");
	/*fs::path diskroot = disk_root_path;
	fs::path virtpath = (input == "") ? virtual_curr_path : fs::u8path(input);

	deliver(assembleResponse(FTPCode::POS_EARLY_STATUS_OK, "gonna open data conn to list directory contents, without metadata"));

	auto self(shared_from_this());
	_data_acceptor.async_accept(
		[this, self, diskroot, virtpath](std::error_code ec, tcp::socket socket) {
			fs::path path_to_list = diskroot;
			path_to_list /= virtpath;
			//std::cout << "path thing root: " << diskroot.u8string() << " virtual: " << virtpath.u8string() << " combined: "<< path_to_list.u8string() << "\n";
			std::string towrite = list_complete(path_to_list, false, false);
			deliver_data(std::move(socket), towrite);
		}
	);*/
}
void Session::comm_mlsd(const std::string& input) {
	const std::string root_point = curr_root_point;
	const fs::path to_list = assemble_path(curr_root_point, virtual_curr_path);

	deliver(assembleResponse(FTPCode::POS_EARLY_STATUS_OK, "gonna open data conn to list directory contents, with metadata"));

	auto self(shared_from_this());
	_data_acceptor.async_accept(
		[this, self, to_list, root_point](std::error_code ec, tcp::socket socket) {
			std::string towrite;
			if (root_point == "") {
				for (const auto& i : root_points) {
					towrite += "Type=dir;Perm=el; ";
					towrite += i.first;
					towrite += "\r\n";
				}
			}
			else {
				towrite = list_complete(to_list, true, false);
			}
			deliver_data(std::move(socket), towrite);
		}
	);
}

void Session::comm_list(const std::string& input) {
	const std::string root_point = curr_root_point;
	const fs::path to_list = assemble_path(curr_root_point, virtual_curr_path);

	deliver(assembleResponse(FTPCode::POS_EARLY_STATUS_OK, "gonna open data conn to list directory contents, with metadata"));

	auto self(shared_from_this());
	_data_acceptor.async_accept(
		[this, self, to_list, root_point](std::error_code ec, tcp::socket socket) {
			std::string towrite;
			if (root_point == "") {
				for (const auto& i : root_points) {
					towrite += "dr-------- 1 loli loli 0 Jan 1 17:07 ";
					towrite += i.first;
					towrite += "\r\n";
				}
			}
			else {
				towrite = list_complete(to_list, true, false);
			}
			deliver_data(std::move(socket), towrite);
		}
	);
}
void Session::comm_cwd(const std::string& input) {
	if (input.size() > 0) {
		if (input[0] == '/') {

			std::string first_part = "";
			for (const auto& i : fs::u8path(input)) {
				if (i != "/") {
					first_part = i.u8string();
					break;
				}
			}

			const auto found_root = root_points.find(first_part);
			if (found_root != root_points.end()) {
				curr_root_point = found_root->first;

				virtual_curr_path = strip_root_point(fs::u8path(input));
				deliver(assembleResponse(FTPCode::POS_COMPLETE_FILE_ACTION_OKAY, (std::string)"\"" + virtual_curr_path.u8string() + "\""));
				return;
			}
			else if (input.size() == 1) {
				curr_root_point = "";
				virtual_curr_path = "/";
				deliver(assembleResponse(FTPCode::POS_COMPLETE_FILE_ACTION_OKAY, (std::string)"\"" + virtual_curr_path.u8string() + "\""));
				return;
			}
			deliver(assembleResponse(FTPCode::NEG_PERM_SYNTAX_ERROR, "something went wrong"));
			return;
		}
		else {
			virtual_curr_path /= fs::u8path(input);
			deliver(assembleResponse(FTPCode::POS_COMPLETE_FILE_ACTION_OKAY, (std::string)"\"" + virtual_curr_path.u8string() + "\""));
			return;
		}
	}
	
	deliver(assembleResponse(FTPCode::NEG_PERM_SYNTAX_ERROR, "something went wrong"));

	/*if (input.size() > 0) {
		if (input[0] == '/') { //dash means non-relative
			virtual_curr_path = fs::u8path(input);
		}
		else {
			virtual_curr_path /= fs::u8path(input);
		}
		deliver(assembleResponse(FTPCode::POS_COMPLETE_FILE_ACTION_OKAY, (std::string)"\"" + virtual_curr_path.u8string() + "\""));
		return;
	}
	deliver(assembleResponse(FTPCode::NEG_PERM_SYNTAX_ERROR, "you need to provide an argument to change the directory to"));*/
}
void Session::comm_pasv(const std::string& input) {
	if (_data_acceptor.is_open()) {
		_data_acceptor.close();
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

	deliver(assembleResponse(FTPCode::POS_COMPLETE_ENTER_PASV, stream.str()));
}

void Session::do_read() {
	auto self(shared_from_this());

	asio::async_read_until(_socket, _response, "\r\n",
		[this, self](std::error_code ec, size_t len) {
			if (!ec) {

				const std::map<std::string, std::function<void(const std::string&)>> commands {
					{"user", std::bind(&Session::comm_user, this, std::placeholders::_1)},
					{"pwd ", std::bind(&Session::comm_pwd, this, std::placeholders::_1)},
					{"cwd ", std::bind(&Session::comm_cwd, this, std::placeholders::_1)},
					{"pasv", std::bind(&Session::comm_pasv, this, std::placeholders::_1)},
					{"type", std::bind(&Session::comm_type, this, std::placeholders::_1)},
					{"mlsd", std::bind(&Session::comm_mlsd, this, std::placeholders::_1)},
					{"list", std::bind(&Session::comm_list, this, std::placeholders::_1)},
					{"size", std::bind(&Session::comm_size, this, std::placeholders::_1)},
					{"retr", std::bind(&Session::comm_retr, this, std::placeholders::_1)},
					{"quit", std::bind(&Session::comm_quit, this, std::placeholders::_1)},
					{"abor", std::bind(&Session::comm_abor, this, std::placeholders::_1)},
					{"rest", std::bind(&Session::comm_rest, this, std::placeholders::_1)},
					{"feat", std::bind(&Session::comm_feat, this, std::placeholders::_1)},
					{"opts", std::bind(&Session::comm_opts, this, std::placeholders::_1)},
					{"syst", std::bind(&Session::comm_syst, this, std::placeholders::_1)},
					{"noop", std::bind(&Session::comm_noop, this, std::placeholders::_1)},
					//{"nlst", std::bind(&Session::comm_nlst, this, std::placeholders::_1)},
				};

				std::istream respstream(&_response);
				std::string msg;
				std::getline(respstream, msg, '\r');
				_response.consume(_response.size());

				SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 9);
				std::cout << _socket.remote_endpoint() << " -> local: " << msg << "\n";
				SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 15);

				size_t first_space = msg.find_first_of(' ');
				std::string comm = msg.substr(0, first_space);
				char char_comm[5];
				char_comm[3] = ' ';
				char_comm[4] = 0;
				if (comm.size() > 2) {
					char_comm[0] = tolower(comm[0]);
					char_comm[1] = tolower(comm[1]);
					char_comm[2] = tolower(comm[2]);
					if (comm.size() > 3) {
						char_comm[3] = tolower(comm[3]);
					}
				}

				const auto found_func = commands.find(char_comm);
				if (found_func != commands.end()) {
					const std::string para = (first_space == std::string::npos) ? "" : msg.substr(first_space + 1, std::string::npos);
					found_func->second(para);
				}
				else {
					deliver(assembleResponse(FTPCode::NEG_PERM_SYNTAX_ERROR, "command not in list of implemented commands"));
				}
			}
			else {

			}

			do_read();
		}
	);
}

Server::Server(asio::io_context& context, const tcp::endpoint& endpoint, std::unordered_map<std::string, fs::path>& points) : _context(context), _acceptor(context, endpoint), _root_points(points) {
	do_accept();
}

void Server::do_garbage() {
	std::list<std::shared_ptr<Session>>::iterator i = _sessions.begin();
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

void Server::do_accept() {
	_acceptor.async_accept(
		[this](std::error_code ec, tcp::socket socket) {
			if (!ec) {
				_sessions.emplace_front(std::make_shared<Session>(std::move(socket), _context, _root_points))->start();
			}

			do_garbage();

			do_accept();
		}
	);
}

