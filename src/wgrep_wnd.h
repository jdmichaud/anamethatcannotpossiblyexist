//  Copyright Jean-Daniel Michaud 2007. Use, modification and
//  distribution is subject to the Boost Software License, Version
//  0.1. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#ifndef WGREP_WINDOWS_H_
#define WGREP_WINDOWS_H_

#include "fx.h"
#include "grep.hpp"
#include "wgrep_app.h"
#include "output_wnd.h"

#define REGEXPS_FOLDER   "Software\\wgrep\\regexps"
#define FILTERS_FOLDER   "Software\\wgrep\\filters"
#define FOLDERS_FOLDER   "Software\\wgrep\\folders"

extern OutputWindow  *outputWindow[];
extern bool          busyOutput[];

class GrepWindow : public FXMainWindow 
{
   // Macro for class hierarchy declarations
   FXDECLARE(GrepWindow)

   struct thread_grep_t
   {
      thread_grep_t(Grep *grep, unsigned short index) : _grep(grep), _index(index) { }
      void operator()()
      {
         TRACE_L1("GrepWindow::onFind grep started on output window index " << _index);

         unsigned int occurences = _grep->grep();
		 if (!_grep->_stop) // If output_window terminating, no need to add anything in it
		 {
		    char  occstr[256];
		    OutputWindow::callback(std::string(itoa(occurences, occstr, 10)) + " occurrence(s) have been found.", (void *) _index);
            outputWindow[_index]->setExplicitTitle("");

            busyOutput[_index] = false;

            TRACE_L1("GrepWindow::onFind grep done on output window index " << _index);
		 }
         TRACE_L2("grep: notifying grep stop");
		 _grep->_stop = true;
         _grep->_thread_stopped.notify_all();
      }
      
      Grep           *_grep;
      unsigned short _index;
   };

public:
   GrepWindow(FXApp* a, char *startingfolder = NULL);
   ~GrepWindow() 
   { 
      onExit();
      getApp()->exit(); 
   }

   virtual void create();
   enum
   {
      ID_FIND = FXMainWindow::ID_LAST,
      ID_CANCEL,
      ID_BROWSE,
      ID_ADVANCED,
      ID_KEYPRESS,
      ID_SEARCHFILENAME_CHECK,
      ID_EXCLUDE_CHECK
   };

   long onExit(void);
   long onFind(FXObject*, FXSelector, void*);
   long onCancel(FXObject*, FXSelector, void*);
   long onBrowse(FXObject*, FXSelector, void*);
   long onAdvanced(FXObject*, FXSelector, void*);
   long onKeyPress(FXObject*, FXSelector, void*);
   void setOutputIndex(unsigned short index);
   long onCheck(FXObject* obj, FXSelector sel, void* ptr);
   void newExternalInstance(const std::string &command);
   void getFilter(const std::string &str, std::vector<std::string> &v);

protected:
  GrepWindow() {} //default constructor

  FXComboBox        *regexp; 
  FXComboBox        *filter;
  FXComboBox        *directory;
  FXComboBox        *exclude_combo;
  FXButton          *find;
  FXButton          *cancel;
  FXButton          *browse;
  FXButton          *advanced;

  FXCheckButton     *matchword;
  FXCheckButton     *matchcase;
  FXCheckButton     *regexpenabled;
  FXCheckButton     *subfolders;
  FXCheckButton     *outputpane2;
  FXCheckButton     *searchfilename;
  FXCheckButton     *exclude;

  FXFont              *font;                    // Text window font
  FXString             searchpath;              // To search for files

  FXDirDialog       *_dirDialog;

  std::string       _startingfolder;
  unsigned short    _currentOutputIndex;
  bool              _advanced;

private:
  GrepApp* getApp() const { return (GrepApp*) FXMainWindow::getApp(); }


};


#endif /*  ! WGREP_WINDOWS_H_ */
