#pragma once

#include "ftp_server.hpp"
#include "ftp_response.hpp"

#include <codecvt>
#include <fstream>
#include <string>
#include <asio.hpp>
#include <vector>
#include <iostream>
#include <fstream>

#include <bitextractor.hpp>
#include <bitarchiveinfo.hpp>
#include <bitexception.hpp>
using namespace bit7z;

#include <filesystem>
namespace fs = std::filesystem;

const size_t chunk_size_small = 0xFFFFF;
const size_t chunk_threshhold = 0xFFFFFF; //if the file is above this size, it will be sent in chunks

//"better" version which sends the file in chunks
void Session::comm_retr(const std::string& input) {
	
	std::string iinput = input;

	if (input.find_first_of('/') == 0) {
		iinput = input.substr(1, std::string::npos);
	}

	fs::path diskroot = disk_root_path;
	fs::path virtpath = virtual_curr_path;

	deliver(assembleResponse(FTPCode::POS_EARLY_STATUS_OK, "gonna open data conn to send file"));
	
	auto self(shared_from_this());
	_data_acceptor.async_accept(
		[this, self, diskroot, virtpath, iinput](std::error_code ec, tcp::socket socket) {
			fs::path path_to_get = diskroot;
			path_to_get /= virtpath;
			path_to_get /= fs::u8path(iinput);
			//std::cout << "path thing: " << diskroot.u8string() << " " << virtpath.u8string() << " " << path_to_get.u8string() << "\n";
			//read_file_complete(towrite, path_to_get, rest);



			std::vector<uint8_t> data;

			fs::path before_curr = "";
			for (const auto& e : path_to_get) {
				const auto extt = before_curr.extension().u8string();
				if (extt == ".zip" || extt == ".rar" || extt == ".7z") {
					const Bit7zLibrary lib{ L"7z.dll" };


					std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
					std::wstring wide_old_curr_path = converter.from_bytes(before_curr.u8string());

					BitArchiveInfo arc{ lib, wide_old_curr_path, BitFormat::Auto };

					for (const auto& a : arc.items()) {
						std::string path;
						for (const auto& b : fs::path(a.path())) {
							path += "__";
							path += b.u8string();
						}

						if (path == e.u8string()) {
							try {
								BitExtractor extractor{ lib, BitFormat::Auto };
								extractor.extract(wide_old_curr_path, data, a.index());
								//TODO: implement REST here too
								deliver_data(std::move(socket), std::move(data));
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
			const std::wstring wide_curr_path = converter.from_bytes(path_to_get.u8string());

			try {

				std::ifstream ifile(wide_curr_path, std::ios::binary | std::ios::ate);
				if (!ifile.bad()) {
					size_t progress = 0;
					std::streamoff fsize = ifile.tellg();
					ifile.seekg(0, std::ios::beg);

					size_t chunk_size = (fsize > chunk_threshhold) ? chunk_size_small : fsize; //send the entire file in one go if it's small enough ...
					if (rest != 0) {
						chunk_size = chunk_size_small; //...unless we've been given an offset (probably indicating that it's looking for a tiny bit of metadata at the end)

						progress = rest;
						ifile.seekg(rest, std::ios::beg);
						rest = 0;
					}

					while (progress < fsize) {
						size_t readCount = ((fsize - progress) < chunk_size) ? (fsize - progress) : chunk_size;

						data.resize(readCount);
						ifile.read((char*)data.data(), data.size());

						printf("sending part of file: %d bytes into the file and size %d\n", progress, readCount);
						asio::write(socket, asio::buffer(data.data(), data.size()));

						progress += readCount;

						if (!_alive) {
							break;
						}
					}

					ifile.close();
				}
				deliver(assembleResponse(FTPCode::POS_COMPLETE_CLOSING_DATA_CONNECTION, "closing data connection, transmission succeeded"));
				socket.close();
			}
			catch (std::exception& e) {
				std::cout << "taihen in file read: " << e.what() << "\n";
			}
		}
	);
}