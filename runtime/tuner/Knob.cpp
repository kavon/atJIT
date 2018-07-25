
#include <tuner/Knob.h>
#include <tuner/param.h>

namespace tuner {

template <typename S>
bool operator== (const ScalarRange<S>& A, const ScalarRange<S>& B) {
  return (
    A.max() == B.max() &&
    A.min() == B.min() &&
    A.getDefault() == B.getDefault()
  );
}

} // end namespace


namespace tuned_param {
  bool IntRange::operator== (IntRange const& Other) {
    return (
      static_cast<tuner::ScalarRange<int> const&>(*this)
      == static_cast<tuner::ScalarRange<int> const&>(Other)
    );
  }
}
