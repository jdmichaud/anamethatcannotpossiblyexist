/////////////////////////////////////////////////////////////////////////////
// SingleInstanceApp.h
// Abstract:
//		SingleInstanceApp class implements functionality to make
//		single instance app.	
//
// (L) Copyleft Maxim Egorushkin. e-maxim@yandex.ru
// Thanks by e-mail would be enough to use it :)

#pragma once
#include <windows.h>
#include <string>
#include "wgrep_wnd.h"
#include "log_server.h"

/////////////////////////////////////////////////////////////////////////////
// CSingleInstanceApp
// Abstract:
//		MFC friendly wrapper of SingleInstanceApp. Derive your application
//		class from it instead of CWinApp.

class CSingleInstanceApp
{
public:
   CSingleInstanceApp(const std::string &pszUniqueString, GrepApp *application) 
      : _uniqueString(pszUniqueString), _application(application)
   {
   }

   void setMainWindow(GrepWindow *mainWindow)
   {
      _mainWindow = mainWindow;
   }

	// parameters:
	//		[in] pszUniqueString - string to give the file mapping 
	//		system unique name. GUID would fit the best.
	//		[out] uMessage - variable to register message to.
	//		This message will be posted to first app instance thread.
	//		So, intrested app can process this message to get the command line
	//		the second instance was tried to launch with.
	void createInstance(LPCTSTR pszUniqueString)
   {
      _uMessage = ::RegisterWindowMessage(pszUniqueString);
      TRACE_L2("createInstance: unique string: " << pszUniqueString << " message handle: " << _uMessage);

      _hMapping = ::CreateFileMapping(
         INVALID_HANDLE_VALUE,
         0,
         PAGE_READWRITE,
         0,
         sizeof(*_pSharedData),
         pszUniqueString);

      TRACE_L2("createInstance: map handle: " << std::hex << _hMapping);
      
      // ::GetLastError returns ERROR_ALREADY_EXISTS for the second instance
      //S_OK == ::GetLastError();
      DWORD lastError = ::GetLastError();
      _bAmITheFirst = (lastError == S_OK);
      
      _pSharedData = static_cast<SharedData*>(::MapViewOfFile(_hMapping, FILE_MAP_WRITE, 0, 0, 0));
      
      if (_bAmITheFirst)
      {
         // set the thread ID of the first instance to the shared mapping
         _pSharedData->dwThreadID = ::GetCurrentThreadId();
         TRACE_L2("createInstance: GetCurrentThreadId(): " << _pSharedData->dwThreadID);
         // set empty command line
         _pSharedData->pszCmdLine[0] = 0; 
      }
      else
      {
         // copy command line to shared mapping
         ::lstrcpyn(_pSharedData->pszCmdLine, ::GetCommandLine(), 
            sizeof(_pSharedData->pszCmdLine) / sizeof(*_pSharedData->pszCmdLine));
         // post the notification message to the first instance
         ::PostThreadMessage(_pSharedData->dwThreadID, _uMessage, 0, 0);
         TRACE_L2("createInstance: sending " << _uMessage << " to thread " << _pSharedData->dwThreadID);
      }
   }


  ~CSingleInstanceApp()
  {
    ::UnmapViewOfFile(_pSharedData);
    //::CloseHandle(_hMapping); // Crash on this
  }

	// returns:
	//		true if running the first instance
	bool isFirst() const
   {
	   return _bAmITheFirst;
   }

  // returns:
  //		The command line the second instance was tried to launch with.
  LPCTSTR GetSecondInstanceCmdLine() const
  {
    return _pSharedData->pszCmdLine;
  }

   // Override this to get notified when second instance was tried to launch.
	virtual void OnSecondInstance()
  {
    TRACE_L1("---- Second instance was tried to launch.");
    TRACE_L1("---- Command line is \"" << GetSecondInstanceCmdLine() << "\".");
    TRACE_L1("---- Override CSingleInstanceApp::OnSecondInstance to do something.");
    _mainWindow->newExternalInstance(GetSecondInstanceCmdLine());
  }

   void operator()()
   {
      TRACE_L1("CSingleInstanceApp(): listening to wgrep application creation...");

      createInstance(_uniqueString.c_str());
      if (!isFirst())
      {
         TRACE_L1("CSingleInstanceApp(): wgrep is already launched. Sending message to present instance...");
         TRACE_L1("CSingleInstanceApp(): wgrep down");
         _application->exit();
         exit(1); // just in case
      }

      MSG msg;
      while (GetMessage(&msg, NULL, 0, 0))
      {
         if (_uMessage == msg.message)
         {
            TRACE_L1("CSingleInstanceApp(): external wgrep launched.");
            OnSecondInstance();
            continue;
         }
      }
   }

private:
	bool        _bAmITheFirst;
	HANDLE      _hMapping;
	
struct SharedData
	{
		DWORD dwThreadID;
		TCHAR pszCmdLine[0x800];
	}           *_pSharedData;

  GrepWindow  *_mainWindow;
  GrepApp     *_application;
  std::string _uniqueString;
  UINT        _uMessage;
};

/////////////////////////////////////////////////////////////////////////////
