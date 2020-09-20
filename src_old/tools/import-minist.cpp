#include "import/minist.hpp"
#include "algorithm.hpp"
#include <set>
#include <boost/program_options.hpp>

namespace ts = track_select;
namespace po = boost::program_options;

ts::data::FeatureVectorSet extract(const ts::import::Minist& train,
                                   unsigned int data_size,
                                   std::string& info) {
  std::map<unsigned short, std::vector<ts::data::FeatureVector>> samples;
  std::map<unsigned short, std::set<unsigned int>> indexes;

  for (auto& sample : train.featureVectorSet()) {
    samples[sample.category()].push_back(sample);
  }

  std::random_device dev;
  for (unsigned short i = 0; i < 10; i++) {
    unsigned int max_int = samples.at(i).size();
    std::uniform_int_distribution<unsigned int> dist(0, max_int - 1);
    for (unsigned int j = 0; j < data_size / 10; j++) {
      unsigned int count_before = indexes[i].size();
      do {
        indexes[i].insert(dist(dev));
      } while(indexes[i].size() == count_before);

    }
  }

  ts::data::FeatureVectorSet data_set(info);
  for (auto& key_val : indexes)
    for (auto& index : key_val.second)
      data_set << samples.at(key_val.first)[index];

  ts::algorithm::random_shuffle(data_set.begin(), data_set.end());

  std::map<unsigned short, unsigned int> sample_counts;
  for (auto& sample : data_set)
    sample_counts[sample.category()]++;

  return data_set;
}

int main(int argc, char** argv) {

  po::options_description desc("Imports MINST data set");
  desc.add_options()
    ("help", "produce help message")
    ("data", po::value<std::string>(), "path to data file")
    ("labels", po::value<std::string>(), "path to labels file")
    ("size", po::value<unsigned int>(), "data size")
    ("info", po::value<std::string>(), "Info about data set");

  po::variables_map vm;
  po::command_line_parser parser(argc, argv);
  parser.allow_unregistered().options(desc);

  po::parsed_options parsed = parser.run();
  po::store(parsed, vm);
  po::notify(vm);

  if (vm.count("help")) {
    std::cout << desc << std::endl;
    return 1;
  }

  if (!vm.count("data")) {
    std::cerr << "Error: missing data file path." << std::endl
              << std::endl
              << "Usage" << desc << std::endl;
    return -1;
  }

  if (!vm.count("labels")) {
    std::cerr << "Error: missing labels file path." << std::endl
              << std::endl
              << "Usage" << desc << std::endl;
    return -1;
  }


  if (!vm.count("size")) {
    std::cerr << "Error: missing data size." << std::endl
              << std::endl
              << "Usage" << desc << std::endl;
    return -1;
  }


  if (!vm.count("info")) {
    std::cerr << "Error: missing data info." << std::endl
              << std::endl
              << "Usage" << desc << std::endl;
    return -1;
  }

  std::string data_fname = vm["data"].as<std::string>();
  std::string label_fname = vm["labels"].as<std::string>();
  unsigned int size = vm["size"].as<unsigned int>();
  std::string info = vm["info"].as<std::string>();

  ts::import::Minist train(data_fname, label_fname);

  ts::data::FeatureVectorSet train_set = extract(train, size, info);

  pqxx::connection conn("");
  pqxx::work w(conn);
  std::cout << "Save train with id: " << train_set.store(w) << std::endl;
  w.commit();

  return 0;
}
