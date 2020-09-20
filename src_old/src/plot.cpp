#include "plot.hpp"

#include <fstream>

namespace track_select {
  namespace plot {
    void gnuplot(std::string str) {
      std::ofstream tmp("tmp");
      tmp << str;

      tmp.close();
      std::system("gnuplot tmp -p && rm tmp");
    }

    void image_reconstruction(std::vector<std::pair<Vector, Vector>> images,
                              std::string fname) {
      throw ErrorNotImplemented("plot::image_reconstruction");
    }


    void featureVectorSet(const data::FeatureVectorSet& data,
                          std::map<unsigned short, std::string> legend,
                          unsigned int dim1,
                          unsigned int dim2,
                          unsigned int dim3) {
      throw ErrorNotImplemented("plot::featureVectorSet");

    }
  }
}
