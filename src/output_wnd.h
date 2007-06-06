//  Copyright Jean-Daniel Michaud 2007. Use, modification and
//  distribution is subject to the Boost Software License, Version
//  0.1. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#ifndef _OUTPUT_WND_H_
#define _OUTPUT_WND_H_

#include <string>
#include <boost/thread/thread.hpp>
#include <boost/thread/condition.hpp>
#include "fx.h"
#include "grep.hpp"
#include "wgrep_app.h"



class OutputWindow;
extern OutputWindow  *outputWindow[];
extern unsigned short nextFreeSlot;
extern boost::mutex	 callbackMutex;
extern bool          busyOutput[];

class GrepWindow;

class OutputWindow : public FXMainWindow 
{
   // Macro for class hierarchy declarations
   FXDECLARE(OutputWindow)

   struct thread_command_t
   {
      thread_command_t(const std::string& command) : _command(command) { }
      void operator()()
      {
         system(_command.c_str());
      }
      
      std::string _command;
   };

public:
   static unsigned short factory(FXApp *app, GrepWindow *mainWindow)
   {
      unsigned short slotUsed = nextFreeSlot++ % 256;
      outputWindow[slotUsed] = new OutputWindow(app, mainWindow, slotUsed);
      return slotUsed;
   }
   
   OutputWindow(FXApp* a, GrepWindow *mainWindow, unsigned short index);

   ~OutputWindow() 
   {
      delete _rcmenu;
      delete _outputlist;
      delete font;
      delete _grep;
      outputWindow[_index] = NULL; 
      //nextFreeSlot = _index;
   }

   virtual void create();
   enum
   {
      ID_NEW = FXMainWindow::ID_LAST,
      ID_CLOSE,
      ID_LISTSELECT,
      ID_OPEN_CONTAINING,
      ID_OPEN_VISUAL,
      ID_OPEN_AS_TEXT,
      ID_TOGGLE_ONTOP
   };

   void appendText(const std::string& text);
   long onCmdClose(FXObject*, FXSelector, void*);
   long onListSelect(FXObject*, FXSelector, void*);
   long onListClicked(FXObject *obj, FXSelector sel, void *ptr);
   long onListDoubleClicked(FXObject *obj, FXSelector sel, void *ptr);
   long onVScrollChanged(FXObject *obj, FXSelector sel, void *ptr);
   void prepare(const std::string &pattern, const std::string &folder);
   void setGrep(Grep *grep) { _grep = grep; }
   void setThread(boost::thread *thrd) { _thread = thrd; }
   void setIndex(unsigned short index) { _wndIndex = index; }
   long onFocusIn(FXObject* sender,FXSelector sel,void* ptr);
   long onKeyPress(FXObject* obj, FXSelector sel, void* ptr);
   long launchItem();
   long onListRightClicked(FXObject *obj, FXSelector sel, void *ptr);
   long onOpenContainingFolder(FXObject *obj, FXSelector sel, void *ptr);
   void setExplicitTitle(const std::string& suffix);
   long onOpenResultAsText(FXObject *obj, FXSelector sel, void *ptr);
   long onTop(FXObject *obj, FXSelector sel, void *ptr);

   static bool callback(const std::string& text, void *ptr) 
   { 
		// boost::mutex::scoped_lock scoped_lock(callbackMutex); // Deadlock mutex (?)

      unsigned short i = ((ptr != NULL) ? (unsigned short) ptr : 0);

      TRACE_L3("GrepWindow::callback called with: " << text << ", index: " << (unsigned short) ptr);
      outputWindow[i]->appendText(text); 
      return true;
   }

   static bool onfile_callback(const std::string& text, void *ptr) 
   { 
		// boost::mutex::scoped_lock scoped_lock(callbackMutex); // Deadlock mutex (?)

      unsigned short i = ((ptr != NULL) ? (unsigned short) ptr : 0);

      outputWindow[i]->setExplicitTitle(text); 
      return true;
   }

protected:
   OutputWindow() {} //default constructor

   FXText                  *_output;
   FXList                  *_outputlist;
   FXMenuPane              *_rcmenu;
   FXFont                  *font;                    // Text window font
   Grep                    *_grep;

   unsigned short          _index;
   unsigned int            _itemIndex;
   std::string             _editorPath;
   std::string             _editorName;
   std::string             _wgrepPath;
   std::string             _pattern;
   std::string             _folder;
   bool                    _autoScroll;
   GrepWindow              *_mainWindow;
   bool                    _onTop;
   boost::thread           *_thread;
   unsigned short          _wndIndex;

private:
  GrepApp* getApp() const { return (GrepApp*) FXMainWindow::getApp(); }
};

#endif // ! _OUTPUT_WND_H_
