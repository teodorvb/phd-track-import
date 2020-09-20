#include "objective/common.hpp"

namespace track_select {
  namespace objective {

    real classify(const std::vector<Vector>& seq,
		  const std::vector<hmm::GHMM>& models,
		  const Vector& priors) {
      unsigned short max_index = 0;
      real max = models[0].p(seq.begin(), seq.end()) + priors(0);
      for (unsigned int i = 1; i < models.size(); i++) {
	real p = models[0].p(seq.begin(), seq.end()) + priors(i);
	if (max > p) {
	  max = p;
	  max_index = i;
	}
      }

      return (real)max_index;
    }
    
    Vector classify(const std::vector<std::vector<std::vector<Vector>>>& data,
		    const std::vector<hmm::GHMM>& models,
		    const Vector& priors) {

      int data_size = 0;
      for (const auto& ds : data)
	data_size += ds.size();
      
      Vector asg(data_size);
      int index = 0;
      for (const auto& cat : data) {
	for (const auto& seq : cat) {
	  asg(index++) = classify(seq, models, priors);
	}
      }
      return asg;
    }
    
    
    real f1_score(const Matrix& confusion) {
      real tp = confusion(msmm::YES,msmm::YES);
      real fp = confusion(msmm::NO, msmm::YES);
      real fn = confusion(msmm::YES, msmm::NO);

      real purity = tp/(tp + fp);
      real recall = tp/(tp + fn);
      return 2 * (purity * recall)/(purity + recall);
    }

  }
}
