/*******************************************************
 * Copyright (C) 2013-2014 Davide Faconti, Icarus Technology SL Spain>
 * All Rights Reserved.
 *
 * This file is part of CAN/MoveIt Core library
 *
 * Dissemination of this information or reproduction of this material
 * is strictly forbidden unless prior written permission is obtained
 * from Icarus Technology SL Incorporated.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
 * SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
 * FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *******************************************************/

#ifndef LOG_H
#define LOG_H

#include <memory>
#include "spdlog/spdlog.h"

namespace CanMoveIt {

// Simple static interface to access the loggers.
// For custamization of the logger, please refer to https://github.com/gabime/spdlog
class Log{

public:
    typedef std::shared_ptr<spdlog::logger> LoggerPtr;

    static LoggerPtr& CAN()
    {
        static auto _log = spdlog::stdout_color_mt("CAN");
        return _log;
    }
    static LoggerPtr& CO301()
    {
        static auto _log = spdlog::stdout_color_mt("CO301");
        return _log;
    }

    static LoggerPtr& SYS()
    {
        static auto _log = spdlog::stdout_color_mt("SYS");
        return _log;
    }
    static LoggerPtr& MAL()
    {
        static auto _log = spdlog::stdout_color_mt("MAL");
        return _log;
    }
};

// Note: in case of problem at destruction time, refer to:
// https://stackoverflow.com/questions/335369/finding-c-static-initialization-order-problems/335746#335746

} // end namespace


#endif // LOG_H
