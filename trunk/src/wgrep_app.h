//  Copyright Jean-Daniel Michaud 2007. Use, modification and
//  distribution is subject to the Boost Software License, Version
//  0.1. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#ifndef WGREP_APP_H
#define WGREP_APP_H

#include <vector>
#include <string>
#include "fx.h"

// Version
#define VERSION_MAJOR            0
#define VERSION_MINOR            2
#define VERSION_PATCH            0

#define MAX_KEY_LENGTH           256
#define MAX_VALUE_NAME           256
#define MAX_NUMBER_ITEMS_STORED  20

class GrepApp : public FXApp {
   FXDECLARE(GrepApp)

protected:

private:
   GrepApp() {}

public:
   // Icons
   FXIcon         *openIcon;                    // Small application icon
   FXIcon         *saveIcon;                    // Small application icon
   FXIcon         *saveAsIcon;                  // Small application icon
   FXIcon         *closeIcon;                  // Small application icon
   FXIcon         *helpIcon;                  // Small application icon
   FXIcon         *grepIcon;                  // Application icon

public:
  // Construct application object
  GrepApp(const FXString& name);

  // Initialize application
  virtual void init(int& argc, char** argv, bool connect = TRUE);

  // Exit application
  virtual void exit(FXint code = 0);

  // Delete application object
  virtual ~GrepApp();


#ifdef WIN32
   static std::string   getKeyValue(const std::string &root, const std::string &path, const std::string &key);
   static unsigned int  listKeys(const std::string &root, const std::string &path, std::vector<std::string> &keys);
   static bool          addKeyValue(const std::string &root, const std::string &path, const std::string& key, const std::string& value);
#endif // WIN32

};

#endif /* ! WGREP_APP_H */
