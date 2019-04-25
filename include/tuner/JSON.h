#pragma once

#include <fstream>
#include <string>
#include <iomanip>

// quick and dirty JSON dumper
class JSON {
  static int depth;
public:

  static void indent(std::ostream &file) {
    for (int i = 0; i < depth; i++)
      file << "  ";
  }

  static void beginObject(std::ostream &file) {
    indent(file);
    file << "{\n";
    depth++;
  }

  static void endObject(std::ostream &file) {
    depth--;
    indent(file);
    file << "}";
  }

  static void beginArray(std::ostream &file) {
    indent(file);
    file << "[\n";
    depth++;
  }

  static void endArray(std::ostream &file) {
    depth--;
    indent(file);
    file << "]";
  }

  static void beginBind(std::ostream &file, std::string key) {
    indent(file);
    fmt(file, key);
    file << " : ";
  }

  static void comma(std::ostream &file) {
    file << ",\n";
  }

  static void fmt(std::ostream &file, std::string val) {
    file << "\"" << val << "\"";
  }

  static void fmt(std::ostream &file, const char *val) {
    file << "\"" << val << "\"";
  }

  template < typename ValTy >
  static void fmt(std::ostream &file, ValTy val) {
    file << val;
  }



  //////////////////////
  // common operations

  template < typename ValTy >
  static void output(std::ostream &file, std::string key, ValTy val, bool hasNext = true) {
    beginBind(file, key);
    fmt(file, val);
    if (hasNext)
      comma(file);
  }

};
