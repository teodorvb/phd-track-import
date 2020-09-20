#include <track-select>
#include <fstream>
#include <iostream>
#include <string>
#include <boost/tokenizer.hpp>
#include <iomanip>

#include "plot.hpp"
#include "algorithm.hpp"
#include "data/feature_vector_set.hpp"
#include "data/sequence_set.hpp"
#include "objective/common.hpp"
#include "tools/timer.hpp"
#include "objective/ffnn_classify.hpp"

#include <boost/math/distributions/binomial.hpp>
namespace ts = track_select;

namespace level_detect {
  const int win_size = 5;


  float pks(float z) {
    if (z < 0)  throw std::runtime_error("Bad Z in KSdist");
    if (z == 0) return 0;
    if (z < 1.18) {
      float y = exp(-1.23370055013616983/std::pow(z, 2));
      return 2.25675833419102515*sqrt(-log(y))
        *(y + std::pow(y,9) + std::pow(y,25) + std::pow(y,49));
    } else {
      float x = exp(-2.*std::pow(z, 2));
      return 1. - 2.*(x - std::pow(x,4) + std::pow(x,9));
    }
  }

  float qks(float z) {
    if (z < 0) throw std::runtime_error("Bad Z in KSdist");
    if (z == 0) return 1;
    if (z < 1.18) return 1. - pks(z);
    float x = exp(-2 * std::pow(z, 2));
    return 2*(x - std::pow(x, 4) + std::pow(x, 9));
  }


  float kstest(Eigen::VectorXf data1, Eigen::VectorXf data2) {
    int j1=0,j2=0,n1=data1.size(),n2=data2.size();
    float d1,d2,dt,en1,en2,en,fn1=0.0,fn2=0.0;

    std::sort(data1.data(), data1.data()+n1);
    std::sort(data2.data(), data2.data()+n2);
    en1=n1;
    en2=n2;
    float d=0.0;

    while (j1 < n1 && j2 < n2) {
      if ((d1=data1(j1)) <= (d2=data2(j2)))
        do
          fn1=++j1/en1;
        while (j1 < n1 && d1 == data1(j1));
      if (d2 <= d1)
        do
          fn2=++j2/en2;
        while (j2 < n2 && d2 == data2(j2));
      if ((dt=std::abs(fn2-fn1)) > d) d=dt;
    }

    en=std::sqrt(en1*en2/(en1+en2));
    return qks((en+0.12+0.11/en)*d);
  }

  float kstest(Eigen::VectorXf data, float cdf(const float)) {
    int j, n = data.size();
    float dt, en, ff, fn, fo = 0.0;

    std::sort(data.data(), data.data()+n); // sort in ascending order
    en = n;
    float d = 0.0;
    for (j = 0; j < n; j++) {
      fn = (float)(j+1)/en;
      ff = cdf(data(j));
      dt = std::max(std::abs(fo - ff), std::abs(fn - ff));
      if (dt > d) d = dt;
      fo = fn;
    }

    en = std::sqrt(en);
    return qks((en + 0.12 + 0.11/en)*d);
  }

  // CDF for gaussian distribution with mean 0 and stadnard deviation 1
  float normalCDF(float x) {
    return 0.5 * ( 1.0 + boost::math::erf(x/std::sqrt(2)));
  }

  float eigen_std(Eigen::VectorXf vec) {
    return std::sqrt((vec - Eigen::VectorXf::Ones(vec.size())*vec.mean())
    .cwiseAbs2().sum()/(vec.size() - 1));
  } // eigen_std


  float eigen_median(Eigen::VectorXf vec) {
    int N = vec.size();
    std::sort(vec.data(), vec.data() + N);
    return (N % 2)  ? vec(N/2) : (vec(N/2 - 1) +vec(N/2))/2;
  } // eigen_std

  Eigen::VectorXf eigen_whiten(Eigen::VectorXf vec) {
    return (vec - Eigen::VectorXf::Ones(vec.size())*vec.mean())/eigen_std(vec);
  }


  template<class T>
  bool has_gradient(T vec) {
    Eigen::VectorXf y = vec;
    float m = y.mean();
    if ( m > 1) y/= m;

    Eigen::MatrixXf x(y.size(), 2);
    x.col(0) = Eigen::VectorXf::Ones(y.size());
    x.col(1) = Eigen::VectorXf::LinSpaced(y.size(), 0, 1);

    Eigen::VectorXf prms = x.colPivHouseholderQr().solve(y);

    float std_err = eigen_std(x.col(1)* prms(1) + (prms(0)*Eigen::VectorXf::Ones(y.size()) - y))/(y - Eigen::VectorXf::Ones(y.size())*y.mean()).squaredNorm();

    float slope = prms(1);

    return std::abs(slope)/std_err > 2;
  } // has_gradient

  typedef std::vector<std::pair<int,int> > LevelSegments;


  LevelSegments segment_detect(const Eigen::VectorXf& seq) {

    float tr = 0.2;

    float N = seq.size();
    float win_N = std::floor(N/win_size);
    Eigen::VectorXf wins(win_N);

    for (unsigned int i = 0; i < win_N; i++)
      wins(i) = eigen_std(seq.segment(i*win_size, win_size));

    LevelSegments segments;
    bool level_on = false;
    unsigned int ls = 0;
    for (unsigned int i = 1; i < win_N; i++) {
      if (std::abs(wins.segment(ls, i - ls + 1).mean()/eigen_std(seq.segment(ls*win_size, (i -ls + 1)*win_size)) - 1) < tr &&
          std::abs(wins.segment(i-1, 2).mean()/eigen_std(seq.segment((i-1)*win_size,  2 *win_size)) - 1) < tr &&
          kstest(eigen_whiten(seq.segment(ls*win_size, (i - ls+1)*win_size)), normalCDF) > 0.05 && // This test the entire level for normality
          !has_gradient(seq.segment(ls*win_size, (i - ls + 1)*win_size)) &&
          !has_gradient(seq.segment(i*win_size, win_size))) {

        level_on = true;
      } else if (level_on) {
        segments.push_back(std::make_pair(ls*win_size, (i) * win_size -1));
        level_on = 0;
        ls = i;
      } else {
        ls++;
      } // endif

    } // endfor

    if (level_on) {
      segments.push_back(std::make_pair(ls*win_size, win_N*win_size -1));
    }

    return segments;
  }


  LevelSegments segment_merge(const LevelSegments& sgm, const Eigen::VectorXf& seq) {
    int sgm_N = sgm.size();
    if (!sgm_N) return sgm;

    int merge_N = sgm_N - 1;
    Eigen::VectorXf merge = Eigen::VectorXf::Zero(merge_N);
    for (int i = 0; i < merge_N; i++)
      merge(i) = ((sgm[i+1].first - sgm[i].second) < 2) &&
        (kstest(seq.segment(sgm[i].first, sgm[i].second - sgm[i].first + 1),
                seq.segment(sgm[i+1].first, sgm[i+1].second - sgm[i+1].first + 1)) > 0.05);

    std::cout << merge.transpose() << std::endl;

    LevelSegments merged_segments(sgm_N - merge.sum());
    int merged_index = 0;

    int sgm_index = 0;
    bool merging = false;
    int merge_start = -1;
    for (int i = 0; i < merge_N; i++) {
      if (merge(i) && merging) {
        sgm_index++;

      } else if (merge(i) && !merging) {
        merging = true;
        merge_start = sgm_index;
        sgm_index++;

      } else if (!merge(i) && merging) {
        merging = false;
        merged_segments[merged_index] = std::make_pair(sgm[merge_start].first, sgm[sgm_index].second);
        sgm_index++;
        merged_index++;

      } else if (!merge(i) && !merging) {
        merged_segments[merged_index] = sgm[sgm_index];
        merged_index++;
        sgm_index++;
      } // endif

    } // endfor
    if (merging) {
      merged_segments[merged_index] = std::make_pair(sgm[merge_start].first, sgm[sgm_N-1].second);
    } else {
      merged_segments[merged_index] = sgm[sgm_N -1];
    }

    return merged_segments;
  } // segment_merge

  LevelSegments segment_extend(const LevelSegments& sgm, const Eigen::VectorXf& seq) {
    LevelSegments extended_segments;
    int seq_N = seq.size();
    int sgm_N = sgm.size();

    for (int i = 0; i < sgm_N; i++) {
      Eigen::VectorXf segment = seq.segment(sgm[i].first, sgm[i].second - sgm[i].first + 1);
      float m = eigen_median(segment);
      float s = eigen_std(segment);
      std::pair<int, int> extended_sgm = sgm[i];

      if (sgm[i].second < seq_N-1) { // if can extend on the right try to extend
        int low = (i < sgm_N - 1) ?
          std::min(std::min(sgm[i].second + win_size -1, seq_N - 1), sgm[i+1].first - 1) :
          std::min(sgm[i].second + win_size -1, seq_N - 1);

        for (int j = sgm[i].second + 1; j < low + 1; j++)
          if (std::abs(seq(j) - m) < 2*s)
            extended_sgm.second++;

      } // endif

      if (sgm[i].first > 0) { // if can extend on the left try to extend
        int high = (i > 0) ?
          std::max(std::max(sgm[i].first - win_size +1, 0), extended_segments[i-1].second + 1) :
          std::max(sgm[i].first - win_size + 1, 0);

        for (int j = sgm[i].first - 1; j >= high; j--)
          if (std::abs(seq(j) - m) < 2*s)
            extended_sgm.first--;
      } // endif

      extended_segments.push_back(extended_sgm);
    } // endfor

    return extended_segments;
  } // segment_extend


  LevelSegments segment_split(const LevelSegments& sgm, const Eigen::VectorXf& seq) {
    LevelSegments split_segments;
    int sgm_N = sgm.size();

    for (int i = 0; i < sgm_N; i++) {
      Eigen::VectorXf segment = seq.segment(sgm[i].first, sgm[i].second - sgm[i].first + 1);

      float m = eigen_median(segment);
      float s = eigen_std(segment);

      int sgm_start = 0;
      bool sgm_on = false;

      for (int j = 0; j < segment.size(); j++) {
        bool outlier = std::abs(segment(j) - m) > 3*s;
        if (outlier && sgm_on) {
          if (j - sgm_start + 1 > 3 || true)
            split_segments.push_back(std::make_pair(sgm[i].first + sgm_start, sgm[i].first + j -1));

          sgm_on = false;
        } else if (!outlier && !sgm_on) {
          sgm_start = j;
          sgm_on = true;
        } //endif
      } // endfor
      if (sgm_on) {
        split_segments.push_back(std::make_pair(sgm[i].first + sgm_start, sgm[i].second));
      } //endif

    } // endfor

    return split_segments;
  } // segment_split


  LevelSegments segment_delete(const LevelSegments& sgm, const Eigen::VectorXf& seq) {
    LevelSegments deleted_levels;

    for (unsigned int i = 0; i < sgm.size(); i++) {
      Eigen::VectorXf segment = seq.segment(sgm[i].first, sgm[i].second - sgm[i].first + 1);
      if (segment.size() > 10 && !has_gradient(segment) && kstest(eigen_whiten(segment), normalCDF) > 0.05)
        deleted_levels.push_back(sgm[i]);
    } // endfor

    return deleted_levels;
  }


  std::vector<LevelSegments> created_levels(const LevelSegments& sgm, const Eigen::VectorXf& seq) {
    LevelSegments current_sgm = sgm;
    std::vector<LevelSegments> groups;

    while (current_sgm.size()) {
      std::vector<int> merged_index;
      merged_index.push_back(0);

      int current_sgm_N = current_sgm.size();
      Eigen::VectorXf level = seq.segment(current_sgm[0].first, current_sgm[0].second - current_sgm[0].first +1);

      for (int j = 1; j < current_sgm_N; j++)
        if (kstest(level, seq.segment(current_sgm[j].first, current_sgm[j].second - current_sgm[j].first + 1)) > 0.05)
          merged_index.push_back(j);

      LevelSegments new_segments;
      LevelSegments merged_segments;

      for (unsigned int i = 0; i < current_sgm.size(); i++)
        if (std::find(merged_index.begin(), merged_index.end(), i) != merged_index.end())
          merged_segments.push_back(current_sgm[i]);
        else
          new_segments.push_back(current_sgm[i]);


      current_sgm = new_segments;
      groups.push_back(merged_segments);
    } //endwhile

    return groups;
  } // create_levels

} // level_detect

int main(int argc, char** argv) {
  pqxx::connection conn;
  pqxx::work w(conn);
  ts::data::FeatureVectorSet raw = ts::data::FeatureVectorSet::read(17, w);

  ts::algorithm::whiten(raw.begin(), raw.end(), raw.begin());

  ts::objective::FFNNClassify obj(40, 10, ts::objective::FFNNClassify::F1_SCORE, raw);
  obj.setProperty("initialisation sigma", 1.0);
  obj.setProperty("mutation sigma", 2.0);
  obj.setProperty("adaptive mutation scale", 12);
  obj.setProperty("adaptive mutation bias", -6);
  obj.setProperty("iterations", 1000);

  ts::optimizer::ContinuousGa ga(obj);
  ga();

  std::cout << ga.report().best().fitness() << " " << ga.report().best().property("Accuracy") << std::endl;

  return 0;
}
