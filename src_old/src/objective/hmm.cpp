#include "objective/hmm.hpp"
#include <algorithm>
#include <tuple>

namespace track_select {
  namespace objective {

    unsigned int calcNNHMMParams(unsigned int hidden_units,
                                 unsigned int hidden_states,
                                 unsigned int observed_dim,
                                 unsigned int categories) {
      return categories * (std::pow(hidden_states, 2) + hidden_states +
                           hidden_states * ((observed_dim + 1) * hidden_units +
                                            hidden_units + 1));
    }


    HMM::HMM(unsigned int p_size,
             unsigned int hidden_units,
             unsigned int hidden_states,
             HMM::FitnessType fitness_type,
             const data::SequenceSet& data)
      : Classify(p_size,
                 calcNNHMMParams(hidden_units,
                                 hidden_states,
                                 data.dim(),
                                 calcCategoriesCount(data)),
                 fitness_type,
                 data.dim(),
                 calcCategoriesCount(data)),
        hidden_units_count_(hidden_units),
        hidden_states_count_(hidden_states),
      data_(&data) {

      priors_ = Vector::Zero(categoriesCount());
      for (auto& s : (*data_)) priors_(s.category())++;

      priors_ /= priors_.sum();
    }


    HMM::HMM(const HMM& obj)
      : Classify((const Classify&)obj),
        hidden_units_count_(obj.hidden_units_count_),
        hidden_states_count_(obj.hidden_states_count_),
        data_(obj.data_),
        priors_(obj.priors_) {}



    HMM& HMM::operator=(const HMM& obj) {
      ObjectiveFunc::operator=((const ObjectiveFunc&)obj);
      hidden_units_count_ = obj.hidden_units_count_;
      hidden_states_count_ = obj.hidden_states_count_;

      data_ = obj.data_ ;
      priors_ = obj.priors_;

      return *this;
    }

    optimizer::OptimizerReportItem
    HMM::operator()(const Vector& params) const {
      std::vector<hmm::NNHMM> cls = buildFromParams(params);

      std::vector<std::tuple<unsigned short, Vector>> class_probs;
      // The matrix has the following convention (true label, preducted label);

      for (auto& sample : (*data_)) {
        Vector log_probs(cls.size());
        for (unsigned int i = 0; i < cls.size(); i++)
          log_probs(i) = cls[i].p(sample.begin(), sample.end());
        class_probs.push_back(std::make_tuple(sample.category(), log_probs));
      }

      optimizer::OptimizerReportItem item = buildItem(class_probs);
      item.setVector(params);
      return item;
    }

    std::vector<hmm::NNHMM>
    HMM::buildFromParams(const Vector& params) const {
      std::vector<hmm::NNHMM> res;
      unsigned int index = 0;

      for (unsigned int c_i = 0; c_i < categoriesCount(); c_i++) {
        // Build the initial state probability vector
        Vector pi(hiddenStatesCount());
        for (unsigned int s_i = 0; s_i < hiddenStatesCount(); s_i++)
          pi(s_i) = params(index++);


        // Build the transition probabilites
        Matrix tr(hiddenStatesCount(), hiddenStatesCount());
        for (unsigned int s1_i = 0; s1_i < hiddenStatesCount(); s1_i++)
          for (unsigned int s2_i = 0; s2_i < hiddenStatesCount(); s2_i++)
            tr(s1_i, s2_i) = params(index++);

        hmm::NNHMM::normalize(pi, tr);

        std::vector<std::vector<nn::FeedForwardNetwork::layer>> em;
        for (unsigned int states_i = 0; states_i < hiddenStatesCount();
             states_i++) {

          Vector hidden_biases(hiddenUnitsCount());
          Matrix hidden_weights(hiddenUnitsCount(), inputDim());
          Vector output_biases(1);
          Matrix output_weights(1, hiddenUnitsCount());

          for (unsigned int hu = 0; hu < hiddenUnitsCount(); hu++)
            hidden_biases(hu) = params(index++);

          for (unsigned int hu = 0; hu < hiddenUnitsCount(); hu++)
            for (unsigned int i = 0; i < inputDim(); i++)
              hidden_weights(hu, i) = params(index++);


          output_biases(0) = params(index++);

          for (unsigned int hu = 0; hu < hiddenUnitsCount(); hu++)
            output_weights(0, hu) = params(index++);

          em.push_back({
              std::make_tuple(hidden_weights, hidden_biases,
                              nn::FeedForwardNetwork::SIGMOID_ACTIVATION),
                std::make_tuple(output_weights, output_biases,
                                nn::FeedForwardNetwork::SIGMOID_ACTIVATION)});
        } // endfor states

        res.push_back(hmm::NNHMM(pi, tr, em));
      } // endfor category

      return res;
    }



    HMM* HMM::clone() const {
      return new HMM(*this);
    }
  }
}
