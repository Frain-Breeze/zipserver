#pragma once

#include <chrono>
#include <filesystem>

namespace util_file {
	tm* last_modify(const std::filesystem::path& path) {
		const auto time_modified = std::filesystem::last_write_time(path);
		auto elapse = std::chrono::duration_cast<std::chrono::seconds>(std::filesystem::file_time_type::clock::now().time_since_epoch() - std::chrono::system_clock::now().time_since_epoch()).count();
		auto systemTime = std::chrono::duration_cast<std::chrono::seconds>(time_modified.time_since_epoch()).count() - elapse;
		return localtime(&systemTime);
	}

	std::string write_time_unix(tm* lsystemTime) {
		const char* months[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
		char buf[256];
		snprintf(buf, 256, "%s %d %d:%d", months[lsystemTime->tm_mon], lsystemTime->tm_mday, lsystemTime->tm_hour, lsystemTime->tm_min);
		return buf;
	}

	std::string write_time_mlsd(tm* lsystemTime) {
		char buf[256];
		snprintf(buf, 256, "Modify=%04d%02d%02d%02d%02d%02d.000", lsystemTime->tm_year + 1900, lsystemTime->tm_mon + 1, lsystemTime->tm_mday, lsystemTime->tm_hour, lsystemTime->tm_min, lsystemTime->tm_sec);
		return buf;
	}
}

