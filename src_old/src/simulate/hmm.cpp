#include "simulate/hmm.hpp"

namespace track_select {
  namespace simulate {


    unsigned short HMM::spinWheel(const std::vector<real>& wheel) {
      real ball = dist_(dev_);


      unsigned short next_level = 0;
      while(next_level < wheel.size() - 1) {
        if (ball >= wheel[next_level] && ball < wheel[next_level + 1])
          break;
        next_level++;
      }

      return next_level;
    }


    HMM::HMM(Matrix tr, Vector pi) :
      states_(pi.rows()),
      at_zero_(true),
      pi_(pi),
      tr_(tr),
      dist_(0, 1) {

      if (tr_.rows() != tr_.cols()) {
        std::stringstream ss;
        ss << "Transition matrix dim are incorrect : "
           << tr_.rows() << "x" << tr_.cols() << std::endl;
        throw std::runtime_error(ss.str());
      }

      if (tr_.rows() != pi_.rows())
        throw std::runtime_error("Initial state prob inconsistent with "
                                 "transition prob");
      pi_wheel_.resize(pi_.rows() + 1);
      pi_wheel_[0] = 0;
      for (unsigned short i = 0; i < pi_.rows(); i++)
        pi_wheel_[i + 1] = pi_wheel_[i] + pi_(i);


      for (unsigned short state = 0; state < tr_.rows(); state++) {
        std::vector<real> wheel(tr_.row(state).cols() + 1);

        wheel[0] = 0;
        for (unsigned short i = 0; i < tr_.cols(); i++)
          wheel[i + 1] = wheel[i] + tr_(state, i);

        tr_wheels_.push_back(wheel);
      }

      current_state_ = spinWheel(pi_wheel_);

    }

    void HMM::reset() {
      current_state_ = spinWheel(pi_wheel_);
      at_zero_ = true;
    }

    unsigned short HMM::operator()() {
      at_zero_ = false;

      unsigned short old = current_state_;
      current_state_ = spinWheel(tr_wheels_[old]);

      return old;
    }



  }
}
