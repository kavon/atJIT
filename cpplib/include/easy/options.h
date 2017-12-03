#ifndef OPTIONS
#define OPTIONS

#include <easy/runtime/Context.h>

#define EASY_NEW_OPTION_STRUCT(Name) \
  struct Name; \
  template<> struct is_option<Name> { \
    static constexpr bool value = true; }; \
  struct Name

#define EASY_HANDLE_OPTION_STRUCT(Name, Ctx) \
  void handle(easy::Context &Ctx) const


namespace easy {
namespace options{

  template<class T>
  struct is_option {
    static constexpr bool value = false;
  };

  EASY_NEW_OPTION_STRUCT(opt_level)
    : public std::pair<unsigned, unsigned> {

    opt_level(unsigned OptLevel, unsigned OptSize)
               : std::pair<unsigned, unsigned>(OptLevel,OptSize) {}

    EASY_HANDLE_OPTION_STRUCT(opt_level, C) {
      C.setOptLevel(first, second);
    }
  };
}
}

#endif // OPTIONS
