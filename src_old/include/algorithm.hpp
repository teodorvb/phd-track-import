#ifndef __TS_ALGORITHM_H_
#define __TS_ALGORITHM_H_
#include <track-select>
#include <fstream>

namespace track_select {
  namespace algorithm {


    /** @breif Transforms data set so that ever dimension has variance 1
     *  and mean 0.
     */
    template<class inputIter, class outputIter>
    nn::FeedForwardNetwork whiten(inputIter begin,
                                  inputIter end,
                                  outputIter out) {

      unsigned int dim = begin->rows();
      Vector mean = Vector::Zero(dim);
      for (inputIter it = begin; it != end; it++)
        mean += *it;
      mean /= (end - begin);

      Vector precision = Vector::Zero(dim);
      for (inputIter it = begin; it != end; it++)
        precision += ((*it) - mean).cwiseAbs2();
      precision /= (end-begin);
      precision = precision.cwiseSqrt().cwiseInverse();

      Vector biases = -mean.cwiseProduct(precision);
      Matrix weights = Matrix::Zero(dim, dim);
      weights.diagonal() = precision;

      nn::FeedForwardNetwork tr({
          std::make_tuple(weights, biases,
                          nn::FeedForwardNetwork::LINEAR_ACTIVATION)});
      for (inputIter it = begin; it != end; it++) {
        *(out) = *it;
        *(out) = tr(*it);
        out++;
      }

      return tr;
    }

    /** @breif Transforms data set so that every dimension has maximum value
     *  higher and minimum value lower.
     *
     *  @note Dimensions will loose their ratio. For example, images may look
     *  different.
     */
    template<class inputIter, class outputIter>
    nn::FeedForwardNetwork allDimSameScale(inputIter begin, inputIter end,
                                           outputIter out,
                                           real lower = 0.1, real higher = 0.9){
      Vector min = *begin;
      for (inputIter it = begin; it != end; it++)
        min = (*it).binaryExpr(min, [](real x, real y) -> real {
            return std::min(x, y);
          });

      Vector max = *begin;

      for (inputIter it = begin; it != end; it++)
        max = (*it).binaryExpr(max, [](real x, real y) -> real {
            return std::max(x, y);
          });

      Vector scale = (max - min).cwiseInverse() * (higher - lower);

      Vector biases = (-min).cwiseProduct(scale).unaryExpr([=](real x) -> real {
          return x + lower;
        });
      Matrix weights = Matrix::Zero(scale.rows(), scale.rows());
      weights.diagonal() = scale;

      nn::FeedForwardNetwork tr({
          std::make_tuple(weights, biases,
                          nn::FeedForwardNetwork::LINEAR_ACTIVATION)});

      for (inputIter it = begin; it != end; it++) {
        *(out) = *it;
        *(out) = tr(*it);
        out++;
      }

      return tr;
    }

    /** @breif transforms a data set so that the minimum value in the data set
     *  is lower and the maximum is higher
     *
     *  @note Ratio between dimensions are the same. For example images would
     *  just get darker.
     */
    template<class inputIter, class outputIter>
    nn::FeedForwardNetwork mapToRange(inputIter begin, inputIter end,
                                      outputIter out,
                                      real lower = 0.1, real higher = 0.9) {
      unsigned int dim = begin->rows();

      real min = begin->minCoeff();
      for (inputIter it = begin; it != end; it++)
        min = std::min(min, it->minCoeff());

      real max = begin->maxCoeff();

      for (inputIter it = begin; it != end; it++)
        max = std::max(max, it->maxCoeff());

      real scale = (higher - lower)/(max - min);

      Vector biases = Vector::Ones(dim) * (lower - min*scale);
      Matrix weights = Matrix::Zero(dim, dim);
      weights.diagonal() = Vector::Ones(dim)*scale;

      nn::FeedForwardNetwork tr({
          std::make_tuple(weights, biases,
                          nn::FeedForwardNetwork::LINEAR_ACTIVATION)});

      for (inputIter it = begin; it != end; it++) {
        *(out) = *it;
        *(out) = tr(*it);
        out++;
      }

      return tr;
    }



    template<class iter>
    void random_shuffle(iter begin, iter end) {
      static std::random_device rd;
      static std::mt19937 g(rd());
      std::shuffle(begin, end, g);
    }


    std::string makeUUID();


#endif
