/*
 * CApp.cpp
 *
 *  Created on: 23 Oct 2011
 *      Author: martin
 */

#include "CApp.h"

#include <log4cplus/logger.h>
#include <log4cplus/loglevel.h>
#include <log4cplus/loggingmacros.h>
#include <log4cplus/layout.h>
#include <log4cplus/configurator.h>
#include <log4cplus/fileappender.h>
#include <log4cplus/consoleappender.h>
#include <log4cplus/ndc.h>

#include <iostream>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>

#include <string.h>
#include <boost/filesystem/operations.hpp>
#include <boost/system/error_code.hpp>

namespace muroa {

using namespace std;
using namespace log4cplus;

namespace bfs = boost::filesystem;

CApp* CApp::m_inst_ptr = 0;
std::mutex CApp::m_mutex;


CSettings& CApp::settings() { return CApp::getInstPtr()->m_settings; }
Logger& CApp::logger() { return CApp::getInstPtr()->m_logger; };

CSettings& CApp::getSettingsRef() { return m_settings; }
log4cplus::Logger& CApp::getLoggerRef() { return m_logger; }

CApp::CApp(int argc, char** argv) throw(configEx) : m_settings(this)
{
    m_called_from_path = bfs::current_path();

    bfs::path abs_prog_path(argv[0]);
    if( abs_prog_path.is_relative()) {
    	abs_prog_path = m_called_from_path / abs_prog_path;
    }

    m_prog_name = abs_prog_path.filename().string();
    abs_prog_path.remove_filename();
    m_abs_prog_dir = bfs::canonical(abs_prog_path);

    if( m_settings.parse(argc, argv) != 0) {
    	throw configEx("error parsing commandline parameters");
    }

    m_error_handler_ptr = auto_ptr<log4cplus::ErrorHandler>(new CAppenderErrorHandler);
    initLog();

    m_settings.readConfigFile();
}

CApp::~CApp() {}

CApp* CApp::getInstPtr(int argc, char** argv) throw(configEx) {
	lock_guard<mutex> lk(m_mutex);
	if( m_inst_ptr == 0) {
		m_inst_ptr = new CApp(argc, argv);
	}
	return m_inst_ptr;
}

void CApp::serviceChanged() {
	std::cerr << "CApp::serviceChanged()" << std::endl;
}

void CApp::initLog() {
    try
    {
        log4cplus::PropertyConfigurator::doConfigure("log.properties");
    }
    catch( ... )
    {
       std::cerr<<"Exception occured while opening log.properties\n";
       BasicConfigurator config;
       config.configure();
    }

    m_logger = Logger::getInstance("main");
    Appender* appender;
    bool logfile_accessible = accessible(m_settings.logfile());
    if( logfile_accessible ) {
        appender = new FileAppender(m_settings.logfile());
        appender->setErrorHandler(m_error_handler_ptr);
    }
    else {
    	appender = new ConsoleAppender();
    	appender->setErrorHandler(m_error_handler_ptr);
        //SharedAppenderPtr log_appender(console_appender);

    }
    SharedAppenderPtr log_appender(appender);
	log_appender->setName("LogAppender");
	std::auto_ptr<Layout> myLayout = std::auto_ptr<Layout>(new log4cplus::TTCCLayout());
	log_appender->setLayout(myLayout);

	m_logger.addAppender(log_appender);
    // logger.setLogLevel ( DEBUG_LOG_LEVEL );
	m_logger.setLogLevel ( m_settings.debuglevel() );
    if( !logfile_accessible ) {
    	LOG4CPLUS_WARN(m_logger, "Could not open logfile '" << m_settings.logfile() << "'. Logging to console instead." << endl
    			                  << "Pass '--logfile </path/to/file.log>' to specify a logfile writable by '" << m_prog_name << "'.");
    }
}


bool CApp::accessible(string logfile_name) {
	bfs::path lf(logfile_name);
	if(bfs::is_directory(lf)) {
		return false;
	}
	else {
		int rc = access(logfile_name.c_str(), R_OK|W_OK);
		if(rc == 0) {
			return true;
		}
		else {
			switch (errno) {
			case EACCES:
			case EROFS:
				return false;

			case ENOENT:
			{
				// try to create parent path, if not yet there:
				boost::system::error_code ec;
				create_directories(lf.parent_path(), ec);
				if( ec ) {
					// could not create path to logfile
					return false;
				}
				int wr_fd = open(logfile_name.c_str(), O_RDWR|O_CREAT|FD_CLOEXEC, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH);
				if(wr_fd == -1) {
					return false;
				}
				else {
					close(wr_fd);
					return true;
				}
			}
			default:
				return false;
			}
		}
	}
	return false;
}

int CApp::daemonize() {
	errno = 0;
    if ( pid_t pid = fork() ) {
    	if (pid > 0) {
    		// in the parent process, exit.
    		//
    		LOG4CPLUS_INFO(logger(), "forking to background ...");
    		cout << "forking to background ..." << endl;
    		exit(0);
    	}
    	else {
    		// pid < 0 -> failed to fork
    		LOG4CPLUS_ERROR(logger(), "failed to fork" << strerror(errno) );
    		return 1;
    	}
    }
    else {
    	// in the child process
        // Make the process a new session leader. This detaches it from the
        // terminal.
        setsid();

        // A process inherits its working directory from its parent. This could be
        // on a mounted filesystem, which means that the running daemon would
        // prevent this filesystem from being unmounted. Changing to the root
        // directory avoids this problem.
        chdir("/");

        // The file mode creation mask is also inherited from the parent process.
        // We don't want to restrict the permissions on files created by the
        // daemon, so the mask is cleared.
        umask(0);

        // A second fork ensures the process cannot acquire a controlling terminal.
        errno = 0;
        if (pid_t pid = fork()) {
        	if (pid > 0) {
        		exit(0);
        	}
        	else {
        		LOG4CPLUS_ERROR(logger(), "second fork failed: " << strerror(errno) );
        		return 1;
        	}
        }

        // Close the standard streams. This decouples the daemon from the terminal
        // that started it.
        close(0);
        close(1);
        close(2);

        // We don't want the daemon to have any standard input.
        errno = 0;
        if (open("/dev/null", O_RDONLY) < 0)
        {
    		LOG4CPLUS_ERROR(logger(), "Unable to open /dev/null: " << strerror(errno) );
    		return 1;
        }

        // Send standard output to a log file.
        const char* output = "/tmp/muroa.log";
        const int flags = O_WRONLY | O_CREAT | O_APPEND;
        const mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
        errno = 0;
        if (open(output, flags, mode) < 0)
        {
    		LOG4CPLUS_ERROR(logger(), "Unable to open output file " << output << " :"<< strerror(errno) );
    		return 1;
        }

        // Also send standard error to the same log file.
        if (dup(1) < 0)
        {
    		LOG4CPLUS_ERROR(logger(), "Unable to duplicate stdout" );
    		return 1;
        }
    }
    return 0;

}

} /* namespace muroa */

#ifdef MUROA_FAKE_LOG4CPLUS_RVALREF
namespace log4cplus {
Logger::Logger (Logger && rhs)
{
    value = rhs.value;
    rhs.value = 0;
}


Logger &
Logger::operator = (Logger && rhs)
{
    Logger (std::move (rhs)).swap (*this);
    return *this;
}
}
#endif

