#include "algorithm.hpp"
#include "data/feature_vector.hpp"

namespace ts = track_select;
int main(int argc, char** argv) {
  std::vector<int> data1 = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
  std::vector<int> data2 = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
  if (data1 != data2) return -1;

  ts::algorithm::random_shuffle(data1.begin(), data1.end());
  if (data1 == data2) return -1;

  ts::algorithm::random_shuffle(data1.begin(), data1.end());
  if (data1 == data2) return -1;

  std::vector<ts::data::FeatureVector> data;
  std::random_device dev;

  std::normal_distribution<ts::real> dim0(4, 10);
  std::normal_distribution<ts::real> dim1(0, 2);
  std::normal_distribution<ts::real> dim2(20, 20);
  std::uniform_int_distribution<unsigned short> category(0, 65535);
  std::uniform_int_distribution<char> source_data_set(0, 255);
  std::uniform_int_distribution<unsigned int> track_id(0, 16000000);
  std::uniform_int_distribution<unsigned int> source_range(0, 16000000);
  for (unsigned int i =0; i < 10000; i++) {
    ts::data::FeatureVector vec(3);
    vec(0) = dim0(dev);
    vec(1) = dim1(dev);
    vec(2) = dim2(dev);

    vec.setCategory(category(dev));
    vec.setSourceTrackId(category(dev));

    std::string buffer;
    buffer.resize(100);
    for (unsigned int j = 0; j < 100; j++)
      buffer[j] = source_data_set(dev);

    vec.setSourceDataSet(buffer);

    unsigned int source_range_low = source_range(dev);
    vec.setSourceRange(ts::msmm::CustomRange(source_range_low,
                                             source_range_low + 2));

    data.push_back(vec);
  }

  std::vector<ts::data::FeatureVector> tr1(data.size());
  ts::algorithm::whiten(data.cbegin(), data.cend(), tr1.begin());

  ts::Vector mean = ts::Vector::Zero(3);

  for (auto& t : tr1)
    mean += t/(tr1.size());

  std::cout << mean.squaredNorm() << std::endl;
  if (std::abs(mean.squaredNorm()) > 0.01) return -1;

  for (unsigned int i = 0; i < data.size(); i++) {
    if (tr1[i].sourceDataSet() != data[i].sourceDataSet()) return -1;
    if (tr1[i].sourceTrackId() != data[i].sourceTrackId()) return -1;
    if (tr1[i].sourceRange() != data[i].sourceRange()) return -1;
    if (tr1[i].category() != data[i].category()) return -1;
  }

  std::vector<ts::data::FeatureVector> tr2(data.size());
  ts::algorithm::allDimSameScale(data.cbegin(), data.cend(), tr2.begin(),
                                 3, 5);
  ts::Vector tr2_min = tr2.front();
  for (const auto& t : tr2)
    tr2_min = t.binaryExpr(tr2_min, [](ts::real x, ts::real y) -> ts::real {
        return std::min(x, y);
      });

  ts::Vector tr2_max = tr2.front();

  for (const auto& t : tr2)
    tr2_max = t.binaryExpr(tr2_max, [](ts::real x, ts::real y) -> ts::real {
        return std::max(x, y);
      });

  if (tr2_min != ts::Vector::Ones(3)*3) return -1;
  if (tr2_max != ts::Vector::Ones(3)*5) return -1;



  for (unsigned int i = 0; i < data.size(); i++) {
    if (tr2[i].sourceDataSet() != data[i].sourceDataSet()) return -1;
    if (tr2[i].sourceTrackId() != data[i].sourceTrackId()) return -1;
    if (tr2[i].sourceRange() != data[i].sourceRange()) return -1;
    if (tr2[i].category() != data[i].category()) return -1;
  }



  std::vector<ts::data::FeatureVector> tr3(data.size());
  ts::algorithm::mapToRange(data.cbegin(), data.cend(), tr3.begin(),
                            3, 5);

  ts::Vector tr3_min = tr3.front();
  for (const auto& t : tr3)
    tr3_min = t.binaryExpr(tr3_min, [](ts::real x, ts::real y) -> ts::real {
        return std::min(x, y);
      });

  ts::Vector tr3_max = tr3.front();

  for (const auto& t : tr3)
    tr3_max = t.binaryExpr(tr3_max, [](ts::real x, ts::real y) -> ts::real {
        return std::max(x, y);
      });

  if (tr3_min == ts::Vector::Ones(3)*3) return -1;
  if (tr3_max == ts::Vector::Ones(3)*5) return -1;

  ts::real min = tr3.front().minCoeff();
  for (auto& t : tr3)
    min = std::min(min, t.minCoeff());

  ts::real max = tr3.front().maxCoeff();
  for (auto& t : tr3)
    max = std::max(max, t.maxCoeff());

  if (min != 3) return -1;
  if (max != 5) return -1;


  for (unsigned int i = 0; i < data.size(); i++) {
    if (tr3[i].sourceDataSet() != data[i].sourceDataSet()) return -1;
    if (tr3[i].sourceTrackId() != data[i].sourceTrackId()) return -1;
    if (tr3[i].sourceRange() != data[i].sourceRange()) return -1;
    if (tr3[i].category() != data[i].category()) return -1;
  }




  return 0;
}
