// ---------------------------------------------------------------------------
//
// This file is part of SymPol
//
// Copyright (C) 2006-2010  Thomas Rehn <thomas@carmen76.de>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
// ---------------------------------------------------------------------------

#ifndef YAL_LOGGER_H
#define YAL_LOGGER_H

#include "reportlevel.h"
#include "usagestats.h"

#include <boost/shared_ptr.hpp>
#include <string>
#include <ostream>
#include <sstream>
#include <iomanip>

namespace yal {
    class Logger;
    typedef boost::shared_ptr<Logger> LoggerPtr;
    
    class Logger {
    public:
        static LoggerPtr getLogger(const std::string& name) {
            return LoggerPtr(new Logger(name));
        }
        
        std::ostringstream& get(LogLevel level) {
            m_level = level;
            m_os << m_name;
            switch (level) {
                case ERROR:
                    m_os << " ERROR:  ";
                    break;
                case WARNING:
                    m_os << " WARN:   ";
                    break;
                case INFO:
                    m_os << " INFO:   ";
                    break;
                case DEBUG:
                    m_os << " DEBUG:  ";
                    break;
                case DEBUG2:
                    m_os << " DEBUG2: ";
                    break;
                case DEBUG3:
                    m_os << " DEBUG3: ";
                    break;
                case DEBUG4:
                    m_os << " DEBUG4: ";
                    break;
                default:
                    m_os << " xyz:";
                    break;
            }
            return m_os;
        }
        
        void logUsageStats() {
#if HAVE_SYSCONF_PROTO && HAVE_GETRUSAGE_PROTO
            std::cout << "USAGE:     " << UsageStats::processTimeUser() << "s  @ " 
                << UsageStats::processSize() / 1024 << "K" << std::endl;
#endif
        }
        
        void flush() { 
            if (m_level > ReportLevel::get()) {
                return;
            }
            
            std::cout << m_os.str();
            std::cout.flush();
            m_os.str(std::string());
        }
        
        LogLevel level() const { return m_level; }
    private:
        Logger(const std::string& name) : m_name(name) {}
        
        std::string m_name;
        std::ostringstream m_os;
        LogLevel m_level;
    };
    
#define YALLOG_ERROR(LOG, X) \
  if (yal::ERROR <= yal::ReportLevel::get()) { \
      LOG->get(yal::ERROR) << X << std::endl; \
      LOG->flush(); \
  }

#define YALLOG_WARNING(LOG, X) \
  if (yal::WARNING <= yal::ReportLevel::get()) { \
      LOG->get(yal::WARNING) << X << std::endl; \
      LOG->flush(); \
  }

#define YALLOG_INFO(LOG, X) \
  if (yal::INFO <= yal::ReportLevel::get()) { \
      LOG->get(yal::INFO) << X << std::endl; \
      LOG->flush(); \
  }
  
#define YALLOG_DEBUG(LOG, X) \
  if (yal::DEBUG <= yal::ReportLevel::get()) { \
      LOG->get(yal::DEBUG) << X << std::endl; \
      LOG->flush(); \
  }

#define YALLOG_DEBUG2(LOG, X) \
  if (yal::DEBUG2 <= yal::ReportLevel::get()) { \
      LOG->get(yal::DEBUG2) << X << std::endl; \
      LOG->flush(); \
  }

#define YALLOG_DEBUG3(LOG, X) \
  if (yal::DEBUG3 <= yal::ReportLevel::get()) { \
      LOG->get(yal::DEBUG3) << X << std::endl; \
      LOG->flush(); \
  }

#define YALLOG_DEBUG4(LOG, X) \
  if (yal::DEBUG4 <= yal::ReportLevel::get()) { \
      LOG->get(yal::DEBUG4) << X << std::endl; \
      LOG->flush(); \
  }
}

#endif

