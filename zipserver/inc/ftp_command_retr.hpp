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

//#include <bitextractor.hpp>
//#include <bitarchiveinfo.hpp>
//#include <bitexception.hpp>
//using namespace bit7z;

#define LIBARCHIVE_STATIC
#include <archive.h>
#include <archive_entry.h>

#include <filesystem>
namespace fs = std::filesystem;

const size_t chunk_size_small = 0xFFFFF;
const size_t chunk_threshhold = 0xFFFFFF; //if the file is above this size, it will be sent in chunks

//"better" version which sends the file in chunks
void Session::comm_retr(const std::string& input) {

	std::string root_point = curr_root_point;
	fs::path curr_path = virtual_curr_path;
	curr_path /= fs::u8path(input);

	if (input.find_first_of('/') == 0) {
		//iinput = input.substr(1, std::string::npos);

		//TODO: tf is this here doing?
		std::string first_part = "";
		for (const auto& i : fs::u8path(input)) {
			if (i != "/") {
				first_part = i.u8string();
				break;
			}
		}

		const auto found_root = root_points.find(first_part);
		if (found_root != root_points.end()) {
			root_point = found_root->first;
			curr_path = strip_root_point(fs::u8path(input));
		}
		else {
			printf("bad\n\n\n\n\n\n\n\n\n\n\n\n");
		}
	}
	else {
		root_point = input.substr(0, input.find_first_of('/'));

		//HACK: should root point even be processed here? doesn't make a whole lot of sense to me...
		const auto found_root = root_points.find(root_point);
		if (found_root != root_points.end()) {
			root_point = found_root->first;
			curr_path = strip_root_point(fs::u8path(input));
		}
		else {
			root_point = curr_root_point;
			curr_path = virtual_curr_path;
			curr_path /= input;
		}
	}


	printf("curr path: %s\n", curr_path.u8string().c_str());
	fs::path to_retrieve = assemble_path(root_point, curr_path);
	if (fs::is_directory(to_retrieve)) {
		deliver(assembleResponse(FTPCode::NEG_PERM_FILE_UNAVAILABLE, "this is a folder. not a file. what are you trying to achieve here?"));
		return;
	}
	deliver(assembleResponse(FTPCode::POS_EARLY_STATUS_OK, "gonna open data conn to send file"));

	auto self(shared_from_this());
	_data_acceptor.async_accept(
		[this, self, to_retrieve](std::error_code ec, tcp::socket socket) {
			fs::path path_to_get = to_retrieve;

			std::vector<uint8_t> data;

			printf("path to get: %s\n", path_to_get.u8string().c_str());

			fs::path before_curr = "";
			for (const auto& e : path_to_get) {
				const auto extt = before_curr.extension().u8string();
				if (extt == ".zip" || extt == ".rar" || extt == ".7z") {
					archive* ar = archive_read_new();
					archive_read_support_format_all(ar);
					int r = archive_read_open_filename(ar, before_curr.u8string().c_str(), 16384);
					if(r != ARCHIVE_OK) { std::cout << "taihen in file extract: " << archive_error_string(ar) << "\n"; }
					
					archive_entry* ent;
					while(archive_read_next_header(ar, &ent) == ARCHIVE_OK) {
						printf("%s -- ", archive_entry_pathname(ent));

						//fs::path curr_path = before_curr;
						std::string additional_path = "";
						const char* ent_name = archive_entry_pathname_utf8(ent);
						for (int i = 0; ent_name[i]; i++) {
							if (ent_name[i] == '/')
								additional_path += "__";
							else
								additional_path += ent_name[i];
						}
						//curr_path /= fs::u8path(additional_path);
						//curr_path /= e;
						//printf("%s\n", curr_path.u8string().c_str());

						if (e != additional_path)
							continue;

						if (archive_entry_filetype(ent) != AE_IFREG)
							continue;

						int64_t outsize = archive_entry_size(ent);
						data.resize(outsize);
						int64_t data_read = archive_read_data(ar, data.data(), outsize);
						printf("read %lld, should be %lld bytes\n", data_read, outsize);
						if ((int)data_read < 0)
							printf("error in data read: %s\n", archive_error_string(ar));
						archive_read_close(ar);


						deliver_data(std::move(socket), std::move(data));
						return;
					}
					/*const Bit7zLibrary lib{ L"7z.dll" };


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
					}*/
				}
				before_curr /= e;
			}

			//if we get here, it must be a normal file
			//std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
			//const std::wstring wide_curr_path = converter.from_bytes(path_to_get.u8string());

			try {

				std::ifstream ifile(path_to_get.u16string(), std::ios::binary | std::ios::ate); //TODO: why is u16string required 
				if (!ifile.bad()) {
					int64_t progress = 0;
					int64_t fsize = ifile.tellg();
					ifile.seekg(0, std::ios::beg);

					if (fsize < 0)
						throw std::exception("file size is negative");

					int64_t chunk_size = (fsize > chunk_threshhold) ? chunk_size_small : fsize; //send the entire file in one go if it's small enough ...
					if (rest != 0) {
						chunk_size = chunk_size_small; //...unless we've been given an offset (probably indicating that it's looking for a tiny bit of metadata at the end)

						progress = rest;
						ifile.seekg(rest, std::ios::beg);
						rest = 0;
					}

					while (progress < fsize) {
						int64_t readCount = ((fsize - progress) < chunk_size) ? (fsize - progress) : chunk_size;

						data.resize(readCount);
						ifile.read((char*)data.data(), data.size());

						printf("sending part of file: %lld bytes into the file and size %lld (with fsize=%lld)\n", progress, readCount, fsize);
						asio::write(socket, asio::buffer(data.data(), data.size()));

						progress += readCount;

						if (!_alive) {
							break;
						}
					}

					ifile.close();
				}
				else {
					deliver(assembleResponse(FTPCode::NEG_TEMP_TRANSFER_ABORTED, "closing data connection, transmission failed (file couldn't be opened)"));
				}
				deliver(assembleResponse(FTPCode::POS_COMPLETE_CLOSING_DATA_CONNECTION, "closing data connection, transmission succeeded"));
				socket.close();
			}
			catch (std::exception& e) {
				std::cout << "taihen in file read: " << e.what() << "\n";
				deliver(assembleResponse(FTPCode::NEG_TEMP_TRANSFER_ABORTED, "closing data connection, transmission failed (" + (std::string)e.what() + ")"));
				socket.close();
			}
		}
	);

	//printf("badddd\n\n\n\n\n\n\n\n\n\n\n\n\n");
	/*std::string iinput = input;

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
	);*/
}