#ifndef EXCEPTIONS
#define EXCEPTIONS

#include <stdexcept>
#include <string>
#include <sstream>

namespace easy {
  struct exception
      : public std::runtime_error {
    exception(std::string const &Message, std::string const &Reason)
      : std::runtime_error(Message + Reason) {}
    virtual ~exception() = default;
  };
}

#define DefineEasyException(Exception, Message) \
  struct Exception : public easy::exception { \
    Exception() : easy::exception(Message, "") {} \
    Exception(std::string const &Reason) : easy::exception(Message, Reason) {} \
    virtual ~Exception() = default; \
  }

#endif
