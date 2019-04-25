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

  // tuner kind option & correspondence.
  EASY_NEW_OPTION_STRUCT(tuner_kind) {

    tuner_kind(tuner::AutoTuner kind)
               : kind_(kind) {}

    EASY_HANDLE_OPTION_STRUCT(IGNORED, C) {
      C.setTunerKind(kind_);
    }

    private:
      tuner::AutoTuner kind_;
  };

  // the feedback module used to analyze perf
  EASY_NEW_OPTION_STRUCT(feedback_kind) {

    feedback_kind(tuner::FeedbackKind val)
               : val_(val) {}

    EASY_HANDLE_OPTION_STRUCT(IGNORED, C) {
      C.setFeedbackKind(val_);
    }

    private:
      tuner::FeedbackKind val_;
  };

  // if true, the driver will be more willing to
  // wait, i.e. block, on compilation jobs.
  EASY_NEW_OPTION_STRUCT(blocking) {

    blocking(bool val)
               : val_(val) {}

    EASY_HANDLE_OPTION_STRUCT(IGNORED, C) {
      C.setWaitForCompile(val_);
    }

    private:
      bool val_;
  };

  // option used for writing the ir to a file, useful for debugging
  EASY_NEW_OPTION_STRUCT(dump_ir) {
    dump_ir(std::string const &file)
               : file_(file), beforeFile_(file + ".before") {}

    EASY_HANDLE_OPTION_STRUCT(dump_ir, C) {
      C.setDebugFile(file_);
      C.setDebugBeforeFile(beforeFile_);
    }

    private:
    std::string file_;
    std::string beforeFile_;
  };
}
}

#endif // OPTIONS
