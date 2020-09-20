#include "import/smi.hpp"
#include "algorithm.hpp"
#include <set>
#include <boost/program_options.hpp>
#include <track-select>
#include <track-select-msmm>
#include <fstream>
#include <thread>
#include <mutex>
#include <tuple>
#include <Eigen/Eigenvalues>

namespace ts = track_select;
namespace po = boost::program_options;

namespace track_select {
  std::tuple<data::FeatureVectorSet, nn::FeedForwardNetwork>
  extract_scores(const data::FeatureVectorSet& src,
                 std::vector<std::string> new_scores,
                 std::string info_suffix) {

    data::FeatureVectorSet reduced;
    std::stringstream info;
    info << src.info() << " "
         << info_suffix
         << " Extracted from data set with id "  << src.id() << ".";

    reduced.setInfo(funcs::splitToLines80(info.str()));
    reduced.setFrameInfo(new_scores);

    std::map<std::string, unsigned int> new_scores_map;
    std::vector<std::string> src_frame_info = src.frameInfo();
    for (auto& new_score : new_scores) {
      unsigned int new_score_index = 0;
      for (; new_score_index < src_frame_info.size(); new_score_index++)
        if (src_frame_info[new_score_index] == new_score) {
          new_scores_map[new_score] = new_score_index;
          break;
        }

      if (new_score_index >= src_frame_info.size()) {
        std::stringstream ss;
        ss << "Could not find combi score with name: "
           << new_score << " in source frame info";
        throw std::runtime_error(ss.str());
      }

    }


    Matrix weights = Matrix::Zero(new_scores.size(), src.dim());
    for (unsigned int i = 0; i < new_scores.size(); i++)
      weights(i, new_scores_map.at(new_scores[i])) = 1;
    Vector biases = Vector::Zero(new_scores.size());

    nn::FeedForwardNetwork
      reduce({std::make_tuple(weights,
                              biases,
                              nn::FeedForwardNetwork::LINEAR_ACTIVATION)});

    for (auto& point : src) {
      ts::data::FeatureVector new_point(reduce(point));
      new_point.setSourceDataSet(point.sourceDataSet());
      new_point.setSourceTrackId(point.sourceTrackId());
      new_point.setSourceRange(point.sourceRange());
      new_point.setCategory(point.category());

      reduced << new_point;
    }

    return std::make_tuple(reduced, reduce);
  }

  std::tuple<data::FeatureVectorSet, nn::FeedForwardNetwork>
  pca(const data::FeatureVectorSet& src, real var) {
    Matrix data(src.dim(), src.size());

    for (unsigned int i = 0; i < src.size(); i++) {
      data.col(i) = src[i];
    }
    Vector biases = - data.rowwise().mean();

    data = data.colwise() + biases;

    Eigen::EigenSolver<Matrix> solver(data * data.transpose());
    Matrix evals = solver.eigenvalues().real();
    Matrix evects = solver.eigenvectors().real();

    real total = evals.sum();
    real sum = 0;
    unsigned int count = 0;
    while( count < evals.rows() && sum/total < var) {
      sum += evals(count++);
    }

    Matrix weights(count , src.dim());
    for (unsigned int i = 0; i < count; i++)
      for (unsigned int j = 0; j < src.dim(); j++)
        weights(i, j) = evects(j, i);

    nn::FeedForwardNetwork reduce({
        std::make_tuple(weights, weights * biases,
                        nn::FeedForwardNetwork::LINEAR_ACTIVATION)});

    nn::FeedForwardNetwork decoder({
        std::make_tuple(weights.transpose(),
                        weights.transpose() * weights * biases,
                        nn::FeedForwardNetwork::LINEAR_ACTIVATION)});

    std::stringstream reduced_info;
    reduced_info << src.info()
                 << " Reduced by PCA. Extracted from data set with id "
                 << src.id() << ".";
    data::FeatureVectorSet reduced(funcs::splitToLines80(reduced_info.str()));

    real err = 0;
    for (auto& p : src)
      err += (decoder(reduce(p)) - p).norm();
    err /= src.size();
    std::cout << "Reconstruction error " << err << std::endl;

    for (auto& point : src) {
      ts::data::FeatureVector new_point(reduce(point));
      new_point.setSourceDataSet(point.sourceDataSet());
      new_point.setSourceTrackId(point.sourceTrackId());
      new_point.setSourceRange(point.sourceRange());
      new_point.setCategory(point.category());

      reduced << new_point;
    }

    return std::make_tuple(reduced, reduce);
  }


  std::tuple<data::FeatureVectorSet, nn::FeedForwardNetwork>
  ac(const data::FeatureVectorSet& src,
     const std::vector<unsigned int>& layers) {
    if (!layers.size())
      throw ErrorInvalidOption("Auto-encoder depts needs to be at least 1");

    std::vector<Vector> train;
    std::vector<nn::FeedForwardNetwork::layer> el;
    std::vector<nn::FeedForwardNetwork::layer> dl;
    std::cout << "preparing training set" << std::endl;
    for (auto& d : src)
      train.push_back(d);
    std::cout << "Training AC with layers ";
    for (auto& hu : layers)
      std::cout << hu << " ";
    std::cout << std::endl;

    nn::RBMGaussianBinary input_layer(layers[0], src.dim());
    std::cout << "Trainging layer 0"  << std::endl;

    input_layer.setLearningRate(0.01);
    input_layer.setVelocity(0.9);
    input_layer.setCost(0);
    input_layer.setMaxEpoch(5000);
    input_layer.setKSteps(1);
    input_layer.setGaussianActivationSigma(0.1);

    input_layer.setLog(&std::cout);
    input_layer.train(train.begin(), train.end());

    for (auto& t : train)
      t = input_layer.hiddenExpectation(t);

    el.push_back(std::make_tuple(input_layer.weights(),
                                 input_layer.hiddenBiases(),
                                 nn::FeedForwardNetwork::SIGMOID_ACTIVATION));

    dl.push_back(std::make_tuple(input_layer.weights().transpose(),
                                 input_layer.visibleBiases(),
                                 nn::FeedForwardNetwork::LINEAR_ACTIVATION));

    std::cout << "Trainign other layers" << std::endl;
    std::vector<nn::RBMSigmoidBinary> rbms;
    for (unsigned int i = 1; i < layers.size(); i++) {
      nn::RBMSigmoidBinary rbm(layers[i], layers[i-1]);

      rbm.setLearningRate(0.1);
      rbm.setVelocity(0.9);
      rbm.setCost(0);
      rbm.setMaxEpoch(2000);
      rbm.setKSteps(1);
      rbm.setSigmoidActivationSigma(0.1);

      rbm.setLog(&std::cout);

      rbms.push_back(rbm);
    }

    for (auto& rbm : rbms) {
      rbm.train(train.begin(), train.end());
      for (auto&  t : train) t = rbm.hiddenExpectation(t);

      el.push_back(std::make_tuple(rbm.weights(),
                                   rbm.hiddenBiases(),
                                   nn::FeedForwardNetwork::SIGMOID_ACTIVATION));
      dl.push_back(std::make_tuple(rbm.weights().transpose(),
                                   rbm.visibleBiases(),
                                   nn::FeedForwardNetwork::SIGMOID_ACTIVATION));
    }


    std::reverse(dl.begin(), dl.end());
    nn::FeedForwardNetwork encoder(el);
    nn::FeedForwardNetwork decoder(dl);

    real err = 0;
    for (auto& s : src) {
      err += (decoder(encoder(s)) - s).norm();
    }
    err /= src.size();

    std::cout << "Reconstruction error " << err << std::endl;

    std::stringstream reduced_info;
    reduced_info << src.info()
                 << " Reduced by AC. Extracted from data set with id "
                 << src.id() << ".";
    data::FeatureVectorSet reduced(funcs::splitToLines80(reduced_info.str()));


    for (auto& point : src) {
      ts::data::FeatureVector new_point(encoder(point));
      new_point.setSourceDataSet(point.sourceDataSet());
      new_point.setSourceTrackId(point.sourceTrackId());
      new_point.setSourceRange(point.sourceRange());
      new_point.setCategory(point.category());

      reduced << new_point;
    }

    return std::make_tuple(reduced, encoder);
  }

}

int main(int argc, char** argv) {

  po::options_description desc("Reduce data dimensionality");
  desc.add_options()
    ("help", "produce help message")
    ("type", po::value<std::string>(), "Specify the algorithm used to reduce "
     "the data dimensionality < auto-encoder | pca | no-intensity | "
     "combi-score-2 | random >")

    ("data-set-id", po::value<unsigned int>(), "Specify the id of the data set"
     " to be reduced.")

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

  if (!vm.count("type")) {
    std::cerr << "Error: missing argument --type" << std::endl
             << std::endl
             << "Usage " << desc << std::endl;
    return -1;
  }

  if (!vm.count("data-set-id")) {
    std::cerr << "Error: missing argument --data-set-id" << std::endl
             << std::endl
             << "Usage " << desc << std::endl;
    return -1;
  }

  std::string type = vm["type"].as<std::string>();
  unsigned int id = vm["data-set-id"].as<unsigned int>();
  pqxx::connection conn;
  pqxx::work w(conn);
  ts::data::FeatureVectorSet source
    = ts::data::FeatureVectorSet::read(id, w);

  std::tuple<ts::data::FeatureVectorSet, ts::nn::FeedForwardNetwork> res;

  if (type == "auto-encoder") {
    po::options_description ac_desc("Auto-Encoder");

    ac_desc.add_options()
      ("layers", po::value<std::string>(), "The units of each layer in the "
       "encoder, apart of the input layer, which will be determined by the "
       "dimensionality of the data.");

    po::variables_map ac_vm;
    po::command_line_parser parser(argc, argv);
    parser.allow_unregistered().options(ac_desc);

    po::parsed_options ac_parsed = parser.run();
    po::store(ac_parsed, ac_vm);
    po::notify(ac_vm);

    if (!ac_vm.count("layers")) {
      std::cerr << "Error argument --layers is missing" << std::endl
                << ac_desc << std::endl;
      return -1;
    }

    std::string str = ac_vm["layers"].as<std::string>();
    std::replace(str.begin(), str.end(), 'x', ' ');
    std::stringstream ss(str);
    std::vector<unsigned int> layers;
    while(1) {
      unsigned int hu;
      ss >> hu;
      if (!ss) break;
      layers.push_back(hu);
    }

    res = ts::ac(source, layers);

  } else if (type == "pca") {
    po::options_description
      random_desc("Principal Component Analysis");

    random_desc.add_options()
      ("variance", po::value<ts::real>(), "Percentage of variance to be "
       "captured by the compression projection. It must be in the interval "
       "(0, 1)");

    po::variables_map vm;
    po::command_line_parser parser(argc, argv);
    parser.allow_unregistered().options(random_desc);

    po::parsed_options parsed = parser.run();
    po::store(parsed, vm);
    po::notify(vm);

    if (!vm.count("variance")) {
      std::cerr << "Error argument --variance is missing" << std::endl
                << random_desc << std::endl;
      return -1;
    }

    ts::real var = vm["variance"].as<ts::real>();
    if (!(var > 0 && var < 1)) {
      std::cerr << "Error argument --variance is invalid value" << std::endl
                << random_desc << std::endl;
      return -1;
    }

    res = ts::pca(source, var);


  } else if (type == "random") {
    po::options_description random_desc("Random score subset");
    random_desc.add_options()
      ("size", po::value<unsigned int>(), "Number of randomly selected scores");

    po::variables_map vm;
    po::command_line_parser parser(argc, argv);
    parser.allow_unregistered().options(random_desc);

    po::parsed_options parsed = parser.run();
    po::store(parsed, vm);
    po::notify(vm);

    if (!vm.count("size")) {
      std::cerr << "Error argument --size is missing" << std::endl
                << random_desc << std::endl;
      return -1;
    }

    unsigned int size = vm["size"].as<unsigned int>();
    std::vector<std::string> frame_info = source.frameInfo();
    ts::algorithm::random_shuffle(frame_info.begin(), frame_info.end());
    frame_info.erase(frame_info.begin() + size, frame_info.end());

    res = ts::extract_scores(source, frame_info, "Random subset of scores.");

  } else if (type == "no-intensity") {
    res = ts::extract_scores(source, ts::msmm::Scores::remove_intensity,
                             "No Intensity and Meaningless scores.");

  }  else if (type == "combi-score2") {
    res = ts::extract_scores(source, ts::msmm::Scores::combi_score_2,
                             "CombiScore2.");

  } else {
    std::cerr << "Error: invalid value " << type << "of the argument --type."
              << std::endl << std::endl
              << "Usage" << desc << std::endl;
    return -1;
  }

  unsigned int reduced_id = std::get<0>(res).store(w);
  std::cout << "Storing compressed data set with id "
            << reduced_id << std::endl;
  std::stringstream fname;
  fname << "tr.compress.id-" << reduced_id << ".h5";
  std::get<1>(res).writeHDF5(fname.str());
  w.commit();



  return 0;
}
