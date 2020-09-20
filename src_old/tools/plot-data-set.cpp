#include "data/feature_vector_set.hpp"
#include <iostream>
#include <map>
#include "plot.hpp"
#include <TApplication.h>

namespace ts = track_select;
int main(int argc, char** argv) {

  if (argc != 5) {
    std::cerr << "Plots the first 3 dimensions of a data set"
              << std::endl << std::endl << "Usage:" << std::endl
              << "\tplot-data-set <data-set-id>" << std::endl;
    return -1;
  }
  unsigned int data_set_id;
  unsigned int dim1, dim2, dim3;

  std::stringstream ss;
  ss << argv[1] << " "
     << argv[2] << " "
     << argv[3] << " "
     << argv[4];

  ss >> data_set_id
     >> dim1
     >> dim2
     >> dim3;


  ts::data::FeatureVectorSet  data_set;
  pqxx::connection conn;
  {
    pqxx::work w(conn);
    data_set = ts::data::FeatureVectorSet::read(data_set_id, w);
  }

  std::map<unsigned short, std::string> legend = {
    {0, "Not Selected"},
    {1, "Analysed"},
    {2, "Rejected"},
    {3, "Imported"}
  };

  TApplication theApp("App", &argc, argv);
  ts::plot::featureVectorSet(data_set, legend, dim1, dim2, dim3);
  theApp.Run();

  return 0;
}
