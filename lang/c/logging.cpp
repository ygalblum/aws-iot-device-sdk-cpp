#include "logging.h"

#include "util/logging/Logging.hpp"
#include "util/logging/LogMacros.hpp"
#include "util/logging/ConsoleLogSystem.hpp"

using namespace awsiotsdk::util::Logging;

extern "C" {

void initialize_aws_logging(log_system_type_t log_system_type,
    log_level_t log_level)
{

	std::shared_ptr<LogSystemInterface> p_log_system = nullptr;

    switch (log_system_type) {
    case AWS_LOG_SYS_CONSOLE:
        p_log_system = std::make_shared<ConsoleLogSystem>((LogLevel)log_level);
        break;
    default:
        return;
    }

    InitializeAWSLogging(p_log_system);
}

/**
 * Call this at the exit point of your program, after all calls have finished.
 */
void shutdown_aws_logging(void)
{
    ShutdownAWSLogging();
}

} /* extern "C" */
