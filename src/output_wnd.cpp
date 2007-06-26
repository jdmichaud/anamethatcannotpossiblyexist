//  Copyright Jean-Daniel Michaud 2007. Use, modification and
//  distribution is subject to the Boost Software License, Version
//  0.1. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#include <boost/thread/thread.hpp>
#include <boost/thread/condition.hpp>
#include <boost/filesystem/operations.hpp> // includes boost/filesystem/path.hpp
#include <boost/filesystem/fstream.hpp>    // ditto
#include <boost/filesystem/exception.hpp>
#include <fstream>
#include "log_server.h"
#include "output_wnd.h"
#include "wgrep_wnd.h"

unsigned short       nextFreeSlot = 0;
boost::mutex	      callbackMutex;

// Map
FXDEFMAP(OutputWindow) OutputWindowMap[] = {
  FXMAPFUNC(SEL_COMMAND,           OutputWindow::ID_CLOSE,           OutputWindow::onCmdClose),
  FXMAPFUNC(SEL_CLICKED,           OutputWindow::ID_LISTSELECT,      OutputWindow::onListClicked),
  FXMAPFUNC(SEL_RIGHTBUTTONRELEASE,OutputWindow::ID_LISTSELECT,      OutputWindow::onListRightClicked),
  FXMAPFUNC(SEL_DOUBLECLICKED,     OutputWindow::ID_LISTSELECT,      OutputWindow::onListDoubleClicked),
  FXMAPFUNC(SEL_COMMAND,           FXWindow::ID_VSCROLLED,           OutputWindow::onVScrollChanged),
  FXMAPFUNC(SEL_CHANGED,           FXWindow::ID_VSCROLLED,           OutputWindow::onVScrollChanged),
  FXMAPFUNC(SEL_COMMAND,           FXWindow::ID_HSCROLLED,           OutputWindow::onVScrollChanged),
  FXMAPFUNC(SEL_CHANGED,           FXWindow::ID_HSCROLLED,           OutputWindow::onVScrollChanged),
  FXMAPFUNC(SEL_TIMEOUT,           FXWindow::ID_AUTOSCROLL,          OutputWindow::onVScrollChanged),
  FXMAPFUNC(SEL_FOCUSIN,           NULL,                             OutputWindow::onFocusIn),
  FXMAPFUNC(SEL_KEYPRESS,          NULL,                             OutputWindow::onKeyPress),
  FXMAPFUNC(SEL_COMMAND,           OutputWindow::ID_OPEN_AS_TEXT,    OutputWindow::onOpenResultAsText),
  FXMAPFUNC(SEL_COMMAND,           OutputWindow::ID_OPEN_CONTAINING, OutputWindow::onOpenContainingFolder),
  FXMAPFUNC(SEL_COMMAND,           OutputWindow::ID_TOGGLE_ONTOP,    OutputWindow::onTop)
};

// Object implementation
FXIMPLEMENT(OutputWindow, FXMainWindow, OutputWindowMap, ARRAYNUMBER(OutputWindowMap))

OutputWindow::OutputWindow(FXApp *a, 
                           GrepWindow *mainWindow, 
                           unsigned short index) 
                           : _index(index),
                             _mainWindow(mainWindow),
                             FXMainWindow(a, "Output", NULL, NULL, 
                                          DECOR_ALL | LAYOUT_SIDE_BOTTOM, 150, 700, 1024, 200)
{
   TRACE_L1("OutputWindow::OutputWindow constructor");

   font = new FXFont(getApp(), "courier new", 9);

   _outputlist = new FXList(this, this, ID_LISTSELECT, LAYOUT_FILL_X | LAYOUT_FILL_Y | TEXT_SHOWACTIVE, 0, 0, 1024, 400);
   _outputlist->setFont(font);
   _outputlist->horizontalScrollBar()->setTarget(this);
   _outputlist->verticalScrollBar()->setTarget(this);
   
   _editorPath = getApp()->getKeyValue("HKEY_CURRENT_USER", "SOFTWARE\\wgrep", "EditorPath");
   _editorName = getApp()->getKeyValue("HKEY_CURRENT_USER", "SOFTWARE\\wgrep", "EditorName");
   _wgrepPath  = getApp()->getKeyValue("HKEY_CURRENT_USER", "SOFTWARE\\wgrep", "wgrepPath");

   setIcon(getApp()->grepIcon);
   setMiniIcon(getApp()->grepIcon);

   _itemIndex = 1;

   _rcmenu    = new FXMenuPane(this);
   //new FXMenuTitle(_rcmenu, tr("&File"), NULL, filemenu);
   new FXMenuCommand(_rcmenu, tr("&Open containing folder"), NULL /*getApp()->openIcon*/, this, ID_OPEN_CONTAINING);
   new FXMenuCommand(_rcmenu, tr("Open in Microsoft &Visual Studio"), NULL /*getApp()->saveIcon*/, this, ID_OPEN_VISUAL);
   new FXMenuCommand(_rcmenu, tr("Open as &text file"), NULL /*getApp()->closeIcon*/, this, ID_OPEN_AS_TEXT);
   new FXMenuSeparator(_rcmenu);
   (new FXMenuCheck(_rcmenu, tr("&Stay on top"), this, ID_TOGGLE_ONTOP))->setCheck(false);
   new FXMenuSeparator(_rcmenu);
   new FXMenuCommand(_rcmenu, tr("&Close"), NULL /*getApp()->closeIcon*/, this, ID_CLOSE);

   _onTop = false;
}

void OutputWindow::create()
{
   // Create the windows
   FXMainWindow::create();
}

void OutputWindow::appendText(const std::string& text)
{
   _outputlist->appendItem(text.c_str());
   if (_autoScroll) _outputlist->makeItemVisible(_outputlist->getNumItems() - 1);
   _outputlist->repaint();
}

void OutputWindow::prepare(const std::string &pattern, const std::string &folder)
{
   _outputlist->clearItems();
   _pattern    = pattern;
   _folder     = folder;
   setExplicitTitle("");
   _autoScroll = true;
   _rcmenu->hide();
   show();
   raise();
}

void OutputWindow::setExplicitTitle(const std::string& suffix)
{
   this->setTitle((std::string("\"") + _pattern + "\" in " + ((suffix == "") ? _folder : suffix)).c_str());
}

long OutputWindow::onCmdClose(FXObject*, FXSelector, void*)
{
   TRACE_L1("OutputWindow::onCmdClose called");

   if (_grep)
   {
     delete _grep;
     _grep = NULL;
   }

   if (_thread)
   {
     delete _thread;
     _thread = NULL;
   }
   _rcmenu->hide();
   close();
   busyOutput[_wndIndex] = false;
   return 0;
}

long OutputWindow::onListSelect(FXObject *obj, FXSelector sel, void *ptr)
{
   TRACE_L1("OutputWindow::onListSelect called ptr: " << (!ptr ? 0 : (unsigned int) ptr));

   if (!ptr && !(unsigned int) ptr)
   {
      TRACE_L1("OutputWindow::onListSelect first line selected (invalid)");
      return 0;
   }
   
   _rcmenu->hide();
   return launchItem();
}

long OutputWindow::onListClicked(FXObject *obj, FXSelector sel, void *ptr)
{
   TRACE_L1("OutputWindow::onListClicked called ptr: " << (!ptr ? 0 : (unsigned int) ptr));
   FXEvent* event = (FXEvent*) ptr;

   _rcmenu->hide();
   if (!ptr && !(unsigned int) ptr)
   {
      TRACE_L1("OutputWindow::onListClicked double click on first line (invalid)");
      return 0;
   }

   _itemIndex = (unsigned int) ptr - 1;

   _autoScroll = false;
   return 0;
}

long OutputWindow::onListDoubleClicked(FXObject *obj, FXSelector sel, void *ptr)
{
   TRACE_L1("OutputWindow::onListDoubleClicked called");
   FXEvent* event = (FXEvent*) ptr;

   this->onListSelect(obj, sel, ptr);
   _autoScroll = false;
   return 0;
}

long OutputWindow::onVScrollChanged(FXObject *obj, FXSelector sel, void *ptr)
{
   TRACE_L1("OutputWindow::onVScrollChanged called");

   _autoScroll = false;
   _outputlist->handle(obj, sel, ptr);
   return 0;
}

long OutputWindow::onFocusIn(FXObject* sender,FXSelector sel,void* ptr)
{
   TRACE_L1("OutputWindow::onFocusIn called");
   
   _mainWindow->setOutputIndex(_index);
   return 0;
}

long OutputWindow::onKeyPress(FXObject* obj, FXSelector sel, void* ptr)
{
   TRACE_L1("OutputWindow::onKeyPress called");
   
   FXEvent *event = (FXEvent *) ptr;
   const char *key = event->text.text();
   TRACE_L1("OutputWindow::onKeyPress key pressed: " << (int) key[0]);

   _rcmenu->hide();
   switch (event->code)
   {
   case 65307: // Escape key
      this->close();
      break;

   case 65293: // Enter key
      _outputlist->toggleItem(_itemIndex + 1);
      _outputlist->makeItemVisible(_itemIndex + 1);
      launchItem();
      break;

   case 65473: // F4 key
      _outputlist->toggleItem(_itemIndex + 1);
      if (_itemIndex < _grep->getOccurences())
      {
         ++_itemIndex;
         _outputlist->toggleItem(_itemIndex + 1);
         _outputlist->makeItemVisible(_itemIndex + 1);
         launchItem();
      }
      break;
   }

   return 1;
}

long OutputWindow::launchItem()
{
   if (_itemIndex >= _grep->getOccurences())
   {
      TRACE_L1("OutputWindow::launchItem double click on last line (invalid)");
      return 0;
   }

   char linestr[256];

   std::string filename = _grep->_locations[_itemIndex].filename.native_file_string();
   std::string::iterator it = filename.begin();
   for (unsigned int i = 0; (i = filename.find("/", 0)) != std::string::npos; ++it)
      filename.replace(i, 1, "\\");

   std::string command = _editorPath + " \"" + filename + "\" /" + itoa(_grep->_locations[_itemIndex].line, linestr, 10);
   TRACE_L1("OutputWindow::launchItem: Launch " << _editorName << " with: " << command);
   thread_command_t editorThread(command);
   boost::thread thrd(editorThread);

   return 0;
}

long OutputWindow::onListRightClicked(FXObject *obj, FXSelector sel, void *ptr)
{
   TRACE_L1("OutputWindow::onListRightClicked called");

   FXEvent* event = (FXEvent*) ptr;

   /*unsigned int index = _outputlist->getItemAt(event->win_x, event->win_y);
   onListClicked(obj, sel, (void *) index);*/
   _outputlist->onLeftBtnPress(obj, NULL, ptr);
   _outputlist->onLeftBtnRelease(obj, NULL, ptr);

   _rcmenu->create();
   _rcmenu->position(event->rootclick_x, event->rootclick_y, _rcmenu->getWidth(), _rcmenu->getHeight());
   _rcmenu->show();

#ifdef WIN32
   if (_onTop)
      SetWindowPos((HWND) _rcmenu->id(), HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOOWNERZORDER);
   else
      SetWindowPos((HWND) _rcmenu->id(), HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOOWNERZORDER);
#endif // !WIN32

   return 0;
}

long OutputWindow::onOpenContainingFolder(FXObject *obj, FXSelector sel, void *ptr)
{
   TRACE_L1("OutputWindow::onOpenContainingFolder called on item number " << _itemIndex);
   
   if (_itemIndex >= _grep->getOccurences())
   {
      TRACE_L1("OutputWindow::onOpenContainingFolder double click on last line (invalid)");
      return 0;
   }

   boost::filesystem:: path p = _grep->_locations[_itemIndex].filename;
   std::string cfolder = p.branch_path().native_directory_string();
   TRACE_L1("OutputWindow::onOpenContainingFolder open containing folder : " << cfolder);
   _rcmenu->hide();

   std::string command = std::string("start /b explorer /select,") + _grep->_locations[_itemIndex].filename.native_file_string();
   thread_command_t editorThread(command);
   boost::thread thrd(editorThread);
   
   return 0;
}

long OutputWindow::onOpenResultAsText(FXObject *obj, FXSelector sel, void *ptr)
{
   TRACE_L1("OutputWindow::onOpenResultAsText");
   
   std::ofstream output_file((_wgrepPath + "\\output.txt").c_str(), std::ios_base::out | std::ios_base::trunc);

   for (unsigned int i = 0; i < _outputlist->getNumItems(); ++i)
      output_file << _outputlist->getItemText(i).text();

   output_file.close();
 
   std::string command = _editorPath + " " + _wgrepPath + "\\output.txt";
   TRACE_L1("OutputWindow::onOpenResultAsText: Launch " << _editorName << " with: " << command);
   thread_command_t editorThread(command);
   boost::thread thrd(editorThread);

   return 0;
}

long OutputWindow::onTop(FXObject *obj, FXSelector sel, void *ptr)
{
   TRACE_L1("OutputWindow::onTop");
   
#ifdef WIN32
   if (_onTop)
   {
      _onTop = false;
      SetWindowPos((HWND) id(), HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOOWNERZORDER);
   }
   else
   {
      _onTop = true;
      SetWindowPos((HWND) id(), HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOOWNERZORDER);
   }

   return 0;
#endif // !WIN32
}
