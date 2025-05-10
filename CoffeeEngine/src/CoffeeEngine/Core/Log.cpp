#include "CoffeeEngine/Core/Log.h"

#include <memory>
#include <mutex>
#include <spdlog/logger.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/spdlog.h>
#include <string>
#include <vector>

namespace Coffee
{
    std::shared_ptr<spdlog::logger> Log::s_CoreLogger;
    std::shared_ptr<spdlog::logger> Log::s_ClientLogger;
    std::vector<std::string> Log::s_LogBuffer;

    void Log::Init()
    {
        spdlog::set_pattern("%^[%T] %n: %v%$");

        auto imGuiSink = std::make_shared<LogSink<std::mutex>>();
        imGuiSink->set_level(spdlog::level::trace);

        auto fileSink = std::make_shared<spdlog::sinks::rotating_file_sink<std::mutex>>("logs/Debug.log", 1024*1024 * 5, 5, false); // 5 5MiB files
        fileSink->set_level(spdlog::level::trace);

        s_CoreLogger = spdlog::stdout_color_mt("CORE");
        s_CoreLogger->set_level(spdlog::level::trace);
        s_CoreLogger->sinks().push_back(imGuiSink);
        s_CoreLogger->sinks().push_back(fileSink);
        s_CoreLogger->flush_on(spdlog::level::warn);

        s_ClientLogger = spdlog::stdout_color_mt("APP");
        s_ClientLogger->set_level(spdlog::level::trace);
        s_ClientLogger->sinks().push_back(imGuiSink);
        s_ClientLogger->sinks().push_back(fileSink);
        s_ClientLogger->flush_on(spdlog::level::warn);

        spdlog::flush_every(std::chrono::seconds(5));
    }

} // namespace Coffee
