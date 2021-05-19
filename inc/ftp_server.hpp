#pragma once

#include <filesystem>
#include <string>
#include <stdint.h>
#include <vector>
#include <asio.hpp>
#include <list>
#include <deque>
#include <atomic>
namespace fs = std::filesystem;
using asio::ip::tcp;

typedef std::deque<std::string> message_queue;

std::string list_complete(fs::path& path);
int64_t get_filesize_complete(fs::path& path);
void read_file_complete(std::vector<uint8_t>& data, fs::path& path, int64_t goto_offset);

class Session : public std::enable_shared_from_this<Session> {
public:
	Session(tcp::socket socket, asio::io_context& context, const std::string& root_path);
	~Session();

	void start();
	void deliver(const std::string& msg);
	void do_write();

	void deliver_data(tcp::socket data_socket, const std::string& data);
	void deliver_data(tcp::socket data_socket, std::vector<uint8_t> data);
	void do_data_write();
	void do_read();

	bool alive() { return _alive; }

	//commands
	void comm_user(const std::string& input);
	void comm_pwd(const std::string& input);
	void comm_cwd(const std::string& input);
	void comm_pasv(const std::string& input);
	void comm_type(const std::string& input);
	void comm_size(const std::string& input);
	void comm_retr(const std::string& input);
	void comm_quit(const std::string& input);
	void comm_list(const std::string& input);
	void comm_mlsd(const std::string& input);
	void comm_nlst(const std::string& input);
	void comm_rest(const std::string& input);
	void comm_feat(const std::string& input);
	void comm_opts(const std::string& input);
	void comm_syst(const std::string& input);
	void comm_noop(const std::string& input);

private:
	std::deque<std::pair<tcp::socket, std::vector<uint8_t>>> _data_queue;
	tcp::socket _socket;
	std::string _message;
	message_queue _queue;
	asio::streambuf _response;
	tcp::acceptor _data_acceptor;
	asio::io_context& _context;

	std::atomic<bool> _alive = true;

	int64_t rest = 0;

	fs::path disk_root_path = fs::u8path(u8"X:/");
	fs::path virtual_curr_path = fs::u8path(u8"");
};

class Server {
public:
	Server(asio::io_context& context, const tcp::endpoint& endpoint, const std::string& root_path);

	void do_garbage();
	void do_accept();

private:
	std::string _root_path = "";
	std::list<std::shared_ptr<Session>> _sessions;
	asio::io_context& _context;
	tcp::acceptor _acceptor;
};