#ifndef __OBJECTIVE_HMM_H_
#define __OBJECTIVE_HMM_H_

#include "objective/classify.hpp"
#include "data/sequence_set.hpp"

namespace track_select {
  namespace objective {

    class HMM : public Classify {
    private:
      unsigned int hidden_units_count_;
      unsigned int hidden_states_count_;
    protected:
      const data::SequenceSet* data_;
      Vector priors_;
    public:
      /* @breif Objective function for Neural Network
       */
      HMM(unsigned int population_size,
          unsigned int hidden_units,
          unsigned int hidden_states,
          FitnessType fitness_type,
          const data::SequenceSet& data);

      HMM(const HMM& obj);

      virtual HMM& operator=(const HMM& obj);

      virtual optimizer::OptimizerReportItem
      operator()(const Vector& params) const;


      virtual std::vector<hmm::NNHMM>
      buildFromParams(const Vector& params) const;

      inline unsigned int hiddenUnitsCount() const;
      inline unsigned int hiddenStatesCount() const;
      inline const data::SequenceSet& data() const;

      virtual HMM* clone() const;
    };


    unsigned int HMM::hiddenUnitsCount() const {
      return hidden_units_count_;
    }

    unsigned int HMM::hiddenStatesCount() const {
      return hidden_states_count_;
    }

    const data::SequenceSet& HMM::data() const {
      return *data_;
    }
  }
}
#endif
