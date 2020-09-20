#ifndef __SIMULATE_HMM_H_
#define __SIMULATE_HMM_H_

#include <track-select>

namespace track_select {
  namespace simulate {
    class HMM {
      unsigned short states_;
      unsigned short current_state_;

      bool at_zero_;

      Vector pi_;
      Matrix tr_;

      std::vector<real> pi_wheel_;
      std::vector<std::vector<real>> tr_wheels_;

      std::random_device dev_;
      std::uniform_real_distribution<real> dist_;
      unsigned short spinWheel(const std::vector<real>& wheel);

    public:
      HMM(Matrix tr, Vector pi);
      inline unsigned short states() const;
      inline bool atZero() const;
      inline unsigned short currentState() const;
      void reset();
      unsigned short operator()();


    };


    /* === Inline functions === */

    unsigned short HMM::states() const {
      return states_;
    }

    bool HMM::atZero() const {
      return at_zero_;
    }

    unsigned short HMM::currentState() const {
      return current_state_;
    }

  }
}
#endif
