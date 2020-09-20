#ifndef __OBJECTIVE_COMMON_H_
#define __OBJECTIVE_COMMON_H_

#include "data/sequence_set.hpp"

#include <track-select>
#include<set>

namespace track_select {
  namespace objective {

    /** @breif the confision matrix is of the form confision(assigned, predicted) */
    real f1_score(const Matrix& confusion);
        /** @breif trains HMM to classify sequence set. Pretraining is done by
     *  gaussian mixture model clustering.
     **/

    template<class hmm_type>
    unsigned short classify(const hmm_type& model_yes,
                            real prior_yes,
                            const hmm_type& model_no,
                            real prior_no,
                            const data::Sequence& seq) {


      real p_yes;
      real p_no;
      bool yes_failed = false;
      bool no_failed = false;

      try {
        p_yes = model_yes.p(seq.begin(), seq.end()) + prior_yes;
      } catch (hmm::ErrorHMM e) {
        yes_failed = true;
      }

      try {
        p_no = model_no.p(seq.begin(), seq.end()) + prior_no;
      } catch (hmm::ErrorHMM e) {
        no_failed = true;
      }
      if (yes_failed && no_failed)
        throw hmm::ErrorHMM("Sequence containes onobserved symbol under any "
                            "class");
      if (yes_failed)
        return 0;
      if (no_failed)
        return 1;

      return p_yes > p_no;
    }

    real classify(const std::vector<Vector>& seq,
		  const std::vector<hmm::GHMM>& models,
		  const Vector& priors);
    
    Vector classify(const std::vector<std::vector<std::vector<Vector>>>& data,
		    const std::vector<hmm::GHMM>& models,
		    const Vector& priors);
		    
    template<class containerIt>
    std::vector<hmm::GHMM>
    gHMMtrain(containerIt begin, containerIt end,
              const std::vector<unsigned int>& hidden_units,
              real gmm_init_sigma,
              real gmm_init_mean_samples) {

      unsigned int category_count = hidden_units.size();

      std::vector<std::vector<std::vector<Vector>>> data_set(category_count);
      Vector priors = Vector::Zero(category_count);

      for (auto it = begin; it != end; it++) {
        priors(it->category())++;
        data_set[it->category()].push_back(std::vector<Vector>());
        for (auto& frame : *it)
          data_set[it->category()].back().push_back(frame);
      }

      priors /= priors.sum();

      std::vector<std::vector<std::tuple<Vector, Matrix>>> em(category_count);
      for (unsigned int c = 0; c < category_count; c++) {
        std::vector<Vector> observed_data;
        for (auto & data_point : data_set[c])
          for (auto& frame : data_point)
            observed_data.push_back(frame);
        for (auto& t : clustering::gmm(observed_data.begin(),
                                       observed_data.end(),
                                       hidden_units[c],
                                       gmm_init_sigma,
                                       gmm_init_mean_samples)) {
          em[c].push_back(std::make_tuple(std::get<1>(t), std::get<2>(t)));
        } //endfor
      } //endfor

      std::vector<hmm::GHMM> models;
      for (unsigned int c = 0; c < category_count; c++) {
        Vector pi = Vector::Ones(hidden_units[c]);
        Matrix tr = Matrix::Ones(hidden_units[c], hidden_units[c]);
        hmm::GHMM::normalize(pi, tr);
        models.push_back(hmm::GHMM(pi, tr, em[c]));
      }

      Vector old_asg(end - begin);
      Vector new_asg(end - begin);
      new_asg = classify(data_set, models, priors);

      do {
        old_asg = new_asg;
        for (unsigned int c = 0; c < category_count; c++)
          models[c].train(data_set[c].begin(),  data_set[c].end());

	new_asg = classify(data_set, models, priors);
      } while(old_asg != new_asg);

      return models;
    }



    /** @breif trains HMM to classify sequence set. Pretraining is done by
     *  gaussian mixture model clustering.
     **/
    template<class containerIt>
    std::vector<hmm::DiscreteHMM>
    DiscreteHMMtrain(containerIt data_begin,
                     containerIt data_end,
                     unsigned int hu_y,
                     unsigned int hu_n) {

      std::vector<data::Sequence> yes;
      std::vector<data::Sequence> no;
      for (auto it = data_begin; it != data_end; it++)
        if (it->category() == msmm::YES)
          yes.push_back(*it);
        else
          no.push_back(*it);
      unsigned int data_size = data_end - data_begin;

      real prior_yes = (real)yes.size()/data_size;
      real prior_no = (real)no.size()/data_size;


      std::set<unsigned int> alphabet;
      for (auto it = data_begin; it != data_end; it++)
        for (auto& o : *it)
          alphabet.insert(o(0));

      Vector o_priors = Vector::Zero(alphabet.size());

      for (auto it = data_begin; it != data_end; it++)
        for (auto& o : *it)
          o_priors(o(0))++;

      o_priors /= o_priors.sum();

      Vector pi;
      Matrix tr;

      pi = Vector::Random(hu_y);
      tr = Matrix::Random(hu_y, hu_y);
      hmm::DiscreteHMM::normalize(pi, tr);
      hmm::DiscreteHMM model_yes(pi, tr, o_priors.replicate(1, hu_y));

      pi = Vector::Random(hu_n);
      tr = Matrix::Random(hu_n, hu_n);
      hmm::DiscreteHMM::normalize(pi, tr);
      hmm::DiscreteHMM model_no(pi, tr, o_priors.replicate(1, hu_n));

      Vector old_asg(data_size);
      Vector new_asg(data_size);

      unsigned int index;
      index = 0;
      for (auto it = data_begin; it != data_end; it++)
        new_asg(index++) = classify(model_yes, prior_yes,
                                    model_no, prior_no,
                                    *it);
      do {
        old_asg = new_asg;

        model_yes.train(yes.begin(),  yes.end());
        model_no.train(no.begin(),  no.end());

        index = 0;
        for (auto it = data_begin; it != data_end; it++)
          new_asg(index++) = classify(model_yes, prior_yes,
                                      model_no, prior_no,
                                      *it);

      } while(old_asg != new_asg);

      std::vector<hmm::DiscreteHMM> models;
      models.push_back(model_no);
      models.push_back(model_yes);

      return models;
    }




  } // objective
} // track_select

#endif
