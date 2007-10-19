//  Copyright Jean-Daniel Michaud 2007. Use, modification and
//  distribution is subject to the Boost Software License, Version
//  0.1. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#pragma warning(disable:4786)

#ifdef WIN32
# ifdef _DEBUG
#  define _CRTDBG_MAP_ALLOC
#  include <stdlib.h>
#  include <crtdbg.h>
# endif

# include <windows.h>
#endif // WIN32

#include <vector>
#include <string>
#include "wgrep_app.h"
#include "icon.h"

extern const unsigned char icon[];

FXIMPLEMENT(GrepApp, FXApp, 0, NULL)

// Make some windows
GrepApp::GrepApp(const FXString& name) : FXApp(name, FXString::null)
{
/*
   FXIconSource iconsource(this);
   openIcon    = iconsource.loadScaledIconFile("./Icons/editcopy.gif", 16);
   saveIcon    = iconsource.loadScaledIconFile("./Icons/filesave.gif", 16);
   saveAsIcon  = iconsource.loadScaledIconFile("./Icons/filesaveas.gif", 16);
   closeIcon   = iconsource.loadScaledIconFile("./Icons/close.gif", 16);
   helpIcon   = iconsource.loadScaledIconFile("./Icons/help.gif", 16);
*/
   grepIcon = new FXGIFIcon(this, icon);
   grepIcon->create();
}

GrepApp::~GrepApp()
{
/*
   delete openIcon;
   delete saveIcon;
   delete saveAsIcon;
   delete closeIcon;
   delete helpIcon;
*/
   delete[] grepIcon;
}

// Initialize application
void GrepApp::init(int& argc, char** argv, bool connect)
{
  // After init, the registry has been loaded
  FXApp::init(argc, argv, connect);
}


// Exit application
void GrepApp::exit(FXint code)
{
  // Writes registry, and quits
  FXApp::exit(code);
}

#ifdef WIN32
std::string GrepApp::getKeyValue(const std::string &root, const std::string &path, const std::string &key)
{
   char lszValue[256];
   HKEY hKey;
   LONG returnStatus;
   DWORD dwType = REG_SZ;
   DWORD dwSize = 256;
   // TODO - adapt first parameter with root using a switch
   returnStatus = RegOpenKeyEx(HKEY_CURRENT_USER, path.c_str(), 0L,  KEY_ALL_ACCESS, &hKey);
   if (returnStatus == ERROR_SUCCESS)
   {
      returnStatus = RegQueryValueEx(hKey, key.c_str(), NULL, &dwType, (LPBYTE) &lszValue, &dwSize);
      if (returnStatus == ERROR_SUCCESS)
      {
         RegCloseKey(hKey);
         return lszValue;
      }
   }
 
   RegCloseKey(hKey);
   return "";
}

bool GrepApp::addKeyValue(const std::string &root, const std::string &path, const std::string& key, const std::string& value)
{
   HKEY hKey;

   if(RegOpenKeyEx(HKEY_CURRENT_USER,
                   TEXT(path.c_str()),
                   0,
                   KEY_ALL_ACCESS,
                   &hKey) != ERROR_SUCCESS)   
      return 0;

   unsigned long l = value.size();
   /*
   if (RegQueryValueEx(hKey,
                       key.c_str(),
                       NULL,
                       NULL,
                       (LPBYTE) value.c_str(),
                       &l) == ERROR_SUCCESS)
      return 0;
   */
   if (RegSetValueEx(hKey,          // subkey handle 
                     key.c_str(),   // value name 
                     0,             // must be zero 
                     REG_SZ,        // value type 
   (unsigned char *) value.c_str(), // pointer to value data 
                     value.size())) // data size
   {
      RegCloseKey(hKey); 
      return false;
   }
   else
   {
      RegCloseKey(hKey); 
      return true;
   }

}

unsigned int GrepApp::listKeys(const std::string &root, const std::string &path, std::vector<std::string> &keys)
{
   unsigned int count = 0;

   DWORD    retCode; 

   HKEY hKey;

   keys.clear();
   keys.resize(MAX_NUMBER_ITEMS_STORED);

   if(RegOpenKeyEx(HKEY_CURRENT_USER,
                   TEXT(path.c_str()),
                   0,
                   KEY_READ,
                   &hKey) != ERROR_SUCCESS)   
      return 0;

   TCHAR  achName[MAX_VALUE_NAME]; 
   DWORD cchName = MAX_VALUE_NAME;
   unsigned char  *achValue = new unsigned char[MAX_VALUE_NAME]; 
   DWORD cchValue = MAX_VALUE_NAME;

   retCode = ERROR_SUCCESS;
   while (retCode == ERROR_SUCCESS)
   { 
      cchName = MAX_VALUE_NAME;
      cchValue = MAX_VALUE_NAME; 
      achValue[0] = '\0'; 
      achName[0] = '\0'; 
      retCode = RegEnumValue(hKey, count, 
         achName, 
         &cchName, 
         NULL, 
         NULL,
         achValue,
         &cchValue);
      
      if (retCode == ERROR_SUCCESS ) 
      { 
         unsigned short index = atoi(achName);
         if (index >= 0 && index < MAX_NUMBER_ITEMS_STORED)
            keys[index] = (char *) achValue;
         ++count;
      }
   }
 
   RegCloseKey(hKey);
   delete[] achValue;
   return count;
}

#endif // WIN32
