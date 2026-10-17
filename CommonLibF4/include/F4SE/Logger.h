// Minimal in-project logger to avoid external dependencies (fmt/spdlog).
// Provides formatting wrappers similar to the previous API but uses
// C++20 <format> and a simple file sink. Pre-init messages are sent to
// OutputDebugString and buffered to the log file once init() runs.

#pragma once

#include <chrono>
#include <filesystem>
#include <fstream>
#include <format>
#include <mutex>
#include <optional>
#include <source_location>
#include <string>

// Forward-declare a few in-repo types to avoid heavyweight includes
namespace REL { class Version; }

namespace F4SE::log
{
	[[nodiscard]] std::optional<std::filesystem::path> log_directory();

	void init();

	// Internal writer used by the template wrappers. Defined in Logger.cpp
	void write_log(std::source_location a_loc, const char* a_level, const std::string& a_message);

	// Wrapper that implicitly captures source_location at the call site
	// via the default argument in the converting constructor.
	struct fmt_str
	{
		std::string_view str;
		std::source_location loc;

		fmt_str(const char* s, std::source_location l = std::source_location::current())
			: str(s), loc(l) {}
		fmt_str(std::string_view s, std::source_location l = std::source_location::current())
			: str(s), loc(l) {}
	};

	// Helper to safely build format args from forwarded parameters (ensures lvalues)
	template <class... Args>
	inline std::string vformat_runtime(std::string_view fmt, Args&&... args)
	{
		auto tup = std::tuple<std::decay_t<Args>...>(std::forward<Args>(args)...);
		return std::apply([&](auto&... vs) {
			return std::vformat(fmt, std::make_format_args(vs...));
		}, tup);
	}

	template <class... Args>
	void trace(fmt_str fmt, Args&&... args)
	{
		auto s = sizeof...(Args) > 0 ? vformat_runtime(fmt.str, std::forward<Args>(args)...) : std::string(fmt.str);
		write_log(fmt.loc, "TRACE", s);
	}

	template <class... Args>
	void debug(fmt_str fmt, Args&&... args)
	{
		auto s = sizeof...(Args) > 0 ? vformat_runtime(fmt.str, std::forward<Args>(args)...) : std::string(fmt.str);
		write_log(fmt.loc, "DEBUG", s);
	}

	template <class... Args>
	void info(fmt_str fmt, Args&&... args)
	{
		auto s = sizeof...(Args) > 0 ? vformat_runtime(fmt.str, std::forward<Args>(args)...) : std::string(fmt.str);
		write_log(fmt.loc, "INFO", s);
	}

	template <class... Args>
	void warn(fmt_str fmt, Args&&... args)
	{
		auto s = sizeof...(Args) > 0 ? vformat_runtime(fmt.str, std::forward<Args>(args)...) : std::string(fmt.str);
		write_log(fmt.loc, "WARN", s);
	}

	template <class... Args>
	void error(fmt_str fmt, Args&&... args)
	{
		auto s = sizeof...(Args) > 0 ? vformat_runtime(fmt.str, std::forward<Args>(args)...) : std::string(fmt.str);
		write_log(fmt.loc, "ERROR", s);
	}

	template <class... Args>
	void critical(fmt_str fmt, Args&&... args)
	{
		auto s = sizeof...(Args) > 0 ? vformat_runtime(fmt.str, std::forward<Args>(args)...) : std::string(fmt.str);
		write_log(fmt.loc, "CRIT", s);
	}
}
