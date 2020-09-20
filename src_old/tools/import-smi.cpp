#include "import/smi.hpp"
#include "algorithm.hpp"
#include "tools/command_line.hpp"

#include <set>
#include <boost/program_options.hpp>
#include <track-select>
#include <track-select-msmm>
#include <fstream>
#include <thread>
#include <mutex>
#include <fstream>

namespace ts = track_select;
namespace po = boost::program_options;

int main(int argc, char** argv) {

  po::options_description desc("Imports SMI data set");
  desc.add_options()
    ("help", "produce help message")
    ("reference", po::value<std::string>(), "path to reference file (csv). "
     "Format is <data-set-id, track-id, class>. The class can be either "
     "1 (YES) or 0 (NO).")

    ("global-drift-ranges", po::value<std::string>(), "path to file containing "
     "global drift ranges for each data set. Format is <data-set-id, range_log,"
     " range_high>. If there is more than one range add another line with the "
     " same data set id")

    ("info", po::value<std::string>(), "Info about data set")
    ("log", po::value<std::string>(), "log file");

  po::variables_map vm;
  po::command_line_parser parser(argc, argv);
  parser.allow_unregistered().options(desc);

  po::parsed_options parsed = parser.run();
  po::store(parsed, vm);
  po::notify(vm);

  if (vm.count("help")) {
    std::cout << desc << std::endl;
    return -1;
  }

  if (!ts::tools::checkArgument(vm, desc, "reference")) return -1;
  if (!ts::tools::checkArgument(vm, desc, "global-drift-ranges")) return -1;
  if (!ts::tools::checkArgument(vm, desc, "info")) return -1;
  if (!ts::tools::checkArgument(vm, desc, "log")) return -1;

  std::string reference_file = vm["reference"].as<std::string>();
  std::string info = vm["info"].as<std::string>();
  std::string gdr_file = vm["global-drift-ranges"].as<std::string>();
  std::ofstream log(vm["log"].as<std::string>());


  ts::import::ImportData reference;
  ts::import::ImportGDR gdrs;

  try {
    ts::algorithm::readCSV(reference_file, reference);
  } catch (ts::ErrorFileRead e) {
    std::cerr << "Error reading file " << reference_file << std::endl;
    std::cerr << "Error message: " << e.what() << std::endl;
    return -1;
  }


  try {
    ts::algorithm::readCSV(gdr_file, gdrs);
  } catch (ts::ErrorFileRead e) {
    std::cerr << "Error reading file " << gdr_file << std::endl;
    std::cerr << "Error message: " << e.what() << std::endl;
    return -1;
  }

  /* import the batches and save result */
  log << "Importing sub-tracks ..." << std::endl;
  ts::data::SequenceSet image("Sub-Track. " + info + ". Image Sequences");
  ts::data::SequenceSet intensity("Sub-Track. " + info + ". Intensity Sequences");
  ts::data::FeatureVectorSet score("Sub-Track. " + info + ". Score Vectors");

  ts::data::SequenceSet extrp_images("Extrapolated. " + info + ". Image Sequences");
  ts::data::SequenceSet extrp_tracks("Extrapolated. " + info + ". Intensity Sequences");

  ts::import::smiLabelled(reference, gdrs, ts::msmm::Scores::all_scores,
                          image, intensity, score, extrp_images, extrp_tracks,
                          log);

  log << "Data sets created." << std::endl;
  try {
    pqxx::connection conn;
    pqxx::work w(conn);
    log << "Storing image sequence set with ID "
        << image.store(w) << std::endl;

    log << "Storing intensity sequences with ID "
        << intensity.store(w) << std::endl;

    log << "Storing score vectors with ID "
        << score.store(w) << std::endl;

    log << "Storing extrapolated image sequence set with ID "
        << extrp_images.store(w) << std::endl;
    log << "Storing extrapolated intensity sequence set with ID "
        << extrp_tracks.store(w) << std::endl;

    w.commit();
  } catch (std::runtime_error& e) {
    log << "ERROR: " << e.what() << std::endl;
    throw e;
  }

  log << "Data importing finished" << std::endl;
  log.close();

  return 0;
}
