//  Copyright Jean-Daniel Michaud 2007. Use, modification and
//  distribution is subject to the Boost Software License, Version
//  0.1. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#ifndef _LOG_SERVER_H_
#define _LOG_SERVER_H_

//#undef WIN32

#ifdef WIN32
#pragma warning(disable:4996)

// Stupid warnings shall not pass
#define _SCL_SECURE_NO_DEPRECATE
#define _CRT_SECURE_NO_DEPRECATE
#endif

#include <string>
#include <iostream>
#include <sstream>
#include <fstream>
#include <time.h>
#include <sys/types.h>
#include <sys/timeb.h>
#include <boost/thread/thread.hpp>
#include <boost/thread/condition.hpp>

#ifdef WIN32
 #include <windows.h>

 #ifdef _STACK_WALKER
  #include "StackWalker.h"
 #endif // !_STACK_WALKER

#endif // !WIN32


#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)
#define AT __FILE__ "(" TOSTRING(__LINE__) ")"

class LogServer;

#define TRACE(level, msg)                                      \
{                                                              \
	LogServer *logServer = LogServer::Instance();	            \
	std::stringstream trace;						                  \
	trace << msg << std::endl;						                  \
	logServer->trace(level, __FILE__, __LINE__, trace.str());	\
	logServer->flush();								                  \
}

#define TRACE_L1(msg) TRACE(1, msg)
#define TRACE_L2(msg) TRACE(2, msg)
#define TRACE_L3(msg) TRACE(3, msg)

class LogServer
{
public:
	LogServer(int logLevel = 1, bool onFile = true, bool onStdOutput = false, const std::string &outputFilename = "")	: _logLevel(logLevel), _onFile(onFile), _onStdOutput(onStdOutput)
	{ 
      _logFileName = ((outputFilename == "") ? ".\\wgrep.log" : outputFilename); 
		start();
#ifdef WIN32
    //SetUnhandledExceptionFilter(exceptionHandler);
#endif // !WIN32
	}
	
	~LogServer() { _logFile.close(); }

   static LogServer		*Instance(int logLevel = 1, bool onFile = true, bool onStdOutput = false, const std::string &outputFilename = "")
	{
		static LogServer	*logServer = NULL;
      if (logServer == NULL)
         logServer = new LogServer(logLevel, onFile, onStdOutput, outputFilename);
		return logServer;
	}

	int start()
	{ 
      if (_onFile)
      {
		   _logFile.open(_logFileName.c_str(), std::ofstream::out | std::ofstream::app);

		   if (_logFile.is_open())
		   {
			   trace(1, __FILE__, __LINE__, "Log server up and running", true);
			   trace(1, __FILE__, __LINE__, _logFileName, true);
		   }
		   else
		   {
			   std::cerr << "Error opening file" << std::endl; 
			   return 1;
		   }
      }
      return 0;
	}

	int getLogLevel(void) const { return _logLevel; }
	int setLogLevel(int logLevel) { _logLevel = logLevel; }
	
	const std::string getDate(void) const
	{
		struct _timeb time;
		_ftime(&time);
		char date[256];
		struct tm *today = localtime(&time.time);
		
		sprintf(date, "%02i/%02i/%02i %02i:%02i:%02i(%03i)", today->tm_mday, today->tm_mon, today->tm_year - 100, 
			today->tm_hour, today->tm_min, today->tm_sec, time.millitm);

		return date;
	}

	void trace(int logLevel, const char* filename, int location, const std::string& string, bool trailing_carriage_return = false)
	{
    boost::mutex::scoped_lock scoped_lock(_mutex);

    const char* it = filename + strlen(filename);
    for (; it >= filename; --it)
      if (*it == '\\')
      {
        ++it;
        break;
      }

      if (_logLevel >= logLevel)
      {
        if (_onFile)
          _logFile << "[" << logLevel << "]" << getDate() << " - " << it << "(" << location << ")" << " - " << string;
        if (_onStdOutput)
          std::cout << "[" << logLevel << "]" << getDate() << " - " << it << "(" << location << ")" << " - " << string;
      }
      if (trailing_carriage_return)
      {
        if (_onFile)
          _logFile << std::endl;
        if (_onStdOutput)
        {
          std::cout << std::endl;
          std::cout.flush();
        }
      }
	}

	void flush() { if (_onFile) _logFile.flush(); }
	void setQuiet(bool onStdOutput = false) { _onStdOutput = onStdOutput; }

   #ifdef WIN32
   static LONG __stdcall exceptionHandler(LPEXCEPTION_POINTERS pExceptPtrs)
   {
      TRACE_L2("exceptionHandler: A crash just occured");

     	PEXCEPTION_RECORD pExceptRec = pExceptPtrs->ExceptionRecord;

	   // Avoid recursive exceptions
	   if (!(pExceptRec->ExceptionFlags & 0x0010))
      {
		   TRACE_L1("Unhandled exception:"
               << " code = " << std::hex << pExceptRec->ExceptionCode
               << " flags = " << pExceptRec->ExceptionFlags
			      << " address = " << pExceptRec->ExceptionAddress << std::dec);

         if (pExceptRec->ExceptionCode == EXCEPTION_ACCESS_VIOLATION)
         {
            // Trace the details
            
            TRACE_L1 ("Exception parameter[0] " << pExceptRec->ExceptionInformation[0]
               << " parameter[1] " << std::hex <<  pExceptRec->ExceptionInformation[1]);
               
         }
         
         MEMORYSTATUS ms;
         ms.dwLength=sizeof(ms);
         GlobalMemoryStatus(&ms);
         
         // Trace the data
         
         TRACE_L1("Total phys. mem: " << ms.dwTotalPhys
               << " available: "          << ms.dwAvailPhys
               << " total page file: "    << ms.dwTotalPageFile
               << " available: "          << ms.dwAvailPageFile
               << " total VM: "           << ms.dwTotalVirtual
               << " available: "          << ms.dwAvailVirtual);
      }
 #ifdef _STACK_WALKER
      // TO DO: the stack walker
         // Dump the stack
//         MyStackWalker sw;
//         sw.ShowCallstack(GetCurrentThread(), pExp->ContextRecord);      }
 #endif // !_STACK_WALKER

      exit(3);
   }
   #endif // !WIN32

private:
	std::ofstream	_logFile;
	std::string		_logFileName;
	int				_logLevel;
	bool			   _onStdOutput;
	bool			   _onFile;
	boost::mutex	_mutex;
};


#endif
