/*  This file is part of Jellyfish.

    Jellyfish is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Jellyfish is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Jellyfish.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef __JELLYFISH_FILE_HEADER_HPP__
#define __JELLYFISH_FILE_HEADER_HPP__

#include <stdlib.h>
#include <unistd.h>
#include <sys/utsname.h>

#include <string>
#include <vector>
#include <iostream>
#include <sstream>

#include <json/json.h>

namespace jellyfish {
/// Generic file header. It contains by default the hostname, the
/// current time, the current working directory and the path to the
/// executable. It is not portable for now (Linux/GNU specific).
class generic_file_header {
  static const int MAX_HEADER_DIGITS = 9;
  Json::Value root_;

  struct buffer {
    char* data;
    buffer(size_t size) : data(new char[size]) { }
    ~buffer() { delete [] data; }
  };

  static void chomp(std::string& s) {
    size_t found  = s.find_last_not_of(" \t\f\v\n\r");
    if (found != std::string::npos)
      s.erase(found+1);
    else
      s.clear();
  }

public:
  explicit generic_file_header(int alignment = 8)
  {
    root_["alignment"] = alignment;
  }

  bool operator==(const generic_file_header& rhs) const { return root_ == rhs.root_; }
  bool operator!=(const generic_file_header& rhs) const { return root_ != rhs.root_; }

  /// Write the header to an output stream. The format will be: the
  /// length written in text and decimal, followed by the header in
  /// terse JSON format, followed by some padding to align according
  /// to the `alignment_` member.
  void write(std::ostream& os) const {
    Json::FastWriter writer;
    std::string      header = writer.write(root_);
    chomp(header);

    std::ostringstream size_stream;
    size_stream << header.size();
    os << size_stream.str() << header;

    int align = alignment();
    if(align > 0) {
      int padding = ((size_t)size_stream.tellp() + header.size()) % align;
      if(padding) {
        char pad[align - padding];
        memset(pad, '\0', align - padding);
        os.write(pad, align - padding);
      }
    }
  }

  /// Read an input stream to search for a header. If one is found,
  /// true is returned. In that case, the position in the input stream points after the header and padding.
  ///
  /// If false is returned, the parsing failed. The
  /// position in the input stream may have changed and the keys
  /// present in this header may be anything.
  bool read(std::istream& is) {
    std::string len;
    int i;
    for(i = 0; i < MAX_HEADER_DIGITS && isdigit(is.peek()); ++i)
      len += is.get();
    if(i == MAX_HEADER_DIGITS || is.peek() != '{')
      return false;
    unsigned long hlen = atol(len.c_str());
    if(hlen < 2)
      return false;

    buffer hbuf(hlen);
    is.read(hbuf.data, hlen);
    if(!is.good())
      return false;

    Json::Reader reader;
    if(!reader.parse(hbuf.data, hbuf.data + is.gcount(), root_, false))
      return false;

    int align = alignment();
    if(align == 0)
      return true;
    int padding = (len.size() + is.gcount()) % align;
    if(padding == 0)
      return true;
    is.ignore(align - padding);
    return true;
  }

  const Json::Value root() const { return root_; }

  void fill_standard() {
    root_["hostname"] = get_hostname();
    root_["pwd"]      = get_pwd();
    root_["time"]     = get_localtime();
    root_["exe_path"] = get_exe_path();
  }

  std::string operator[](const std::string& key) { return root_.get(key, "").asString(); }
  std::string operator[](const char* key) { return root_.get(key, "").asString(); }
  int alignment() const { return std::max(0, root_.get("alignment", 0).asInt()); }

  std::vector<std::string> cmdline() const {
    std::vector<std::string> res;
    for(int i = 0; i < root_["cmdline"].size(); ++i)
      res.push_back(root_["cmdline"][i].asString());
    return res;
  }


  void set_cmdline(int argc, char* argv[]) {
    root_["cmdline"].clear();
    for(int i = 0; i < argc; i++)
      root_["cmdline"].append(argv[i]);
  }

protected:
  std::string get_hostname() const {
    struct utsname buf;
    if(uname(&buf) == -1)
      return "";
    return buf.nodename;
  }

  std::string get_pwd() const {
#ifdef PATH_MAX
    size_t len = PATH_MAX;
#else
    size_t len = 1024;
#endif
    char path[len + 1];

    if(!getcwd(path, len + 1))
      path[0] = '\0';
    return path;
  }

  std::string get_localtime() const {
    time_t t = time(0);
    std::string res(ctime(&t));
    chomp(res);
    return res;
  }

  std::string get_exe_path() const {
#ifdef PATH_MAX
    size_t len = PATH_MAX;
#else
    size_t len = 1024;
#endif

    char path[len + 1];
    ssize_t l = readlink("/proc/self/exe", path, len + 1);
    if(l == -1)
      return "";
    return std::string(path, l);
  }
};

  std::ostream& operator<<(std::ostream& os, const generic_file_header& h) {
    Json::StyledWriter w;
    return os << w.write(h.root());
  }

}

#endif /* __JELLYFISH_FILE_HEADER_HPP__ */
