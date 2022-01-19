#pragma once

#include "ftp_server.hpp"

#include <algorithm>
#include <filesystem>
#include <unordered_map>

namespace fs = std::filesystem;

std::string Session::clean_root_point(const std::string& to_clean) {
	std::string input = to_clean;
	input.erase(std::remove(input.begin(), input.end(), '/'), input.end());
	return input;
}

fs::path Session::strip_root_point(const fs::path& to_strip) {
	fs::path ret_rel = to_strip.relative_path();
	if (ret_rel.empty()) { return ret_rel; }
	ret_rel = ret_rel.lexically_relative(*ret_rel.begin());
	printf("ret_rel = %s\n", ret_rel.u8string().c_str());
	if(ret_rel == ".") { ret_rel = ""; }
	return ret_rel;
}

fs::path Session::assemble_path(const std::string& root, const fs::path& virt) {
	fs::path ret;

	const auto found_root = root_points.find(root);
	if (found_root != root_points.end()) {
		ret = found_root->second;
	}
	ret /= virt;

	return ret;
}