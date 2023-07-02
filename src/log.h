#pragma once

#include <iostream>
#include <chrono>
#include <ctime>

#ifndef LPVPN_VERSION
#define LPVPN_VERSION "unknown_version"
#endif

#ifndef LPVPN_GIT_VERSION
#define LPVPN_GIT_VERSION "unknown_git_version"
#endif

constexpr const char* file_name(const char* path) {
	const char* file = path;
	while (*path) {
		if (*path++ == '/' || *path++ == '\\') {
			file = path;
		}
	}
	return file;
}

#define LOG(msg) \
	do { \
		auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()); \
		auto utcnow = std::gmtime(&now); \
		char buf[100] = {0}; \
		strftime(buf, sizeof(buf), "[%Y-%m-%dT%H:%M:%S]", utcnow); \
		std::clog << buf << " v" LPVPN_VERSION "(" LPVPN_GIT_VERSION ") " << file_name(__FILE__) << ":" << __LINE__ << " " << msg << std::endl; \
	} while (0);
