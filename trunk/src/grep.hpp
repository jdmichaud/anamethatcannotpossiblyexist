//  Copyright Jean-Daniel Michaud 2007. Use, modification and
//  distribution is subject to the Boost Software License, Version
//  0.1. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#ifndef _GREP_HXX_
#define _GREP_HXX_

#include <string>
#include <iostream>
#include <vector>
#include <boost/regex.hpp>
#include <boost/filesystem/operations.hpp> // includes boost/filesystem/path.hpp
#include <boost/filesystem/fstream.hpp>    // ditto
#include <boost/filesystem/exception.hpp>
#include "log_server.h"

typedef bool (*GrepCallback)(const std::string&, void *ptr);


struct location_t
{
   location_t(boost::filesystem::path _f, unsigned int _l) : filename(_f), line(_l) {}
   boost::filesystem::path    filename;
   unsigned int   line;
};

struct options_t
{
   options_t() : matchcase(false), subfolders(true), wholeword(false), regexp(true), searchfilename(false) {}
   options_t(bool m, bool s, bool w, bool r, bool f) 
      : matchcase(m), subfolders(s), wholeword(w), regexp(r), searchfilename(f) {}

   bool matchcase;
   bool subfolders;
   bool wholeword;
   bool regexp;
   bool searchfilename;
};

class Grep
{
public:
   Grep(const std::string              &regexp, 
        const std::string              path, 
        const std::vector<std::string> filters,
        GrepCallback                   find_callback = NULL,
        GrepCallback                   file_callback = NULL,
        void                           *ptr = NULL) : 
      _path(path), _filters(filters), _find_callback(find_callback), 
      _file_callback(file_callback), _ptr(ptr)
   {
      _regexp.assign(regexp, boost::regex_constants::icase);
      _options.matchcase   = false;
      _options.subfolders  = true;
      _options.wholeword   = false;
      _options.regexp      = true;
      _options.searchfilename = false;
   }

   Grep(std::string                    regexp,
        const std::string              &path,
        const std::vector<std::string> &filters, 
        options_t                      options,
        GrepCallback                   find_callback = NULL,
        GrepCallback                   file_callback = NULL,
        void                           *ptr = NULL) : 
      _path(path), _filters(filters), _options(options), 
      _find_callback(find_callback), _file_callback(file_callback), _ptr(ptr)
   {
      if (_options.wholeword)
      {
         regexp = std::string("\\<") + regexp + "\\>";
         TRACE_L1("Grep::Grep: To match whole word, regexp transformed to : " << regexp);
      }

         _regexp.assign(regexp, ((_options.matchcase) ? boost::regex_constants::normal : boost::regex_constants::icase) 
                                | boost::regex::egrep);
   }

//   unsigned int grep(bool (*find_callback) (void *obj, const std::string &output))
   unsigned int grep()
   {
      try
      {
         TRACE_L1("grep: " << _regexp << " in " << _path);
         std::vector<std::string>::iterator it = _filters.begin();
         while (it != _filters.end())
            TRACE_L1("grep: " << *it++);
         boost::filesystem::path path(_path, boost::filesystem::native);
         _occurences = 0;
         grep_folder(path);
         return _occurences;
      }
      catch (const boost::filesystem::filesystem_error &ex)
      {
         std::cout << _path << " does not exist (" << ex.what() << ")" << std::endl;
      }

      return 0;
   }

   unsigned int grep_folder(const boost::filesystem::path& directory)//,     // in this directory,
//                  const std::string & file_name, // search for this name,
//                  path & path_found)        // placing path here if found
   {
      unsigned int count = 0;

      try
      {
         TRACE_L2("Grep::grep_folder: ---> " << directory.string());
         if (!boost::filesystem::exists(directory)) return false;

         if (!boost::filesystem::is_directory(directory)) // greping a file not a folder
            return grep_file(directory, true);

         boost::filesystem::directory_iterator end_itr; // default construction yields past-the-end
         for (boost::filesystem::directory_iterator itr(directory); itr != end_itr; ++itr)
         {
            if (boost::filesystem::is_directory(*itr) && _options.subfolders)
            {
               count += grep_folder(*itr /*, file_name, path_found*/ );
            }
            else
            {
               count += grep_file(*itr);
            }
         }
      }
      catch (...)
      {
         TRACE_L1("Grep::grep_folder: ERROR " << directory.string() << " does not exist");
         std::cout << directory.string() << " does not exist" << std::endl;
      }

      TRACE_L2("Grep::grep_folder: <---" << directory.string());
      return count;
   }

   unsigned int grep_file(const boost::filesystem::path& file, bool ignore_filter = false)
   {
      unsigned int count = 0;
      std::vector<std::string>::iterator it;

      for (it = _filters.begin(); it != _filters.end(); ++it)
      {
         try
         {
            std::string filter = *it;
            size_t dotpos = filter.find_last_of('.');
            filter.replace(dotpos, 1, "\\.");
            filter = std::string(".") + filter + "$";
            
            TRACE_L3("grep_folder: " << filter << " - " << file.leaf());
            
            boost::regex filere(filter, boost::regex_constants::basic);
            if (ignore_filter || _options.searchfilename || boost::regex_search(file.leaf(), _what, filere))
            {
//               count += process_file(file.string() + "/" + file.leaf());
               count += process_file(file);
               break;
            }
         }
         catch (boost::bad_expression &be)
         {
            std::cout << "error: " << be.what() << std::endl;
         }
      }

      return count;
   }

   unsigned int process_stream(std::istream& is, const boost::filesystem::path& filename)
   {
      std::string       line;
      int               match_found = 0;
      int               linenum     = 1;
      unsigned int      count       = 0;

      if (_options.searchfilename)
      {
         bool result = boost::regex_search(filename.leaf(), _what, _regexp, (boost::regex_constants::match_flag_type)
                                          ((_options.matchcase) ? boost::regex_constants::normal : boost::regex_constants::icase));
         if(result)
         {
            if (_find_callback) _find_callback(filename.string(), _ptr);
            _locations.push_back(location_t(filename/*.string()*/, linenum));
            ++count;
            ++_occurences;
         }
      } else while (std::getline(is, line))
      { //boost::regex_constants::icase
         bool result = boost::regex_search(line, _what, _regexp, (boost::regex_constants::match_flag_type)
                                          ((_options.matchcase) ? boost::regex_constants::normal : boost::regex_constants::icase));
         if(result)
         {
            std::cout << filename.string() << ": " << line << std::endl;
            char linestr[256];

            if (_find_callback) _find_callback(filename.string() + "(" + itoa(linenum, linestr, 10) + "):" + line, _ptr);

            _locations.push_back(location_t(filename/*.string()*/, linenum));
            ++count;
            ++_occurences;
         }
         ++linenum;

         if (_options.searchfilename)
            break;
      }

      return count;
   }

   unsigned int process_file(const boost::filesystem::path& name)
   {
      TRACE_L2("Grep::process_file: processing " << name.string());
      std::ifstream is(name.string().c_str());
      if(is.bad())
      {
         TRACE_L1("Grep::process_file: ERROR: Unable to open file" << name.string());
         std::cerr << "Unable to open file " << name.string() << std::endl;
         return 0;
      }
      if (!_options.searchfilename && _file_callback) _file_callback(name.native_file_string(), _ptr);
      return process_stream(is, name);
   }

   unsigned int getOccurences() { return _occurences; }

public:
   std::vector<location_t> _locations;

private:
   boost::regex   _regexp;
   boost::smatch  _what;
   GrepCallback   _find_callback;
   GrepCallback   _file_callback;
   void           *_ptr;
   options_t      _options;

   std::string    _path;
   std::vector<std::string> _filters;
   unsigned int   _occurences;
};

#endif // ! _GREP_HXX_
