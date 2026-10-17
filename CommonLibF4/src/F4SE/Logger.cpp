#include "F4SE/Logger.h"
#include "F4SE/API.h"
#include "F4SE/Interfaces.h"
#include "REL/Module.h"

#include "REX/W32/OLE32.h"
#include "REX/W32/SHELL32.h"

#include <Windows.h>

namespace F4SE::log
{
	namespace {
		std::mutex g_logMutex;
		std::ofstream g_logFile;
		bool g_initialized = false;

		std::string current_time()
		{
			using namespace std::chrono;
			auto now = system_clock::now();
			auto in_time_t = system_clock::to_time_t(now);
			auto ms = duration_cast<milliseconds>(now.time_since_epoch()) % 1000;
			std::tm buf{};
			localtime_s(&buf, &in_time_t);
			char out[64];
			std::snprintf(out, sizeof(out), "%02d:%02d:%02d.%03d", buf.tm_hour, buf.tm_min, buf.tm_sec, static_cast<int>(ms.count()));
			return std::string(out);
		}

		void ensure_directory(const std::filesystem::path& p)
		{
			std::error_code ec;
			std::filesystem::create_directories(p.parent_path(), ec);
			(void)ec;
		}
	}

	std::optional<std::filesystem::path> log_directory()
	{
		wchar_t* buffer{ nullptr };
		const auto result = REX::W32::SHGetKnownFolderPath(REX::W32::FOLDERID_Documents, REX::W32::KF_FLAG_DEFAULT, nullptr, std::addressof(buffer));
		std::unique_ptr<wchar_t[], decltype(&REX::W32::CoTaskMemFree)> knownPath(buffer, REX::W32::CoTaskMemFree);
		if (!knownPath || result != 0) {
			OutputDebugStringA("F4SE::log - failed to get known folder path\n");
			return std::nullopt;
		}

		std::filesystem::path path = knownPath.get();
		path /= std::format("My Games/{}/F4SE", F4SE::GetSaveFolderName());
		return path;
	}

	void init()
	{
		std::scoped_lock lock(g_logMutex);
		if (g_initialized)
			return;

		auto path = log_directory();
		if (!path)
			return;

		// Fix: GetSaveFolderName may return empty on some F4SE versions,
		// or log::init() may be called before F4SE::Init() populates it.
		// Ensure we're in the correct game subfolder — VR-aware.
		auto pathStr = path->generic_string();
		if (pathStr.find("Fallout4") == std::string::npos &&
		    pathStr.find("Fallout4VR") == std::string::npos)
		{
			// Path is "My Games/F4SE" — insert the runtime-appropriate folder.
			const char* folder = REL::Module::IsVR() ? "Fallout4VR" : "Fallout4";
			*path = path->parent_path() / folder / "F4SE";
		}

		// Name the log after the hosting plugin — this vendored copy once
		// hardcoded its project's name, which collided when two CommonLibF4
		// plugins ran in the same game and wrote to the same file.
		// GetPluginName() is populated from PluginVersionData in F4SE::Init
		// before log::init() runs.
		auto pluginName = std::string(F4SE::GetPluginName());
		if (pluginName.empty())
			pluginName = "F4SEPlugin";
		*path /= pluginName + ".log";

		ensure_directory(*path);

		g_logFile.open(path->string(), std::ios::out | std::ios::trunc);
		if (!g_logFile.is_open()) {
			OutputDebugStringA("F4SE::log - failed to open log file\n");
			return;
		}

		g_initialized = true;
	}

	void write_log(std::source_location a_loc, const char* a_level, const std::string& a_message)
	{
		std::string out = std::format("[{}] [{}] {}:{} {}", current_time(), a_level, a_loc.file_name(), a_loc.line(), a_message);

		{
			std::scoped_lock lock(g_logMutex);
			if (g_initialized && g_logFile.is_open()) {
				g_logFile << out << '\n';
				g_logFile.flush();
			}
		}

		OutputDebugStringA(out.c_str());
		OutputDebugStringA("\n");
	}
}
