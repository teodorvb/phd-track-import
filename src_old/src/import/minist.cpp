#include "import/minist.hpp"
#include <fstream>

namespace track_select {
  namespace import {

    unsigned int readInt(void * data) {
      unsigned int data_item = 0;

      for (int j = 0; j < 4; j++)
        data_item += ((unsigned int)(*((unsigned char*)data + j)) << 8*(3 - j));
      return data_item;
    }

    unsigned int readByte(void * data) {
      return (unsigned int)(*((unsigned char*)data));
    }

    Minist::Minist(std::string data_path, std::string label_path)
      : feature_vector_set_("Minist Data Set. Images of hand written symbols "
                            "with resolution 28x28. Each pixel has intensity "
                            "values between 0 and 255") {

      std::ifstream data_file(data_path, std::ios::binary);
      std::ifstream label_file(label_path, std::ios::binary);

      // get length of file:
      data_file.seekg (0, data_file.end);
      int data_bytes = data_file.tellg();
      data_file.seekg (0, data_file.beg);


      label_file.seekg(0, label_file.end);
      int label_bytes = label_file.tellg();
      label_file.seekg (0, data_file.beg);

      char * data = new char[data_bytes];
      char * label = new char[label_bytes];
      // read data as a block:
      data_file.read(data, data_bytes);
      label_file.read(label, label_bytes);
      data_file.close();
      label_file.close();


      unsigned int data_index = 0;
      unsigned int label_index = 0;

      data_magic_number_ = readInt(data + data_index);
      label_magic_number_ = readInt(label + label_index);

      data_index += 4;
      label_index += 4;

      image_count_ = readInt(data + data_index);
      if (image_count_ != readInt(label + data_index))
          throw std::runtime_error("Label count is different from image count");

      data_index += 4;
      label_index += 4;

      image_rows_ = readInt(data + data_index);
      data_index += 4;
      image_cols_ = readInt(data + data_index);
      data_index += 4;


      for (unsigned int image_id = 0; image_id < image_count_; image_id++) {
        Vector vec(image_rows_ * image_cols_);

        for (unsigned int r = 0; r < image_rows_; r++) {
          for (unsigned int c = 0; c < image_cols_; c++) {
            vec(r* image_cols_ + c) =  readByte(data + data_index++);

          }
        }

        labels_.push_back(readByte(label + label_index++));
        vectors_.push_back(vec);
        data::FeatureVector f_vec(vec);
        f_vec.setCategory(labels_.back());

        feature_vector_set_ << f_vec;
      }

      delete data;
      delete label;
    }

    const std::vector<Vector>& Minist::vectors() const {
      return vectors_;
    }

    unsigned int Minist::imageRows() const {
      return image_rows_;
    }

    unsigned int Minist::imageCols() const {
      return image_cols_;
    }

    const data::FeatureVectorSet& Minist::featureVectorSet() const {
      return feature_vector_set_;
    }

  }
}
