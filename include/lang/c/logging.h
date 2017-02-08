/*
 * Copyright 2010-2016 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 * You may not use this file except in compliance with the License.
 * A copy of the License is located at
 *
 *  http://aws.amazon.com/apache2.0
 *
 * or in the "license" file accompanying this file. This file is distributed
 * on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied. See the License for the specific language governing
 * permissions and limitations under the License.
 */

/**
 * @file Logging.hpp
 * @brief
 *
 */

#ifndef _AWS_IOT_DEVICE_SDK_LOGGING_H_
#define _AWS_IOT_DEVICE_SDK_LOGGING_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    AWS_LOG_SYS_CONSOLE = 1
} log_system_type_t;

typedef enum {
    AWS_LOG_LEVEL_OFF   = 0,
    AWS_LOG_LEVEL_FATAL = 1,
    AWS_LOG_LEVEL_ERROR = 2,
    AWS_LOG_LEVEL_WARN  = 3,
    AWS_LOG_LEVEL_INFO  = 4,
    AWS_LOG_LEVEL_DEBUG = 5,
    AWS_LOG_LEVEL_TRACE = 6
} log_level_t;

void initialize_aws_logging(log_system_type_t log_system_type,
    log_level_t log_level);

/**
 * Call this at the exit point of your program, after all calls have finished.
 */
void shutdown_aws_logging(void);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* _AWS_IOT_DEVICE_SDK_LOGGING_H_ */
