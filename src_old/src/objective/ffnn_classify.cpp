#include "objective/ffnn_classify.hpp"
#include <set>
#include <track-select-msmm>

namespace track_select {
  namespace objective {
    real e(real x) {
      return exp(x);
    }

    Vector softmax(Vector vec) {
      vec = vec.unaryExpr(&e);
      return vec/vec.sum();
    }

    unsigned int FFNNClassify::calcParamsCount(unsigned int input_units,
                                               unsigned int hidden_units,
                                               unsigned int output_units) {
      return (input_units + 1) * hidden_units +
        (hidden_units + 1) * output_units;
    }


    unsigned short readTarget(const Vector& target) {
      Vector::Index max_row;
      target.maxCoeff(&max_row);
      return max_row;
    }

    unsigned short readTarget(const real* results, unsigned int c_size) {
      unsigned short max_index = 0;

      for (unsigned short i = 1; i < c_size; i++)
        if (results[i] > results[max_index])
          max_index = i;

      return max_index;
    }

    FFNNClassify::FFNNClassify(unsigned int p_size,
                               unsigned int hidden_units,
                               Classify::FitnessType fitness_type,
                               const data::FeatureVectorSet& data)
      : Classify(p_size,
                 calcParamsCount(data.dim(),
                                 hidden_units,
                                 calcCategoriesCount(data)),
                 fitness_type,
                 data.dim(),
                 calcCategoriesCount(data)),
      hidden_units_count_(hidden_units),
      data_(&data) {}


    FFNNClassify::FFNNClassify(const FFNNClassify& obj)
      : Classify((const Classify&)obj),
        hidden_units_count_(obj.hidden_units_count_),
        data_(obj.data_) {}



    FFNNClassify& FFNNClassify::operator=(const FFNNClassify& obj) {
      Classify::operator=((const Classify&)obj);
      data_ = obj.data_ ;
      hidden_units_count_ = obj.hidden_units_count_;

      return *this;
    }


    optimizer::OptimizerReportItem
    FFNNClassify::operator()(const Vector& params) const {

      nn::FeedForwardNetwork nn = buildFromParams(params);
      // The matrix has the following convention (true label, preducted label);
      std::vector<std::tuple<unsigned short, Vector>> class_probs;
      for (auto& sample : (*data_))
        class_probs.push_back(std::make_tuple(sample.category(),
                                              softmax(nn(sample))));

      optimizer::OptimizerReportItem item = buildItem(class_probs);
      item.setVector(params);
      return item;
    }

    nn::FeedForwardNetwork
    FFNNClassify::buildFromParams(const Vector& params) const {
      Matrix hidden_weights(hiddenUnitsCount(), inputDim());
      Vector hidden_biases(hiddenUnitsCount());

      Matrix output_weights(categoriesCount(), hiddenUnitsCount());
      Vector output_biases(categoriesCount());

      unsigned int index = 0;

      for (unsigned int i = 0; i < hidden_weights.rows(); i++)
        for (unsigned int j = 0; j < hidden_weights.cols(); j++)
          hidden_weights(i, j) = params[index++];

      for (unsigned int i = 0; i < hidden_biases.rows(); i++)
        hidden_biases(i) = params[index++];

      for (unsigned int i = 0; i < output_weights.rows(); i++)
        for (unsigned int j = 0; j < output_weights.cols(); j++)
          output_weights(i, j) = params[index++];

      for (unsigned int i = 0; i < output_biases.rows(); i++)
        output_biases(i) = params[index++];


      return nn::FeedForwardNetwork({
          std::make_tuple(hidden_weights,
                          hidden_biases,
                          nn::FeedForwardNetwork::SIGMOID_ACTIVATION),
            std::make_tuple(output_weights,
                            output_biases,
                            nn::FeedForwardNetwork::SIGMOID_ACTIVATION)});
    }


    const data::FeatureVectorSet& FFNNClassify::data() const {
      return *data_;
    }


    FFNNClassify* FFNNClassify::clone() const {
      return new FFNNClassify(*this);
    }

  }
}
