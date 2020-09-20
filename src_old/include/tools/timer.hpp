#ifndef __TOOLS_TIMER_H_
#define __TOOLS_TIMER_H_
#include <chrono>
#include <ostream>

namespace track_select {
  namespace tools {
    class Timer {
      typedef std::chrono::high_resolution_clock high_resolution_clock;
      typedef std::chrono::milliseconds milliseconds;
    public:
      explicit Timer(bool run = false);

      void Reset();
      milliseconds Elapsed() const;

      template <typename T, typename Traits>
      friend std::basic_ostream<T, Traits>&
      operator<<(std::basic_ostream<T, Traits>& out, const Timer& timer);
    private:
      high_resolution_clock::time_point _start;
    };


    /* ========== Template Functions ============ */

    template <typename T, typename Traits>
    std::basic_ostream<T, Traits>&
    operator<<(std::basic_ostream<T, Traits>& out, const Timer& timer) {
      return out << timer.Elapsed().count();
    }

  }
}

#endif
