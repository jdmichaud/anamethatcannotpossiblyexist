//  Copyright Jean-Daniel Michaud 2007. Use, modification and
//  distribution is subject to the Boost Software License, Version
//  0.1. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#pragma warning(disable:4996)
#pragma warning(disable:4786)
#pragma warning(disable:4005)

#include <iostream>
#include <crtdbg.h>
#include <boost/thread.hpp>
#include "log_server.h"
#include "fx.h"
#include "wgrep_app.h"
#include "wgrep_wnd.h"
#include "output_wnd.h"
#include "limit_single_instance.h"

#define VERSION 0.4.0

#ifdef _DEBUG
 LogServer *logServer = LogServer::Instance(2, true, true,  GrepApp::getKeyValue("HKEY_CURRENT_USER", "Software\\wgrep", "wgrepPath") + "\\wgrep.log");
#else
 LogServer *logServer = LogServer::Instance(1, true, false, GrepApp::getKeyValue("HKEY_CURRENT_USER", "Software\\wgrep", "wgrepPath") + "\\wgrep.log");
#endif

OutputWindow *outputWindow[256];
bool          busyOutput[256];

int main(int argc, char *argv[])
{
   TRACE_L1("wgrep version " << TOSTRING(VERSION) << " up");

   TRACE_L2("main: argc == " << argc);
   for (unsigned short i = 0; i < argc; ++i)
      TRACE_L2("main: argv[" << i << "] == " << argv[i]);

   // Make application
   GrepApp* application = new GrepApp("Find in files");

   memset(busyOutput, false, 256 * sizeof(bool));

   // Start app
   application->init(argc, argv);
   (new FXToolTip(application, 0))->setText("Find in files");

   // Windows
   GrepWindow *grepWindow = NULL;
   if ((argc >= 2) && (strcmp(argv[1], "-tracelevel")))
   {
      TRACE_L1("main: folder : " << argv[1] << " passed as parameter");
      grepWindow = new GrepWindow(application, argv[1]);
   }
   else
      grepWindow = new GrepWindow(application);

   CSingleInstanceApp instance(TEXT("wgrep jean-daniel.michaud@laposte.net"), application);

   // Set the window to forward message
   instance.setMainWindow(grepWindow);
   try
   {
     boost::thread thrd(instance);
   }
   catch (...)
   {
     std::cout << "################### exception ###################" << std::endl;
   }

   // Create the application's windows
   application->create();

   // Run the application
   application->run();

   // Dump memory leaks
   _CrtDumpMemoryLeaks();

   TRACE_L1("wgrep down");
   return 0;
}
