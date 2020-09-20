#ifndef __OBJECTIVE_FFNN_CLASSIFY_H_
#define __OBJECTIVE_FFNN_CLASSIFY_H_

#include "objective/classify.hpp"
#include "data/feature_vector_set.hpp"

namespace track_select {
  namespace objective {

    unsigned short readTarget(const Vector& target);
    unsigned short readTarget(const real* results, unsigned int c_size);

    class FFNNClassify : public Classify {
      unsigned int hidden_units_count_;

    protected:
      const data::FeatureVectorSet * data_;

    public:
      /* @breif Objective function for Neural Network
       */
      FFNNClassify(unsigned int population_size,
                   unsigned int hidden_units,
                   FitnessType fitness_type,
                   const data::FeatureVectorSet & data);

      FFNNClassify(const FFNNClassify& obj);
      virtual FFNNClassify& operator=(const FFNNClassify& obj);

      virtual optimizer::OptimizerReportItem
      operator()(const Vector& params) const;

      virtual nn::FeedForwardNetwork buildFromParams(const Vector& params) const;

      inline unsigned int hiddenUnitsCount() const;
      virtual const data::FeatureVectorSet& data() const;

      virtual FFNNClassify* clone() const;
      static unsigned int calcParamsCount(unsigned int input_dim,
                                          unsigned int hidden_units,
                                          unsigned int output_dim);
    };

    unsigned int FFNNClassify::hiddenUnitsCount() const {
      return hidden_units_count_;
    }


  }
}

#endif // __OBJECTIVE_FFNN_CLASSIFY_H_
