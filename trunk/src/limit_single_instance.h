#include <iostream>
#include "log_server.h"
#include "XmlRpc.h"
using namespace XmlRpc;

//Forward declaration
class NewInstance;

class CommandHandler
{
public:
  CommandHandler();
  void operator()();
  ~CommandHandler();

  void setMainWindow(GrepWindow *mainWindow)
  {
    _mainWindow = mainWindow;
  }

  XmlRpcServer  m_s;
  GrepWindow    *_mainWindow;
  int           m_portNumber;
  NewInstance   *m_new_intance_function_object;
};

class NewInstance : public XmlRpcServerMethod
{
public:
  NewInstance(XmlRpcServer* s, CommandHandler *c) : XmlRpcServerMethod("new_instance", s) 
  {
    m_c = c;
  }

  void execute(XmlRpcValue& params, XmlRpcValue& result)
  {
    std::string args = std::string(params[0]);
    TRACE_L1("NewInstance::execute: received a connection with command line: " << args);
    if (m_c->_mainWindow)
    {
      TRACE_L1("NewInstance::execute: calling newExternalInstance");
      m_c->_mainWindow->newExternalInstance(args);
    }
  }

  CommandHandler *m_c;
};


class InstanceDetector
{
public:
  InstanceDetector() {}

  bool new_instance(int argc, char **argv)
  {
    std::string command_line = argv[0];
    for (int i = 1; i < argc; ++i)
      command_line += std::string(" ") + argv[i];

    XmlRpcClient c("localhost", 12000);
    XmlRpcValue args = command_line;
    XmlRpcValue result;

    TRACE_L1("InstanceDetector::new_instance called with command line: " << command_line);
    if (c.execute("new_instance", args, result))
    {
      TRACE_L1("InstanceDetector::new_instance wgrep is already launched");
      return false;
    }
    else
    {
      TRACE_L1("InstanceDetector::new_instance we are the first!");
      return true;
    }
  }
};

CommandHandler::CommandHandler()
{ 
  m_portNumber = 12000; 
  m_new_intance_function_object = new NewInstance(&m_s, this);
}

CommandHandler::~CommandHandler()
{
  delete m_new_intance_function_object;
}

void CommandHandler::operator ()()
{
  XmlRpc::setVerbosity(0);

  // Create the server socket on the specified port
  m_s.bindAndListen(12000);

  // Enable introspection
  m_s.enableIntrospection(true);

  TRACE_L1("CommandHandler: Waiting for a connection on " << m_portNumber << " ...");
  // Wait for requests indefinitely
  m_s.work(-1.0);
}
