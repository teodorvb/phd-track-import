#ifndef __IMPORT_MINIST_H_
#define __IMPORT_MINIST_H_

#include <string>
#include <vector>
#include <track-select>

#include "data/feature_vector_set.hpp"

namespace track_select {
  namespace import {

    class Minist {
      std::vector<unsigned short> labels_;
      std::vector<Vector> vectors_;
      data::FeatureVectorSet feature_vector_set_;

      unsigned int data_magic_number_;
      unsigned int label_magic_number_;

      unsigned int image_rows_;
      unsigned int image_cols_;
      unsigned int image_count_;

    public:
      Minist(std::string data_path, std::string label_path);

      const std::vector<Vector>& vectors() const;
      const data::FeatureVectorSet& featureVectorSet() const;

      unsigned int imageRows() const;
      unsigned int imageCols() const;
    };

  }
}

#endif
