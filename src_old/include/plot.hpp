#ifndef __PLOT_H_
#define __PLOT_H_

#include <track-select>
#include "data/feature_vector_set.hpp"

namespace track_select {
  namespace plot {
    void gnuplot(std::string str);

    void image_reconstruction(std::vector<std::pair<Vector, Vector>> images,
                              std::string fname);

    void featureVectorSet(const data::FeatureVectorSet& data,
                          std::map<unsigned short, std::string> legend,
                          unsigned int dim1,
                          unsigned int dim2,
                          unsigned int dim3);
  }
}

#endif
