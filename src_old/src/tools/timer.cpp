#include "tools/timer.hpp"

namespace track_select {
  namespace tools {
    Timer::Timer(bool run) {
      if (run) Reset();
    }

    void Timer::Reset() {
      _start = high_resolution_clock::now();
    }

    Timer::milliseconds Timer::Elapsed() const  {
      return std::chrono::
        duration_cast<milliseconds>(high_resolution_clock::now() - _start);
    }

  }
}
