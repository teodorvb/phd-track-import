#include <track-select>
#include <iostream>
#include <boost/program_options.hpp>
#include "data/feature_vector_set.hpp"
#include "plot.hpp"

#include <random>
#include "data/sequence_set.hpp"
#include "simulate/hmm.hpp"
#include "import/minist.hpp"
#include "tools/command_line.hpp"

namespace po = boost::program_options;
namespace ts = track_select;

void plot(ts::data::Sequence& seq1) {
    std::stringstream script;

    script << "$seq1 <<EOD" << std::endl;
    for (auto& s : seq1)  script << s << std::endl;
    script << "EOD" << std::endl;

    script << "plot $seq1 using 0:1 with line lc 'red'" << std::endl;
    ts::plot::gnuplot(script.str());
}

void plot(ts::data::Sequence& seq1, ts::data::Sequence& seq2) {
    std::stringstream script;

    script << "$seq1 <<EOD" << std::endl;
    for (auto& s : seq1)  script << s << std::endl;
    script << "EOD" << std::endl;

    script << "$seq2 <<EOD" << std::endl;
    for (auto& s : seq2)  script << s << std::endl;
    script << "EOD" << std::endl;

    script << "plot $seq1 using 0:1 with line lc 'red', "
           << "$seq2 using 0:1 with line lc 'blue'" << std::endl;
    ts::plot::gnuplot(script.str());
}


ts::data::FeatureVectorSet
generateLinear(ts::real dist, ts::real sigma,
               unsigned int size_positive,
               unsigned int size_negative,
               unsigned int data_dim) {
  std::stringstream info;
  info << "Simulated vector set of type Linear. " << std::endl
       << "  distance - " << dist << std::endl
       << "  sigma - " << sigma << std::endl;

  ts::data::FeatureVectorSet data(info.str());

  std::random_device dev;
  std::normal_distribution<ts::real> random_src(dist, sigma);

  for (unsigned int i = 0; i < size_positive; i++) {
    ts::data::FeatureVector vec(data_dim);
    for (unsigned int j = 0; j < data_dim; j++)
      vec(j) = random_src(dev);
    vec.setCategory(ts::msmm::YES);
    data << vec;
  }


  for (unsigned int i = 0; i< size_negative; i++) {
    ts::data::FeatureVector vec(data_dim);
    for (unsigned int j = 0; j < data_dim; j++)
      vec(j) = -random_src(dev);

    vec.setCategory(ts::msmm::NO);
    data << vec;
  }

  return data;
}


ts::data::FeatureVectorSet
generateXor(ts::real dist, ts::real sigma,
            unsigned int size_positive,
            unsigned int size_negative,
            unsigned int data_dim) {
  std::stringstream info;
  info << "Simulated vector set of type Xor. " << std::endl
       << "  distance - " << dist << std::endl
       << "  sigma - " << sigma << std::endl;

  ts::data::FeatureVectorSet data(info.str());

  std::random_device dev;
  std::normal_distribution<ts::real> random_src(dist, sigma);

  /* Generage Positive */
  for (unsigned int i = 0; i < size_positive/2; i++) {
    ts::data::FeatureVector vec(data_dim);
    for (unsigned int j = 0; j < data_dim; j++)
      vec(j) = random_src(dev);
    vec.setCategory(ts::msmm::YES);
    data << vec;
  }


  for (unsigned int i = size_positive/2; i < size_positive; i++) {
    ts::data::FeatureVector vec(data_dim);
    for (unsigned int j = 0; j < data_dim; j++)
      vec(j) = -random_src(dev);
    vec.setCategory(ts::msmm::YES);
    data << vec;
  }

  /* Generate Negative */
  for (unsigned int i = 0; i< size_negative/2; i++) {
    ts::data::FeatureVector vec(data_dim);
    for (unsigned int j = 0; j < data_dim; j++)
      vec(j) = std::pow(-1, j + 1)*random_src(dev);

    vec.setCategory(ts::msmm::NO);
    data << vec;
  }

  for (unsigned int i = size_negative/2; i< size_negative; i++) {
    ts::data::FeatureVector vec(data_dim);
    for (unsigned int j = 0; j < data_dim; j++)
      vec(j) = std::pow(-1, j)*random_src(dev);

    vec.setCategory(ts::msmm::NO);
    data << vec;

  }

  return data;
}

ts::data::FeatureVectorSet
generateCircle(ts::real rY, ts::real rN, ts::real t,
               unsigned int size_positive,
               unsigned int size_negative,
               unsigned int data_dim) {

  std::stringstream info;
  info << "Simulated vector set of type Circle. " << std::endl
       << "  radius Y - " << rY << std::endl
       << "  radius N - " << rN << std::endl
       << "  thickness sigma - " << t << std::endl;

  ts::data::FeatureVectorSet data(info.str());
  std::random_device dev;
  std::normal_distribution<ts::real> radiusY(rN, t);
  std::normal_distribution<ts::real> radiusN(rY, t);
  std::normal_distribution<ts::real> data_dist(0, 1);

  for (unsigned int i = 0; i < size_positive; i++) {
    ts::data::FeatureVector vec(data_dim);
    for (unsigned int d = 0; d < vec.rows(); d++)
      vec(d) = data_dist(dev);

    vec = (vec/vec.norm()) * radiusY(dev);

    vec.setCategory(ts::msmm::YES);
    data << vec;
  }

  for (unsigned int i = 0; i < size_negative; i++) {
    ts::data::FeatureVector vec(data_dim);
    for (unsigned int d = 0; d < vec.rows(); d++)
      vec(d) = data_dist(dev);
    vec = (vec/vec.norm()) * radiusN(dev);
    vec.setCategory(ts::msmm::NO);
    data << vec;
  }

  return data;
}



void Linear(int argc, char** argv, bool store) {
  po::options_description desc("Generating Linear data set");
  desc.add_options()
    ("size-positive", po::value<unsigned int>(), "positive data size")
    ("size-negative", po::value<unsigned int>(), "negative data size")

    ("linear-dist", po::value<ts::real>(), "distance to origin")
    ("linear-sigma", po::value<ts::real>(), "cloud spread")
    ("linear-data-dim", po::value<unsigned int>(), "dimensionality of the data");
  po::variables_map vm;
  po::command_line_parser parser(argc, argv);
  parser.allow_unregistered().options(desc);

  po::parsed_options parsed = parser.run();
  po::store(parsed, vm);
  po::notify(vm);

  if (!ts::tools::checkArgument(vm, desc, "linear-dist")) return;
  if (!ts::tools::checkArgument(vm, desc, "linear-sigma")) return;
  if (!ts::tools::checkArgument(vm, desc, "linear-data-dim")) return;

  if (!ts::tools::checkArgument(vm, desc, "size-positive")) return;
  if (!ts::tools::checkArgument(vm, desc, "size-negative")) return;

  unsigned int size_positive = vm["size-positive"].as<unsigned int>();
  unsigned int size_negative = vm["size-negative"].as<unsigned int>();

  ts::data::FeatureVectorSet data_set =
    generateLinear(vm["linear-dist"].as<ts::real>(),
                   vm["linear-sigma"].as<ts::real>(),
                   size_positive,
                   size_negative,
                   vm["linear-data-dim"].as<unsigned int>());
  if (store) {
    pqxx::connection conn;
    pqxx::work w(conn);
    std::cout << "Storing data set with id: "
              << data_set.store(w) << std::endl;
    w.commit();

  } else {
    std::map<unsigned short, std::string> legend = {
      {ts::msmm::YES, "YES"},
      {ts::msmm::NO, "NO"}
    };

    ts::plot::featureVectorSet(data_set, legend, 0, 1, 2);
  }

}


void Xor(int argc, char** argv, bool store) {
  po::options_description desc("Generating Xor data set");
  desc.add_options()
    ("size-positive", po::value<unsigned int>(), "positive data size")
    ("size-negative", po::value<unsigned int>(), "negative data size")

    ("xor-dist", po::value<ts::real>(), "distance to origin")
    ("xor-sigma", po::value<ts::real>(), "cloud spread")
    ("xor-data-dim", po::value<unsigned int>(), "dimensionality of the data");

  po::variables_map vm;
  po::command_line_parser parser(argc, argv);
  parser.allow_unregistered().options(desc);

  po::parsed_options parsed = parser.run();
  po::store(parsed, vm);
  po::notify(vm);

  if (!ts::tools::checkArgument(vm, desc, "xor-dist")) return;
  if (!ts::tools::checkArgument(vm, desc, "xor-sigma")) return;
  if (!ts::tools::checkArgument(vm, desc, "xor-data-dim")) return;

  if (!ts::tools::checkArgument(vm, desc, "size-positive")) return;
  if (!ts::tools::checkArgument(vm, desc, "size-negative")) return;

  unsigned int size_positive = vm["size-positive"].as<unsigned int>();
  unsigned int size_negative = vm["size-negative"].as<unsigned int>();

  ts::data::FeatureVectorSet data_set =
    generateXor(vm["xor-dist"].as<ts::real>(),
                vm["xor-sigma"].as<ts::real>(),
                size_positive,
                size_negative,
                vm["xor-data-dim"].as<unsigned int>());
  if (store) {
    pqxx::connection conn;
    pqxx::work w(conn);
    std::cout << "Storing data set with id: "
              << data_set.store(w) << std::endl;
    w.commit();

  } else {
    std::map<unsigned short, std::string> legend = {
      {ts::msmm::YES, "YES"},
      {ts::msmm::NO, "NO"}
    };

    ts::plot::featureVectorSet(data_set, legend, 0, 2, 3);
  }

}


void Circle(int argc, char** argv, bool store) {
  po::options_description desc("Generating Circle data set");
  desc.add_options()
    ("size-positive", po::value<unsigned int>(), "positive data size")
    ("size-negative", po::value<unsigned int>(), "negative data size")

    ("circle-radius-Y", po::value<ts::real>(), "circle radius of class Y")
    ("circle-radius-N", po::value<ts::real>(), "circle radius of class N")
    ("circle-thickness", po::value<ts::real>(), "circle class thickness")
    ("circle-data-dim", po::value<unsigned int>(), "data dimensionality");

  po::variables_map vm;
  po::command_line_parser parser(argc, argv);
  parser.allow_unregistered().options(desc);

  po::parsed_options parsed = parser.run();
  po::store(parsed, vm);
  po::notify(vm);


  if (!ts::tools::checkArgument(vm, desc, "circle-radius-Y")) return;
  if (!ts::tools::checkArgument(vm, desc, "circle-radius-N")) return;
  if (!ts::tools::checkArgument(vm, desc, "circle-thickness")) return;
  if (!ts::tools::checkArgument(vm, desc, "circle-data-dim")) return;

  if (!ts::tools::checkArgument(vm, desc, "size-positive")) return;
  if (!ts::tools::checkArgument(vm, desc, "size-negative")) return;

  unsigned int size_positive = vm["size-positive"].as<unsigned int>();
  unsigned int size_negative = vm["size-negative"].as<unsigned int>();

  ts::data::FeatureVectorSet data_set =
    generateCircle(vm["circle-radius-Y"].as<ts::real>(),
                   vm["circle-radius-N"].as<ts::real>(),
                   vm["circle-thickness"].as<ts::real>(),
                   size_positive,
                   size_negative,
                   vm["circle-data-dim"].as<unsigned int>());

  if (store) {
    pqxx::connection conn;
    pqxx::work w(conn);
    std::cout << "Storing data set with id: "
              << data_set.store(w) << std::endl;
    w.commit();

  } else {
    std::map<unsigned short, std::string> legend = {
      {ts::msmm::YES, "YES"},
      {ts::msmm::NO, "NO"}
    };

    ts::plot::featureVectorSet(data_set, legend, 0, 1, 0);
  }

}



void SequenceLevels(int argc, char** argv, bool store) {

  po::options_description desc("Generate sequence levels");
  desc.add_options()
    ("size-positive", po::value<unsigned int>(), "positive data size")
    ("size-negative", po::value<unsigned int>(), "negative data size")
    ("level-sigma", po::value<ts::real>(),
     "sigma of the noise of level intensity");

  po::variables_map vm;
  po::command_line_parser parser(argc, argv);
  parser.allow_unregistered().options(desc);

  po::parsed_options parsed = parser.run();
  po::store(parsed, vm);
  po::notify(vm);

  if (!ts::tools::checkArgument(vm, desc, "level-sigma")) return;
  if (!ts::tools::checkArgument(vm, desc, "size-positive")) return;
  if (!ts::tools::checkArgument(vm, desc, "size-negative")) return;

  unsigned int size_positive = vm["size-positive"].as<unsigned int>();
  unsigned int size_negative = vm["size-negative"].as<unsigned int>();

  ts::real sigma = vm["level-sigma"].as<ts::real>();
  std::vector<ts::real> level_means = {30, 20, 10};

  std::random_device dev;
  std::vector<std::normal_distribution<ts::real>> dist;
  for (auto& m : level_means)
    dist.push_back(std::normal_distribution<ts::real>(m, sigma));

  unsigned short s_num = level_means.size() + 1;

  ts::Vector class1_pi(s_num);
  class1_pi << 1, 0, 0, 0;

  ts::Matrix class1_tr(s_num, s_num);
  class1_tr <<
    0.99, 0.01,   0,    0,
    0,    0.99,   0, 0.01,
    0,       0,   0,    1,
    0,       0,   0,    1;

  ts::Vector class2_pi(s_num);
  class2_pi << 1, 0, 0, 0;

  ts::Matrix class2_tr(s_num, s_num);
  class2_tr <<
    0.99, 0.01,      0,    0,
    0,    0.99,   0.01,    0,
    0,       0,   0.99, 0.01,
    0,       0,      0,    1;

  ts::simulate::HMM class1_hmm(class1_tr, class1_pi);
  ts::simulate::HMM class2_hmm(class2_tr, class2_pi);

  unsigned short final_state = class1_hmm.states() - 1;


  std::stringstream ss;
  ss << "Simulated sequence set with two classes. " << std::endl
     << "Transition for class 1 " << std::endl
     << class1_tr << std::endl
     << std::endl
     << class1_pi << std::endl
     << std::endl
     << "Emission class1: " << std::endl;
  for (unsigned int i = 0; i < level_means.size(); i++)
    ss << "  state " << i  << " : x ~ N(" << level_means[i]
       <<"," << sigma << ")" << std::endl;


  ss << std::endl

     << "Transition for class 2 " << std::endl
     << class2_tr << std::endl
     << std::endl
     << class2_pi << std::endl
     << std::endl
     << "Emission class2: " << std::endl;
  for (unsigned int i = 0; i < level_means.size(); i++)
    ss << "  state " << i  << " : x ~ N(" << level_means[i]
       <<"," << sigma << ")" << std::endl;

  ss << std::endl
     << "State " << final_state << " means end of sequence" << std::endl;

  ts::data::SequenceSet seq_set;
  seq_set.setInfo(ss.str());
  seq_set.setFrameInfo({"X"});

  for (unsigned int i = 0; i < size_negative; i++) {
    ts::data::Sequence seq;
    class1_hmm.reset();
    unsigned short level = class1_hmm();
    while(level != final_state) {
      ts::Vector v(1);
      v(0) = dist[level](dev);
      seq << v;
      level = class1_hmm();
    }

    seq.setCategory(ts::msmm::NO);
    seq_set << seq;
  }

  for (unsigned int i = 0; i < size_positive; i++) {
    ts::data::Sequence seq;

    class2_hmm.reset();
    unsigned short level = class2_hmm();
    while(level != final_state) {
      ts::Vector v(1);
      v(0) = dist[level](dev);
      seq << v;
      level = class2_hmm();
    }
    seq.setCategory(ts::msmm::YES);
    seq_set << seq;
  }


  if (store) {
    pqxx::connection conn;
    pqxx::work w(conn);
    std::cout << "Storing sequence levels with id: "
              << seq_set.store(w) << std::endl;
    w.commit();
  } else {
    plot(seq_set[0], seq_set[seq_set.size() -1]);
  }

}


void SequenceStepsDirection(int argc, char** argv,  bool store) {
  po::options_description desc("Generate sequence steps direction");
  desc.add_options()
    ("size-positive", po::value<unsigned int>(), "positive data size")
    ("size-negative", po::value<unsigned int>(), "negative data size")
    ("level-sigma", po::value<ts::real>(), "sigma of the noise of level")
    ("level-mean", po::value<ts::real>(), "level mean");

  po::variables_map vm;
  po::command_line_parser parser(argc, argv);
  parser.allow_unregistered().options(desc);

  po::parsed_options parsed = parser.run();
  po::store(parsed, vm);
  po::notify(vm);

  if (!ts::tools::checkArgument(vm, desc, "level-sigma")) return;
  if (!ts::tools::checkArgument(vm, desc, "level-mean")) return;
  if (!ts::tools::checkArgument(vm, desc, "size-positive")) return;
  if (!ts::tools::checkArgument(vm, desc, "size-negative")) return;

  unsigned int size_positive = vm["size-positive"].as<unsigned int>();
  unsigned int size_negative = vm["size-negative"].as<unsigned int>();


  ts::real sigma = vm["level-sigma"].as<ts::real>();
  ts::real level_mean = vm["level-mean"].as<ts::real>();
  std::vector<ts::real> level_means = {-level_mean, level_mean};

  std::random_device dev;
  std::vector<std::normal_distribution<ts::real>> dist;
  for (auto& m : level_means)
    dist.push_back(std::normal_distribution<ts::real>(m, sigma));

  unsigned short s_num = level_means.size() + 1;

  ts::Vector class1_pi(s_num);
  class1_pi << 1, 0, 0;

  ts::Matrix class1_tr(s_num, s_num);
  class1_tr <<
    0.99, 0.01,    0,
       0, 0.99, 0.01,
       0,   0,     1;

  ts::Vector class2_pi(s_num);
  class2_pi << 0, 1, 0;

  ts::Matrix class2_tr(s_num, s_num);
  class2_tr <<
    0.99,   0,   0.01,
    0.01, 0.99,     0,
        0,   0,     1;

  ts::simulate::HMM class1_hmm(class1_tr, class1_pi);
  ts::simulate::HMM class2_hmm(class2_tr, class2_pi);

  unsigned short final_state = class1_hmm.states() - 1;


  std::stringstream ss;
  ss << "Simulated sequence set with two classes. " << std::endl
     << "Transition for class 1 " << std::endl
     << class1_tr << std::endl
     << std::endl
     << class1_pi << std::endl
     << std::endl
     << "Emission class1: " << std::endl;
  for (unsigned int i = 0; i < level_means.size(); i++)
    ss << "  state " << i  << " : x ~ N(" << level_means[i]
       <<"," << sigma << ")" << std::endl;


  ss << std::endl

     << "Transition for class 2 " << std::endl
     << class2_tr << std::endl
     << std::endl
     << class2_pi << std::endl
     << std::endl
     << "Emission class2: " << std::endl;
  for (unsigned int i = 0; i < level_means.size(); i++)
    ss << "  state " << i  << " : x ~ N(" << level_means[i]
       <<"," << sigma << ")" << std::endl;

  ss << std::endl
     << "State " << final_state << " means end of sequence" << std::endl;

  ts::data::SequenceSet seq_set;
  seq_set.setInfo(ss.str());
  seq_set.setFrameInfo({"X"});

  for (unsigned int i = 0; i < size_negative; i++) {
    ts::data::Sequence seq;
    class1_hmm.reset();
    unsigned short level = class1_hmm();
    while(level != final_state) {
      ts::Vector v(1);
      v(0) = dist[level](dev);
      seq << v;
      level = class1_hmm();
    }

    seq.setCategory(ts::msmm::NO);
    seq_set << seq;
  }

  for (unsigned int i = 0; i < size_positive; i++) {
    ts::data::Sequence seq;

    class2_hmm.reset();
    unsigned short level = class2_hmm();
    while(level != final_state) {
      ts::Vector v(1);
      v(0) = dist[level](dev);
      seq << v;
      level = class2_hmm();
    }
    seq.setCategory(ts::msmm::YES);
    seq_set << seq;
  }


  if (store) {
    pqxx::connection conn;
    pqxx::work w(conn);
    std::cout << "Storing sequence levels with id: "
              << seq_set.store(w) << std::endl;
    w.commit();
  } else {
    plot(seq_set[0], seq_set[seq_set.size() -1]);
  }

}


void SequenceMinst(int argc, char** argv, bool store) {
  po::options_description desc("Generating Circle data set");
  desc.add_options()
    ("size-positive", po::value<unsigned int>(), "positive data size")
    ("size-negative", po::value<unsigned int>(), "negative data size")
    ("data-file", po::value<std::string>(), "MINST data file")
    ("label-file", po::value<std::string>(), "MINST labels");

  po::variables_map vm;
  po::command_line_parser parser(argc, argv);
  parser.allow_unregistered().options(desc);

  po::parsed_options parsed = parser.run();
  po::store(parsed, vm);
  po::notify(vm);


  if (!ts::tools::checkArgument(vm, desc, "data-file")) return;
  if (!ts::tools::checkArgument(vm, desc, "label-file")) return;
  if (!ts::tools::checkArgument(vm, desc, "size-positive")) return;
  if (!ts::tools::checkArgument(vm, desc, "size-negative")) return;

  unsigned int size_positive = vm["size-positive"].as<unsigned int>();
  unsigned int size_negative = vm["size-negative"].as<unsigned int>();

  ts::import::Minist minist(vm["data-file"].as<std::string>(),
                            vm["label-file"].as<std::string>());

  ts::data::FeatureVectorSet vec = minist.featureVectorSet();

  std::vector<std::vector<ts::Vector>> visible(10);

  for (const auto& v : vec)
    visible[v.category()].push_back(v);

  std::random_device dev;
  std::vector<std::uniform_int_distribution<unsigned int>> dist;

  dist.push_back(std::uniform_int_distribution<unsigned int>(0, visible[0]
                                                             .size() - 1));
  dist.push_back(std::uniform_int_distribution<unsigned int>(0, visible[1]
                                                             .size() - 1));

  ts::Vector class1_pi(3);
  class1_pi << 1, 0, 0;

  ts::Matrix class1_tr(3, 3);
  class1_tr <<
    0.99, 0.01,    0,
    0,    0.99, 0.01,
    0,       0,    1;

  ts::Vector class2_pi(3);
  class2_pi << 0, 1, 0;

  ts::Matrix class2_tr(3, 3);
  class2_tr <<
    0.99, 0,    0.01,
    0.01,    0.99, 0,
    0,       0,    1;

  ts::simulate::HMM class1_hmm(class1_tr, class1_pi);
  ts::simulate::HMM class2_hmm(class2_tr, class2_pi);

  unsigned short final_state = class1_hmm.states() - 1;


  std::stringstream ss;
  ss << "Simulated sequence set with two classes. " << std::endl
     << "Transition for class 1 " << std::endl
     << class1_tr << std::endl
     << std::endl
     << class1_pi << std::endl
     << std::endl
     << "Emission class1: " << std::endl
     << "  state 0 - handwritten 0" << std::endl
     << "  state 1 - handwritten 1" << std::endl

     << std::endl

     << "Transition for class 2 " << std::endl
     << class2_tr << std::endl
     << std::endl
     << class2_pi << std::endl
     << std::endl
     << "Emission class2: " << std::endl
     << "  state 0 - handwritten 0" << std::endl
     << "  state 1 - handwritten 1" << std::endl


     << std::endl
     << "State " << final_state << " means end of sequence" << std::endl;

  ts::data::SequenceSet seq_set;
  seq_set.setInfo(ss.str());


  for (unsigned int i = 0; i < size_negative; i++) {
    ts::data::Sequence seq;
    class1_hmm.reset();
    unsigned short level = class1_hmm();
    while(level != final_state) {
      seq << visible[level][dist[level](dev)];
      level = class1_hmm();
    }

    seq.setCategory(ts::msmm::NO);
    seq_set << seq;
  }

  for (unsigned int i = 0; i < size_positive; i++) {
    ts::data::Sequence seq;

    class2_hmm.reset();
    unsigned short level = class2_hmm();
    while(level != final_state) {
      seq << visible[level][dist[level](dev)];
      level = class2_hmm();
    }

    seq.setCategory(ts::msmm::YES);
    seq_set << seq;
  }


  if (store) {
    pqxx::connection conn;
    pqxx::work w(conn);

    std::cout << "Storing sequence minst with id: "
              << seq_set.store(w) << std::endl;
    w.commit();

  } else {
    std::cerr << "Error: visualisaiton not available for this data set type"
              << std::endl;
    return;
  }
}


void LevelDetection(bool store) {
  std::vector<ts::real> half;

  std::random_device dev;
  std::normal_distribution<ts::real> data_dist(0, 1);

  unsigned int size = 40;
  // Flat intensity with blinking
  {
    for (unsigned int i = 0; i < size; i++)
      half.push_back(2 + 0.1*data_dist(dev));
  }

  {
    for (unsigned int i = 0; i < 4; i++)
      half.push_back(2 + 0.5*data_dist(dev));
  }

  {
    for (unsigned int i = 0; i < size; i++)
      half.push_back(2 + 0.1*data_dist(dev));
  }

  // Photobleaching step + flat intensity
  {
    for (unsigned int i = 0; i < size; i++)
      half.push_back(1 + 0.1*data_dist(dev));
  }

  // Arch
  {
    ts::real a = - 0.5/std::pow(size, 2);
    ts::real b = 0.5/size;
    for (unsigned int i = 0; i < size; i++)
      half.push_back(a*std::pow(i, 2) + b*i + 2 + 0.1*data_dist(dev));
  }

  // Increasing
  {
    ts::real a = 0.005;
    for (unsigned int i = 0; i < size; i++)
      half.push_back(a*i + 1 + 0.1*data_dist(dev));
  }

  // Large variance in the begining
  {
    for (unsigned int i = 0; i < size/2; i++)
      half.push_back(1 + 0.2*data_dist(dev));

    for (unsigned int i = 0; i < size/2; i++)
      half.push_back(1 + 0.1*data_dist(dev));
  }

  {
    for (unsigned int i = 0; i < size/2; i++)
      half.push_back(2 + 0.4*data_dist(dev));

    for (unsigned int i = 0; i < size/2; i++)
      half.push_back(1 + 0.4*data_dist(dev));
  }

  ts::Vector f(1);
  ts::data::Sequence seq;

  for (int i = 0; i < (int)half.size(); i++) {
    f(0) = half[i];
    seq << f;
  }

  f(0) = 0;
  seq << f;

  for (int i = (int)half.size() - 1; i > -1; i--) {
    f(0) = half[i];
    seq << f;
  }


  plot(seq);
}

int main(int argc, char** argv) {

  po::options_description desc("Creates simulated set. Command line "
                               "options");
  desc.add_options()
    ("help", "produce help message")
    ("type", po::value<std::string>(), "[ circle | xor | sequence-levels | "
     "sequence-MINST | sequence-steps-direction | linear | level-detection ]")
    ("store", "Use to store the data set instead of plotting it");

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

  if (!ts::tools::checkArgument(vm, desc, "type")) return -1;
  std::string type = vm["type"].as<std::string>();

  if (type == "xor")
    Xor(argc, argv, !!vm.count("store"));


  else if (type == "circle")
    Circle(argc, argv, !!vm.count("store"));


  else if (type == "sequence-levels")
    SequenceLevels(argc, argv, !!vm.count("store"));

  else if (type == "sequence-MINST")

    SequenceMinst(argc, argv, !!vm.count("store"));

  else if (type == "sequence-steps-direction")
    SequenceStepsDirection(argc, argv, !!vm.count("store"));

  else if (type == "linear")
    Linear(argc, argv, !!vm.count("store"));

  else if (type == "level-detection")
    LevelDetection(!!vm.count("store"));
  else
    std::cerr << "Error unrecognised data set type " << type << std::endl;


}
