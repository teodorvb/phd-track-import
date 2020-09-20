#include <iostream>
#include <track-select>
#include "data/feature_vector_set.hpp"
#include "data/sequence_set.hpp"
#include "algorithm.hpp"

namespace ts = track_select;
int main(int argc, char ** argv) {

  if (argc != 4) {
    std::cerr << "Reduced the size of a labelled data set to specified positive"
              << std::endl
              << "and negative class size. " << std::endl << std::endl
              << "Usage:" << std::endl
              << "\treduce-data-size <data-set-id> <positive-size> "
              << "<negative-size>" << std::endl;
    return -1;
  }
  unsigned int data_set_id;
  unsigned int size_Y;
  unsigned int size_N;

  std::stringstream ss;
  ss << argv[1] << " " << argv[2] << " " << argv[3];
  ss >> data_set_id >> size_Y >> size_N;

  pqxx::connection conn;

  ts::data::FeatureVectorSet source;

  {
    pqxx::work w(conn);
    source = ts::data::FeatureVectorSet::read(data_set_id, w);
  }

  ts::data::FeatureVectorSet Y;
  ts::data::FeatureVectorSet N;
  for (auto& point : source) {
    if (point.category() == ts::msmm::YES) {
      Y << point;
    } else if (point.category() == ts::msmm::NO) {
      N << point;
    } else {
      std::stringstream ss;
      ss << "Unknown category " << point.category();
      throw std::runtime_error(ss.str());
    }
  }

  if (Y.size() < size_Y) {
    std::stringstream ss;
    ss << "Size of yes class (" << Y.size()
       << ") is smaller than specified size: " << size_Y << std::endl;
    throw std::runtime_error(ss.str());
  }

  if (N.size() < size_N) {
    std::stringstream ss;
    ss << "Size of no class (" << N.size()
       << ") is smaller than specified size: " << size_N << std::endl;
    throw std::runtime_error(ss.str());
  }

  std::stringstream reduced_info;
  reduced_info << "Reduced size to ratio Y:N" << size_Y << ":" << size_N
               << ". Extracted from " << source.id() << ".";

  ts::data::FeatureVectorSet reduced(reduced_info.str());
  if (source.frameInfoSet())
    reduced.setFrameInfo(source.frameInfo());

  ts::algorithm::random_shuffle(Y.begin(), Y.end());
  ts::algorithm::random_shuffle(N.begin(), N.end());

  for (unsigned int i = 0; i < size_Y; i++) reduced << Y[i];
  for (unsigned int i = 0; i < size_N; i++) reduced << N[i];

  {
    pqxx::work w(conn);
    std::cout << "Store reduced data set with id " << reduced.store(w)
              << std::endl;
    w.commit();
  }

  return 0;
}
