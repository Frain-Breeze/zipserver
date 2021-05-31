#pragma once

#include "ftp_codes.hpp"
#include <string>

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