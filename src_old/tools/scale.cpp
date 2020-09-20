#include <boost/program_options.hpp>
#include "data/feature_vector_set.hpp"
#include "data/sequence_set.hpp"
#include "algorithm.hpp"

namespace po = boost::program_options;
namespace ts = track_select;

void whiten(unsigned int data_set_id) {
  // Read source
  pqxx::connection conn;
  pqxx::work w(conn);
  ts::data::FeatureVectorSet source =
    ts::data::FeatureVectorSet::read(data_set_id, w);

  // Create new data set
  std::stringstream info;
  info << source.info()
       << " Whitened. Extracted from data set with id " << source.id();

  ts::data::FeatureVectorSet transformed(ts::funcs::splitToLines80(info.str()));
  if (source.frameInfoSet())
    transformed.setFrameInfo(source.frameInfo());
  else
    transformed.setDim(source.dim());

  transformed.allocateSpace(source.size());
  ts::nn::FeedForwardNetwork tr =
    ts::algorithm::whiten(source.begin(), source.end(), transformed.begin());

  // Store
  ts::database_id tr_id = transformed.store(w);
  std::cout << "Data set with id: " << tr_id << " stored" << std::endl;
  w.commit();

  std::stringstream fname;
  fname << "tr.whiten.id-" << tr_id << ".h5";
  tr.writeHDF5(fname.str());

}


void divide_by_l1(unsigned int data_set_id, unsigned int ref, bool divide_all) {
  pqxx::connection conn;
  pqxx::work w(conn);
  ts::data::FeatureVectorSet vect = ts::data::FeatureVectorSet::read(ref, w);
  ts::data::SequenceSet seq = ts::data::SequenceSet::read(data_set_id, w);


  std::sort(vect.begin(), vect.end(),
            [](const ts::data::FeatureVector& left,
               const ts::data::FeatureVector& right) {
              if (left.sourceDataSet() == right.sourceDataSet())
                return left.sourceTrackId() < right.sourceTrackId();

              return left.sourceDataSet() < right.sourceDataSet();
            });

  std::sort(seq.begin(), seq.end(),
            [](const ts::data::Sequence& left,
               const ts::data::Sequence& right) {
              if (left.sourceDataSet() == right.sourceDataSet())
                return left.sourceTrackId() < right.sourceTrackId();

              return left.sourceDataSet() < right.sourceDataSet();
            });

  unsigned int l1_index = 0;
  while (vect.frameInfo()[l1_index] != "cts_mean_lvl_1") {
    l1_index++;
    if (l1_index >= vect.frameInfo().size()) {
      std::stringstream ss;
      ss << "Error: data set with id " << ref << " does not have dimension "
        "cts_mean_lvl_1";
      throw std::runtime_error(ss.str());
    }
  }

  auto data_it = seq.begin();
  auto ref_it = vect.begin();

  std::stringstream info;
  info << seq.info()
       << " Intensity divided by level 1 intensity. Extracted from data set "
       << "with id " << seq.id();

  ts::data::SequenceSet scaled(ts::funcs::splitToLines80(info.str()));
  if (seq.frameInfoSet())
    scaled.setFrameInfo(seq.frameInfo());

  while(data_it != seq.end()) {
    if (ref_it == vect.end())
      throw std::runtime_error("Error reference set ended");

    if ((data_it->sourceDataSet() == ref_it->sourceDataSet()) &&
        (data_it->sourceTrackId() == ref_it->sourceTrackId())) {
      ts::data::Sequence s(*data_it);

      if (divide_all)
        for (auto& frame : s) frame /= (*ref_it)(l1_index);
      else
        for (auto& frame : s) {
          frame(0) /= (*ref_it)(l1_index);
          frame(3) /= (*ref_it)(l1_index);
        }

      data_it++;
      ref_it++;
      scaled << s;
    } else if (((data_it->sourceDataSet() == ref_it->sourceDataSet())
                && (data_it->sourceTrackId() < ref_it->sourceTrackId())) ||
               (data_it->sourceDataSet() < ref_it->sourceDataSet())) {
      data_it++;
    } else if (((data_it->sourceDataSet() == ref_it->sourceDataSet())
                && (data_it->sourceTrackId() > ref_it->sourceTrackId())) ||
               (data_it->sourceDataSet() > ref_it->sourceDataSet())) {
      ref_it++;
    }

  } // endwhile

  std::cout << "Storing sacled data set with id "
            << scaled.store(w) << std::endl;

  w.commit();
}


int main(int argc, char** argv) {

  po::options_description desc("Scales data sets.");
  desc.add_options()
    ("help", "produce help message")
    ("data-set-id", po::value<unsigned int>(), "data set to be transformed")
    ("type", po::value<std::string>(), "[ whiten | divide-by-l1 ]");

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

  if (!vm.count("type")) {
    std::cerr << "Error: missing argument --type" << std::endl
              << "Usage " << desc << std::endl;
    return 1;
  }

  unsigned int data_set_id = vm["data-set-id"].as<unsigned int>();


  if (vm["type"].as<std::string>() == std::string("whiten")) {
    whiten(data_set_id);
  } else if (vm["type"].as<std::string>() == std::string("divide-by-l1")) {

    po::options_description sub_desc("Divides sequence set by level 1 "
                                     "intensity");
    sub_desc.add_options()
      ("reference-set-id", po::value<unsigned int>(),
       "data set used to extract the level 1 intensity")
      ("divide-all", "whether to divide all dimensions or just Intensity");

    po::variables_map sub_vm;
    po::command_line_parser sub_parser(argc, argv);
    sub_parser.allow_unregistered().options(sub_desc);

    po::parsed_options sub_parsed = sub_parser.run();
    po::store(sub_parsed, sub_vm);
    po::notify(sub_vm);

    if (!sub_vm.count("reference-set-id")) {
      std::cerr << "Error: missing argument --reference-set-id" << std::endl
                << "Usage:" << std::endl << sub_desc << std::endl;
      return -1;
    }
    unsigned int reference_set_id =
      sub_vm["reference-set-id"].as<unsigned int>();

    divide_by_l1(data_set_id, reference_set_id, !!sub_vm.count("divide-all"));

  } else {
    std::cerr << "Error unrecognised data set type "
              << vm["type"].as<std::string>() << std::endl;
  }


}
