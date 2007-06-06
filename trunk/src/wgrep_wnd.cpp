//  Copyright Jean-Daniel Michaud 2007. Use, modification and
//  distribution is subject to the Boost Software License, Version
//  0.1. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#include <boost/thread.hpp>
#include <boost/tokenizer.hpp>
#include <boost/regex.hpp>
#include <vector>
#include <string>
#include "grep.hpp"
#include "log_server.h"
#include "wgrep_wnd.h"
#include "output_wnd.h"

extern bool busyOutput[];

// Map
FXDEFMAP(GrepWindow) GrepWindowMap[]={
  FXMAPFUNC(SEL_COMMAND,           GrepWindow::ID_FIND,                    GrepWindow::onFind),
  FXMAPFUNC(SEL_COMMAND,           GrepWindow::ID_CANCEL,                  GrepWindow::onCancel),
  FXMAPFUNC(SEL_COMMAND,           GrepWindow::ID_BROWSE,                  GrepWindow::onBrowse),
  FXMAPFUNC(SEL_COMMAND,           GrepWindow::ID_ADVANCED,                GrepWindow::onAdvanced),
  FXMAPFUNC(SEL_COMMAND,           GrepWindow::ID_CLOSE,                   GrepWindow::onCancel),
  FXMAPFUNC(SEL_COMMAND,           GrepWindow::ID_SEARCHFILENAME_CHECK,    GrepWindow::onCheck),
  FXMAPFUNC(SEL_KEYPRESS,          NULL,                                   GrepWindow::onKeyPress)
};

// Object implementation
FXIMPLEMENT(GrepWindow, FXMainWindow, GrepWindowMap, ARRAYNUMBER(GrepWindowMap))

GrepWindow::GrepWindow(FXApp *a, char *startingfolder) 
   : FXMainWindow(a, "Find in files (jean-daniel.michaud@laposte.net)", NULL, NULL, DECOR_TITLE | DECOR_CLOSE | DECOR_MINIMIZE | DECOR_RESIZE, 445, 438, 390, 148), 
   _startingfolder(((startingfolder) ? startingfolder : "")), _currentOutputIndex(0), _advanced(0)
{
   TRACE_L1("GrepWindow::GrepWindow constructor");

   new FXLabel(this, "Find what:", NULL, LABEL_NORMAL | LAYOUT_EXPLICIT, 10, 13, 60, 20);

   regexp      = new FXComboBox(this, 26, NULL, 0, COMBOBOX_NORMAL | LAYOUT_EXPLICIT, 100, 11, 173, 20);
   find        = new FXButton(this, "Find", NULL, this, ID_FIND, BUTTON_NORMAL | LAYOUT_EXPLICIT | BUTTON_DEFAULT, 305, 10, 75, 23);
   new FXLabel(this, "In files/file types:", NULL, LABEL_NORMAL | LAYOUT_EXPLICIT, 10, 38, 90, 20);
   filter      = new FXComboBox(this, 26, NULL, 0, COMBOBOX_NORMAL | LAYOUT_EXPLICIT, 100, 37, 173, 20);
   cancel      = new FXButton(this, "Cancel", NULL, this, ID_CANCEL, BUTTON_NORMAL | LAYOUT_EXPLICIT, 305, 38, 75, 23);
   new FXLabel(this, "In folder:", NULL, LABEL_NORMAL | LAYOUT_EXPLICIT, 10, 63, 51, 20);
   directory   = new FXComboBox(this, 26, NULL, 0, COMBOBOX_NORMAL | LAYOUT_EXPLICIT, 100, 63, 173, 20);
   
   browse      = new FXButton(this, "...", NULL, this, ID_BROWSE, BUTTON_NORMAL | LAYOUT_EXPLICIT, 280, 63, 15, 20);
   advanced    = new FXButton(this, "Advanced>>", NULL, this, ID_ADVANCED, BUTTON_NORMAL | LAYOUT_EXPLICIT, 305, 66, 75, 23);

   matchword   = new FXCheckButton(this, "Match whole word only", NULL, 0, CHECKBUTTON_NORMAL | LAYOUT_EXPLICIT | JUSTIFY_LEFT, 10, 91, 130, 20);
   matchcase   = new FXCheckButton(this, "Match case", NULL, 0, CHECKBUTTON_NORMAL | LAYOUT_EXPLICIT | JUSTIFY_LEFT, 10, 110, 73, 20);
   regexpenabled = new FXCheckButton(this, "Regular expression", NULL, 0, CHECKBUTTON_NORMAL | LAYOUT_EXPLICIT | JUSTIFY_LEFT | BUTTON_AUTOGRAY, 10, 128, 110, 20);
   subfolders  = new FXCheckButton(this, "Look in subfolders", NULL, 0, CHECKBUTTON_NORMAL | LAYOUT_EXPLICIT | JUSTIFY_LEFT, 168, 91, 125, 20);
   outputpane2 = new FXCheckButton(this, "Output to new pane", NULL, 0, CHECKBUTTON_NORMAL | LAYOUT_EXPLICIT | JUSTIFY_LEFT, 168, 110, 125, 20);
   searchfilename = new FXCheckButton(this, "Search file name", this, ID_SEARCHFILENAME_CHECK, CHECKBUTTON_NORMAL | LAYOUT_EXPLICIT | JUSTIFY_LEFT, 168, 128, 125, 20);

   _dirDialog = new FXDirDialog(this, "Browse ...");
   
   regexpenabled->setCheck(true);
   subfolders->setCheck(true);

   font = new FXFont(getApp(), "courier new", 8);
   setIcon(getApp()->grepIcon);

#ifdef WIN32
   // Load registry content into combo boxed
   std::vector<std::string>            content;
   unsigned int                        i;

   GrepApp::listKeys("HKEY_LOCAL_MACHINE", REGEXPS_FOLDER, content);
   for (i = 0; i < content.size(); ++i)
      if ((content[i] != "") && (regexp->findItem(content[i].c_str()) < 0)) regexp->appendItem(content[i].c_str());
   content.clear();

   GrepApp::listKeys("HKEY_LOCAL_MACHINE", FILTERS_FOLDER, content);
   for (i = 0; i < content.size(); ++i)
      if ((content[i] != "") && (filter->findItem(content[i].c_str()) < 0)) filter->appendItem(content[i].c_str());
   content.clear();

   GrepApp::listKeys("HKEY_LOCAL_MACHINE", FOLDERS_FOLDER, content);
   for (i = 0; i < content.size(); ++i)
      if ((content[i] != "") && (directory->findItem(content[i].c_str()) < 0)) directory->appendItem(content[i].c_str());
   content.clear();
#endif // !WIN32

   if (_startingfolder != "")
      directory->setText(_startingfolder.c_str());

   regexp->setFocus();
   regexp->setNumVisible(4);
   filter->setNumVisible(4);
   directory->setNumVisible(4);
}
  
void GrepWindow::create()
{
   // Create the windows
   FXMainWindow::create();
   // Make the main window appear
   show();
}

long GrepWindow::onFind(FXObject*, FXSelector, void*)
{
   TRACE_L1("GrepWindow::onFind called");

   FXString                         temp3    = filter->getText();
   std::string                      filtr(temp3.text());
   if (filtr == "") // If filter empty, set it to *.*
   {
      filtr = "*.*";
      filter->setText("*.*");
   }

   typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
   std::string                  str = filtr;
   boost::char_separator<char>  sep(";,");
   std::vector<std::string>     filters;
   tokenizer                    tokens(str, sep);
   for (tokenizer::iterator     tok_iter = tokens.begin(); tok_iter != tokens.end(); ++tok_iter)
     filters.push_back(*tok_iter);

   FXString    temp  = regexp->getText();
   const char  *re   = temp.text();
   FXString    temp2 = directory->getText().text();
   const char  *dir  = temp2.text();
   
   if (!strcmp(re, ""))
   {
      TRACE_L1("GrepWindow::onFind regexp empty");
      return 0;
   }

   if (!strcmp(dir, ""))
   {
      TRACE_L1("GrepWindow::onFind search folder empty");
      return 0;
   }

   TRACE_L1("GrepWindow::onFind calling grep " << re << " " << dir);
   
   options_t options;
   options.matchcase    = matchcase->getCheck();
   options.wholeword    = matchword->getCheck();
   options.subfolders   = subfolders->getCheck();
   options.regexp       = regexpenabled->getCheck();
   options.searchfilename = searchfilename->getCheck();

   if (outputpane2->getCheck() || !outputWindow[_currentOutputIndex])
   {
      _currentOutputIndex = OutputWindow::factory(getApp(), this);
      TRACE_L1("GrepWindow::onFind create new window at index: " << _currentOutputIndex);
   }

   if (busyOutput[_currentOutputIndex])
   {
      TRACE_L1("GrepWindow::onFind output window number " << _currentOutputIndex << " is currently busy. onFind canceled.");
      return 0;
   }

   outputWindow[_currentOutputIndex]->create();
   
   outputWindow[_currentOutputIndex]->prepare(std::string(re), std::string(dir));
   OutputWindow::callback(std::string("Searching for '") + re + "'...", (void *) _currentOutputIndex);
   
   Grep              *grep = NULL;
   try { grep = new Grep(std::string(re), std::string(dir), filters, options, OutputWindow::callback, OutputWindow::onfile_callback, (void *) _currentOutputIndex); }
   catch (boost::regex_error e)
   {
      TRACE_L1("GrepWindow::onFind: " << e.what());
      OutputWindow::callback(std::string("Error: ") + e.what(), (void *) _currentOutputIndex);
      return 1;
   }

   outputWindow[_currentOutputIndex]->setGrep(grep);
   thread_grep_t grepThread(grep, _currentOutputIndex);
   boost::thread thrd(grepThread);
   outputWindow[_currentOutputIndex]->setThread(&thrd);
   outputWindow[_currentOutputIndex]->setIndex(_currentOutputIndex);
   busyOutput[_currentOutputIndex] = true;

   if (regexp->findItem(re) < 0)             regexp->prependItem(re);
   if (filter->findItem(filtr.c_str()) < 0)  filter->prependItem(filtr.c_str());
   if (directory->findItem(dir) < 0)         directory->prependItem(dir);

   TRACE_L1("GrepWindow::onFind grep running");
   return 0;
}

long GrepWindow::onCancel(FXObject*, FXSelector, void*)
{
   TRACE_L1("GrepWindow::onCancel called");

   this->~GrepWindow();
   return 0;
}

long GrepWindow::onBrowse(FXObject*, FXSelector, void*)
{
   TRACE_L1("GrepWindow::onBrowse called");

   directory->setText(_dirDialog->getOpenDirectory(this, "Browse ...", directory->getText()));
   return 0;
}

long GrepWindow::onAdvanced(FXObject*, FXSelector, void*)
{
  TRACE_L1("GrepWindow::onAdvanced called (_advanced == " << (_advanced ? "true" : "false"));
  
  if (!_advanced)
  {
    _advanced = true;
    advanced->setText("Advanced<<");

  }
  else
  {
    _advanced = false;
    advanced->setText("Advanced>>");
  }

  return 0;
}

long GrepWindow::onKeyPress(FXObject* obj, FXSelector sel, void* ptr)
{
   TRACE_L1("GrepWindow::onKeyPress called");

   FXEvent *event = (FXEvent *) ptr;
   const char *key = event->text.text();
   TRACE_L1("GrepWindow::onKeyPress key pressed: " << (int) key[0]);
   switch (key[0])
   {
   case 27: // Escape key
      this->~GrepWindow();
      return 0;
      break;
   case 13: // Enter key
      onFind(obj, sel, ptr);
      break;
   }

   switch (event->code)
   {
   case 65473:

      break;
   }

   FXMainWindow::handle(obj, sel, ptr);
   return 0;
}

void GrepWindow::setOutputIndex(unsigned short index) 
{ 
   TRACE_L1("GrepWindow::setOutputIndex index set to: " << index);
   _currentOutputIndex = index; 
}

long GrepWindow::onCheck(FXObject* obj, FXSelector sel, void* ptr)
{
   TRACE_L1("GrepWindow::onCheck");

   if (obj == searchfilename)
      if ((bool) ptr == true)
         filter->disable();
      else
         filter->enable();

   return 0;
}

void GrepWindow::newExternalInstance(const std::string &command)
{
   TRACE_L1("GrepWindow::newExternalInstance command: " << command);

   boost::char_separator<char>      sep(" ");
   boost::tokenizer< boost::char_separator<char> > tokens(command, sep);
   std::vector<std::string>         argv;

   for (boost::tokenizer< boost::char_separator<char> >::iterator it = tokens.begin(); it != tokens.end(); ++it)
      argv.push_back(*it);

   if ((argv.size() >= 2) && (argv[argv.size() - 1] == "-tracelevel"))
   {
      TRACE_L1("GrepWindow::newExternalInstance: ignored");
   }
   else
   {
      directory->setText(argv[argv.size() - 1].c_str());
   }

   raise();
}

long GrepWindow::onExit(void)
{
#ifdef WIN32
   unsigned int i = 0;
   unsigned int j;
   if (strcmp(regexp->getText().text(), ""))
   {
      char  keyStr[256];
      std::string key = itoa(i, (char *) keyStr, 10);
      GrepApp::addKeyValue("HKEY_CURRENT_USER", REGEXPS_FOLDER, key, regexp->getText().text());
      ++i;      
   }
   for (j = 0; i < regexp->getNumItems() && i < MAX_NUMBER_ITEMS_STORED; ++i, ++j)
   {
      char  keyStr[256];
      std::string key = itoa(i, (char *) keyStr, 10);
      GrepApp::addKeyValue("HKEY_CURRENT_USER", REGEXPS_FOLDER, key, regexp->getItem(j = 0).text());
   }

   i = 0;
   if (strcmp(filter->getText().text(), ""))
   {
      char  keyStr[256];
      std::string key = itoa(i, (char *) keyStr, 10);
      GrepApp::addKeyValue("HKEY_CURRENT_USER", FILTERS_FOLDER, key, filter->getText().text());
      ++i;      
   }
   for (j = 0; i < filter->getNumItems() && i < MAX_NUMBER_ITEMS_STORED; ++i, ++j)
   {
      char  keyStr[256];
      std::string key = itoa(i, (char *) keyStr, 10);
      GrepApp::addKeyValue("HKEY_CURRENT_USER", FILTERS_FOLDER, key, filter->getItem(j).text());
   }

   i = 0;
   if (strcmp(directory->getText().text(), ""))
   {
      char  keyStr[256];
      std::string key = itoa(i, (char *) keyStr, 10);
      GrepApp::addKeyValue("HKEY_CURRENT_USER", FOLDERS_FOLDER, key, directory->getText().text());
      ++i;      
   }
   for (j = 0; i < directory->getNumItems() && i < MAX_NUMBER_ITEMS_STORED; ++i, ++j)
   {
      char  keyStr[256];
      std::string key = itoa(i, (char *) keyStr, 10);
      GrepApp::addKeyValue("HKEY_CURRENT_USER", FOLDERS_FOLDER, key, directory->getItem(j).text());
   }
#endif // !WIN32

   return 0;
}
