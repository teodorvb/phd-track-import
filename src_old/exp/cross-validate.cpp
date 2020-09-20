#include "exp/experiment.hpp"
#include "objective/ffnn_classify.hpp"
#include "objective/hmm.hpp"
#include "objective/common.hpp"
#include "algorithm.hpp"
#include "tools/command_line.hpp"

#include <track-select>
#include <string>
#include <iostream>
#include <stdexcept>
#include <pqxx/pqxx>
#include <algorithm>
#include <boost/program_options.hpp>

using namespace track_select;

namespace track_select {
  namespace exp {

    template<class HMM_type>
    Matrix hmm_est(const data::SequenceSet& data,
                    const HMM_type& yes,  real prior_y,
                    const HMM_type& no, real prior_n) {
      Matrix conf = Matrix::Zero(2, 2);
      for (auto& point : data)
        conf(objective::classify(yes, prior_y, no, prior_n, point),
             point.category())++;

      return conf;
    }

    void cross_validate_discrete_hmm_em(std::string info,
                                        unsigned int hidden_states_y,
                                        unsigned int hidden_states_n,
                                        unsigned int data_set_id) {
      data::SequenceSet train;
      {
        pqxx::connection conn("");
        pqxx::work w(conn);
        train = data::SequenceSet::read(data_set_id, w);
      }

      for (auto& point : train)
        for (auto& frame : point)
          frame(0)--;

      Experiment e(1);
      //      Experiment e(30);

      e.setInfo(funcs::splitToLines80(info));

      e.addProperty("optimizer", "expectation-maximusation")
        .addProperty("validation", "cross-validation")
        .addProperty("validation.folds", 3)
        .addProperty("validation.fitness", objective::FFNNClassify::F1_SCORE)

        .addProperty("classifier", "discrete hmm")
        .addProperty("classifier.priors", "yes")
        .addProperty("classifier.hidden-states-y", hidden_states_y)
        .addProperty("classifier.hidden-states-n", hidden_states_n)

        .addDataSet("Training Set", &train);

      e.run([](Experiment *exp, unsigned int thread_id, std::mutex& mutex) {
          objective::FFNNClassify::FitnessType fitness;
          unsigned int folds;
          unsigned int hidden_states_y;
          unsigned int hidden_states_n;

          std::stringstream ss;
          ss << exp->property("validation.fitness") << " "
             << exp->property("validation.folds") << " "

             << exp->property("classifier.hidden-states-y") << " "
             << exp->property("classifier.hidden-states-n") << " ";

          ss >> fitness
             >> folds
             >> hidden_states_y
             >> hidden_states_n;

          data::SequenceSet data_set = *(data::SequenceSet*)exp->
            dataSet("Training Set");

          auto estimate = [=](const data::SequenceSet& train,
                              const data::SequenceSet& test) {
            real p_yes = 0;
            for (const auto& t : train) p_yes += t.category();
            p_yes /= train.size();

            Vector priors(2);
            priors(msmm::YES) = std::log(p_yes);
            priors(msmm::NO) = std::log(1 - p_yes);
            auto cls = objective::DiscreteHMMtrain(train.begin(), train.end(),
                                                   hidden_states_y,
                                                   hidden_states_n);

            optimizer::OptimizerReportItem result;
            Matrix conf;
            conf = hmm_est(train,
                           cls[msmm::YES], priors(msmm::YES),
                           cls[msmm::NO], priors(msmm::NO));


            result.setProperty("Train Accuracy", conf.trace()/conf.sum());
            result.setProperty("Train Fitness", objective::f1_score(conf));

            conf = hmm_est(test,
                           cls[msmm::YES], priors(msmm::YES),
                           cls[msmm::NO], priors(msmm::NO));

            result.setProperty("Test Accuracy", conf.trace()/conf.sum());
            result.setProperty("Test Fitness", objective::f1_score(conf));

            return result;
          };


          optimizer::OptimizerReportItem result;
          try {
            while(1) {
              try {
                mutex.lock();
                algorithm::random_shuffle(data_set.begin(), data_set.end());
                mutex.unlock();
                result = optimizer::cross_validate(data_set, folds, estimate,
                                                   data::SequenceSet::zero_class_check);
                break;
              } catch (data::ErrorNullClass e) {
                std::cerr << "Warning: cross-validation partition for thread id "
                          << thread_id << " data set id " << data_set.id()
                          << " has zero class. Reshuffling..."
                          << std::endl;
              }
            }

            for (const auto& pair : result.properties())
              exp->addResult(pair.first, pair.second, thread_id);

            exp->addResult("broken", 0, thread_id);

          } catch (ErrorNotFinite e) {
            std::cerr << "Broken 2 " << e.what()
                      << " tid " << thread_id << std::endl;
            exp->addResult("broken", 2, thread_id);
          } catch (std::runtime_error e) {
            std::cerr << "Broken 1 " << e.what()
                      << " tid " << thread_id << std::endl;
            exp->addResult("broken", 1, thread_id);
          }

        });

      std::cout << e << std::endl;
      // pqxx::connection conn("");
      // pqxx::work w(conn);

      // e.store(w);
      // w.commit();
    }


    void cross_validate_ghmm_em(std::string info,
                                unsigned int hidden_states_y,
                                unsigned int hidden_states_n,
                                unsigned int data_set_id) {
      common::Gaussian::covar_fix = 1e-3;
      
      data::SequenceSet train;
      {
        pqxx::connection conn("");
        pqxx::work w(conn);
        train = data::SequenceSet::read(data_set_id, w);
      }

      Experiment e(1);

      e.setInfo(funcs::splitToLines80(info));

      e.addProperty("optimizer", "expectation-maximusation")
        .addProperty("validation", "cross-validation")
        .addProperty("validation.folds", 3)
        .addProperty("validation.fitness", objective::FFNNClassify::F1_SCORE)

        .addProperty("training.gmm.init-sigma", 0.1)
        .addProperty("training.gmm.init-mean-samples", 10)

        .addProperty("classifier", "gaussian hmm")
        .addProperty("classifier.priors", "yes")
        .addProperty("classifier.hidden-states-y", hidden_states_y)
        .addProperty("classifier.hidden-states-n", hidden_states_n)

        .addProperty("covar-fix", "yes")
        .addDataSet("Training Set", &train);

      e.run([](Experiment *exp, unsigned int thread_id, std::mutex& mutex) {
          objective::FFNNClassify::FitnessType fitness;
          unsigned int folds;
          real gmm_init_sigma;
          real gmm_init_mean_samples;
          unsigned int hidden_states_y;
          unsigned int hidden_states_n;

          std::stringstream ss;
          ss << exp->property("validation.fitness") << " "
             << exp->property("validation.folds") << " "

             << exp->property("training.gmm.init-sigma") << " "
             << exp->property("training.gmm.init-mean-samples") << " "

             << exp->property("classifier.hidden-states-y") << " "
             << exp->property("classifier.hidden-states-n") << " ";

          ss >> fitness
             >> folds
             >> gmm_init_sigma
             >> gmm_init_mean_samples
             >> hidden_states_y
             >> hidden_states_n;

          data::SequenceSet data_set = *(data::SequenceSet*)exp->
            dataSet("Training Set");


          auto estimate = [=](const data::SequenceSet& train,
                              const data::SequenceSet& test) {
            real prior_y = 0;
            real prior_n = 0;

            for (auto& s : train)
              if (s.category() == msmm::YES)
                prior_y++;
              else if (s.category() == msmm::NO)
                prior_n++;
              else {
                std::stringstream ss;
                ss << "Unknonw category " << s.category();
                throw std::runtime_error(ss.str());
              } // endif

            prior_y = std::log(prior_y / train.size());
            prior_n = std::log(prior_n / train.size());

            auto cls = objective::gHMMtrain(train.begin(), train.end(),
            {hidden_states_y, hidden_states_n},
                                      gmm_init_sigma,
                                      gmm_init_mean_samples);

            optimizer::OptimizerReportItem result;
            Matrix conf;

            conf = hmm_est(train, cls[msmm::YES], prior_y, cls[msmm::NO], prior_n);
	    std::cout << conf << std::endl;

            result.setProperty("Train Accuracy", conf.trace()/conf.sum());
            result.setProperty("Train Fitness", objective::f1_score(conf));

            conf = hmm_est(test, cls[msmm::YES], prior_y, cls[msmm::NO], prior_n);

	    std::cout << conf << std::endl << " ----- ";

            result.setProperty("Test Accuracy", conf.trace()/conf.sum());
            result.setProperty("Test Fitness", objective::f1_score(conf));

            return result;
          };


          optimizer::OptimizerReportItem result;
          try {
            while(1) {
              try {
                mutex.lock();
                algorithm::random_shuffle(data_set.begin(), data_set.end());
                mutex.unlock();
                result = optimizer::cross_validate(data_set, folds, estimate,
                                                   data::SequenceSet::zero_class_check);
                break;
              } catch (data::ErrorNullClass e) {
                std::cerr << "Warning: cross-validation partition for thread id "
                          << thread_id << " data set id " << data_set.id()
                          << " has zero class. Reshuffling..."
                          << std::endl;
              }
            }

            for (const auto& pair : result.properties())
              exp->addResult(pair.first, pair.second, thread_id);

            exp->addResult("broken", 0, thread_id);

          } catch (ErrorNotFinite e) {
            std::cerr << "Broken 2 " << e.what()
                      << " tid " << thread_id << std::endl;
            exp->addResult("broken", 2, thread_id);
          } catch (std::runtime_error e) {
            std::cerr << "Broken 1 " << e.what()
                      << " tid " << thread_id << std::endl;
            exp->addResult("broken", 1, thread_id);
          }

        });


      pqxx::connection conn("");
      pqxx::work w(conn);

      // e.store(w);
      // w.commit();

      std::cout << e << std::endl;
    }



    void cross_validate_hmm_ga(std::string info,
                               unsigned int hidden_units,
                               unsigned int hidden_states,
                               unsigned int data_set_id,
                               real init_sigma,
                               real mutate_sigma,
                               unsigned int iterations) {
      std::cout << "Running ga on hmm"
                << " HU " << hidden_units
                << " HS " << hidden_states
                << " ID " << data_set_id
                << " SI " << init_sigma
                << " SM " << mutate_sigma
                << " iter " << iterations << std::endl;

      Experiment e(30);

      data::SequenceSet train;
      {
        pqxx::connection conn("");
        pqxx::work w(conn);
        train = data::SequenceSet::read(data_set_id, w);
      }

      e.setInfo(funcs::splitToLines80(info));

      e.addProperty("optimizer", "continuous genetic algorithm")
        .addProperty("optimizer.population", 40)
        .addProperty("optimizer.fitness", objective::FFNNClassify::F1_SCORE)

        .addProperty("optimizer.initialisation-sigma", init_sigma)
        .addProperty("optimizer.mutation-sigma", mutate_sigma)
        .addProperty("optimizer.adaptive-mutation-scale", 12)
        .addProperty("optimizer.adaptive-mutation-bias", -6)
        .addProperty("optimizer.iterations", iterations)

        .addProperty("validation", "cross-validation")
        .addProperty("validation.folds", 3)

        .addProperty("classifier", "nn hmm")
        .addProperty("classifier.hidden-units", hidden_units)
        .addProperty("classifier.hidden-states", hidden_states)
        .addProperty("classifier.class-biases", "uniform")
        .addDataSet("Training Set", &train);

      e.run([](Experiment *exp, unsigned int thread_id, std::mutex& mutex) {
          real population;
          objective::FFNNClassify::FitnessType fitness;
          real initialisation_sigma;
          real mutation_sigma;
          real adaptive_mutation_scale;
          real adaptive_mutation_bias;
          real iterations;
          real folds;
          unsigned int hu;
          unsigned int hs;

          std::stringstream ss;
          ss << exp->property("optimizer.population") << " "
             << exp->property("optimizer.fitness")  << " "
             << exp->property("optimizer.initialisation-sigma")  << " "
             << exp->property("optimizer.mutation-sigma")  << " "
             << exp->property("optimizer.adaptive-mutation-scale")  << " "
             << exp->property("optimizer.adaptive-mutation-bias")  << " "
             << exp->property("optimizer.iterations")  << " "
             << exp->property("validation.folds")  << " "
             << exp->property("classifier.hidden-units")  << " "
             << exp->property("classifier.hidden-states") << " ";

          ss >> population
             >> fitness
             >> initialisation_sigma
             >> mutation_sigma
             >> adaptive_mutation_scale
             >> adaptive_mutation_bias
             >> iterations
             >> folds
             >> hu
             >> hs;

          data::SequenceSet data_set = *(data::SequenceSet*)exp->
            dataSet("Training Set");


          auto estimate = [=](const data::SequenceSet& train,
                              const data::SequenceSet& test) {

            objective::HMM train_obj(population, hu, hs, fitness, train);

            train_obj.setProperty("initialisation sigma", initialisation_sigma);
            train_obj.setProperty("mutation sigma", mutation_sigma);
            train_obj.setProperty("adaptive mutation scale",
                                  adaptive_mutation_scale);

            train_obj.setProperty("adaptive mutation bias",
                                  adaptive_mutation_bias);
            train_obj.setProperty("iterations", iterations);


            optimizer::ContinuousGa ga(train_obj);
            ga();

            objective::HMM test_obj(population, hu, hs, fitness, test);
            optimizer::OptimizerReportItem result;

            optimizer::OptimizerReportItem train_result = ga.report().best();
            result.setProperty("Train Fitness",
                               train_result.fitness());
            result.setProperty("Train Accuracy",
                               train_result.property("Accuracy"));

            optimizer::OptimizerReportItem test_result
            = test_obj(train_result.vector());
            result.setProperty("Test Fitness",
                               test_result.fitness());
            result.setProperty("Test Accuracy",
                               test_result.property("Accuracy"));

            return result;
          };


          optimizer::OptimizerReportItem result;
          try {
            while(1) {
              try {
                mutex.lock();
                algorithm::random_shuffle(data_set.begin(), data_set.end());
                mutex.unlock();
                result = optimizer::cross_validate(data_set, folds, estimate,
                                                   data::SequenceSet::zero_class_check);
                break;
              } catch (data::ErrorNullClass e) {
                std::cerr << "Warning: cross-validation partition for thread id "
                          << thread_id << " data set id " << data_set.id()
                          << " has zero class. Reshuffling..."
                          << std::endl;
              }
            }

            for (const auto& pair : result.properties())
              exp->addResult(pair.first, pair.second, thread_id);

            exp->addResult("broken", 0, thread_id);

          } catch (ErrorNotFinite e) {
            std::cerr << "Broken 2 " << e.what()
                      << " tid " << thread_id << std::endl;
            exp->addResult("broken", 2, thread_id);
          } catch (std::runtime_error e) {
            std::cerr << "Broken 1 " << e.what()
                      << " tid " << thread_id << std::endl;
            exp->addResult("broken", 1, thread_id);
          }

        });

      pqxx::connection conn("");
      pqxx::work w(conn);

      e.store(w);
      w.commit();
    }



    void cross_validate_ga(std::string info,
                           unsigned int hidden_units,
                           unsigned int data_set_id,
                           real init_sigma,
                           real mutate_sigma,
                           unsigned int iterations) {

      Experiment e(30);

      data::FeatureVectorSet train;
      {
        pqxx::connection conn("");
        pqxx::work w(conn);
        train = data::FeatureVectorSet::read(data_set_id, w);
      }
      e.setInfo(funcs::splitToLines80(info));

      e.addProperty("optimizer", "continuous genetic algorithm")
        .addProperty("optimizer.population", 40)
        .addProperty("optimizer.fitness", objective::FFNNClassify::F1_SCORE)

        .addProperty("optimizer.initialisation-sigma", init_sigma)
        .addProperty("optimizer.mutation-sigma", mutate_sigma)
        .addProperty("optimizer.adaptive-mutation-scale", 12)
        .addProperty("optimizer.adaptive-mutation-bias", -6)
        .addProperty("optimizer.iterations", iterations)

        .addProperty("validation", "cross-validation")
        .addProperty("validation.folds", 3)

        .addProperty("classifier", "feed forward neural network")
        .addProperty("classifier.hidden-units", hidden_units)
        .addDataSet("Training Set", &train);

      e.run([](Experiment *exp, unsigned int thread_id, std::mutex& mutex) {
          real population;
          objective::FFNNClassify::FitnessType fitness;
          real initialisation_sigma;
          real mutation_sigma;
          real adaptive_mutation_scale;
          real adaptive_mutation_bias;
          real iterations;
          real folds;
          real hu;


          std::stringstream ss;
          ss << exp->property("optimizer.population") << " "
             << exp->property("optimizer.fitness")  << " "
             << exp->property("optimizer.initialisation-sigma")  << " "
             << exp->property("optimizer.mutation-sigma")  << " "
             << exp->property("optimizer.adaptive-mutation-scale")  << " "
             << exp->property("optimizer.adaptive-mutation-bias")  << " "
             << exp->property("optimizer.iterations")  << " "
             << exp->property("validation.folds")  << " "
             << exp->property("classifier.hidden-units")  << " ";

          ss >> population
             >> fitness
             >> initialisation_sigma
             >> mutation_sigma
             >> adaptive_mutation_scale
             >> adaptive_mutation_bias
             >> iterations
             >> folds
             >> hu;

          data::FeatureVectorSet data_set = *(data::FeatureVectorSet*)exp->
            dataSet("Training Set");


          auto estimate = [=](const data::FeatureVectorSet& train,
                              const data::FeatureVectorSet& test) {

            objective::FFNNClassify train_obj(population, hu, fitness, train);

            train_obj.setProperty("initialisation sigma", initialisation_sigma);
            train_obj.setProperty("mutation sigma", mutation_sigma);
            train_obj.setProperty("adaptive mutation scale",
                                  adaptive_mutation_scale);

            train_obj.setProperty("adaptive mutation bias",
                                  adaptive_mutation_bias);
            train_obj.setProperty("iterations", iterations);



            optimizer::ContinuousGa ga(train_obj);
            ga();

            objective::FFNNClassify test_obj(population, hu, fitness, test);
            optimizer::OptimizerReportItem result;

            optimizer::OptimizerReportItem train_result = ga.report().best();
            result.setProperty("Train Fitness",
                               train_result.fitness());
            result.setProperty("Train Accuracy",
                               train_result.property("Accuracy"));

            optimizer::OptimizerReportItem test_result
            = test_obj(train_result.vector());
            result.setProperty("Test Fitness",
                               test_result.fitness());
            result.setProperty("Test Accuracy",
                               test_result.property("Accuracy"));

            return result;
          };


          optimizer::OptimizerReportItem result;
          try {
            while(1) {
              try {
                mutex.lock();
                algorithm::random_shuffle(data_set.begin(), data_set.end());
                mutex.unlock();
                result = optimizer::cross_validate(data_set, folds, estimate,
                                                   data::FeatureVectorSet::zero_class_check);
                break;
              } catch (data::ErrorNullClass e) {
                std::cerr << "Warning: cross-validation partition for thread id "
                          << thread_id << " data set id " << data_set.id()
                          << " has zero class. Reshuffling..."
                          << std::endl;
              }
            }

            for (const auto& pair : result.properties())
              exp->addResult(pair.first, pair.second, thread_id);

            exp->addResult("broken", 0, thread_id);

          } catch (ErrorNotFinite e) {
            std::cerr << "Broken 2 " << e.what()
                      << " tid " << thread_id << std::endl;
            exp->addResult("broken", 2, thread_id);
          } catch (std::runtime_error e) {
            std::cerr << "Broken 1 " << e.what()
                      << " tid " << thread_id << std::endl;
            exp->addResult("broken", 1, thread_id);
          }

        });

      pqxx::connection conn("");
      pqxx::work w(conn);

      e.store(w);
      w.commit();
    }



    void cross_validate_pso(std::string info,
                            unsigned int hidden_units,
                            unsigned int data_set_id,
                            real init_sigma,
                            unsigned int iterations) {
      Experiment e(30);
      data::FeatureVectorSet train;
      {
        pqxx::connection conn("");
        pqxx::work w(conn);
        train = data::FeatureVectorSet::read(data_set_id, w);
      }
      real theta = 0.9;
      msmm::CustomRange range = optimizer::PSO::alphaRange(theta);
      real alpha = range.low() + range.length()/2;

      e.setInfo(funcs::splitToLines80(info));

      e.addProperty("optimizer", "particle swarm optimisation")
        .addProperty("optimizer.population", 40)
        .addProperty("optimizer.fitness", objective::FFNNClassify::F1_SCORE)

        .addProperty("optimizer.initialisation-sigma", init_sigma)
        .addProperty("optimizer.theta", theta)
        .addProperty("optimizer.alpha", alpha)
        .addProperty("optimizer.convergence-threshold", 0.02)

        .addProperty("optimizer.maximum-iterations", iterations)
        .addProperty("validation", "cross-validation")
        .addProperty("validation.folds", 3)

        .addProperty("classifier", "feed forward neural network")
        .addProperty("classifier.hidden-units", hidden_units)
        .addDataSet("Training Set", &train);

      e.run([](Experiment *exp, unsigned int thread_id, std::mutex& mutex) {
          real population;
          objective::FFNNClassify::FitnessType fitness;

          real initialisation_sigma;
          real theta;
          real alpha;
          real tau;

          real max_iterations;
          real folds;
          real hu;


          std::stringstream ss;
          ss << exp->property("optimizer.population") << " "
             << exp->property("optimizer.fitness")  << " "

             << exp->property("optimizer.initialisation-sigma")  << " "
             << exp->property("optimizer.theta") << " "
             << exp->property("optimizer.alpha") << " "
             << exp->property("optimizer.convergence-threshold") << " "

             << exp->property("optimizer.maximum-iterations")  << " "
             << exp->property("validation.folds")  << " "
             << exp->property("classifier.hidden-units")  << " ";

          ss >> population
             >> fitness

             >> initialisation_sigma
             >> theta
             >> alpha
             >> tau

             >> max_iterations
             >> folds
             >> hu;

          data::FeatureVectorSet data_set = *(data::FeatureVectorSet*)exp->
            dataSet("Training Set");


          auto estimate = [=](const data::FeatureVectorSet& train,
                              const data::FeatureVectorSet& test) {

            objective::FFNNClassify train_obj(population, hu, fitness, train);

            train_obj.setProperty("initialisation sigma", initialisation_sigma);
            train_obj.setProperty("theta", theta);
            train_obj.setProperty("alpha", alpha);
            train_obj.setProperty("convergence threshold", tau);
            train_obj.setProperty("disperse scale", 1); // not used

            train_obj.setProperty("maximum iterations", max_iterations);

            optimizer::PSO pso(train_obj);
            pso();

            objective::FFNNClassify test_obj(population, hu, fitness, test);
            optimizer::OptimizerReportItem train_result = pso.report().best();

            optimizer::OptimizerReportItem result;

            result.setProperty("Train Fitness",
                               train_result.fitness());
            result.setProperty("Train Accuracy",
                               train_result.property("Accuracy"));

            optimizer::OptimizerReportItem test_result =
              test_obj(train_result.vector());

            result.setProperty("Test Fitness",
                               test_result.fitness());
            result.setProperty("Test Accuracy",
                               test_result.property("Accuracy"));

            result.setProperty("Iterations", pso.report().size());
            return result;
          };


          optimizer::OptimizerReportItem result;

          try {
            while(1) {
              try {
                mutex.lock();
                algorithm::random_shuffle(data_set.begin(), data_set.end());
                mutex.unlock();

                result = optimizer::cross_validate(data_set, folds, estimate,
                                                   data::FeatureVectorSet::zero_class_check);
                break;
              } catch (data::ErrorNullClass e) {
                std::cerr << "Warning: cross-validation partition for thread id "
                          << thread_id << " data set id " << data_set.id()
                          << " has zero class. Reshuffling..."
                          << std::endl;
              }
            }

            for (const auto& pair : result.properties())
              exp->addResult(pair.first, pair.second, thread_id);

            exp->addResult("broken", 0, thread_id);

          } catch (ErrorNotFinite e) {
            std::cerr << "Broken 2 " << e.what()
                      << " tid " << thread_id << std::endl;
            exp->addResult("broken", 2, thread_id);
          } catch (std::runtime_error e) {
            std::cerr << "Broken 1 " << e.what()
                      << " tid " << thread_id << std::endl;
            exp->addResult("broken", 1, thread_id);
          }

        });

      pqxx::connection conn("");
      pqxx::work w(conn);

      e.store(w);
      w.commit();
    }


  }
}

namespace ts =  track_select;


namespace po = boost::program_options;
int main(int argc, char** argv) {

  unsigned int id;
  std::string type;
  {
    po::options_description desc("Train classifier with cross-validation.");
    desc.add_options()
      ("help", "produce help message")
      ("type", po::value<std::string>(),
       "Process sequences or vectors. < ga-hmm | nn | ghmm | discrete-hmm >")
      ("data-set-id", po::value<unsigned int>(), "data set id");

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

    if (!ts::tools::checkArgument(vm, desc,"data-set-id")) return -1;
    if (!ts::tools::checkArgument(vm, desc, "type")) return -1;

    id = vm["data-set-id"].as<unsigned int>();
    type = vm["type"].as<std::string>();
  }


  std::string info = "Cross-validates the accuracy of a classifier on verified "
    "data set.";


  if (type == "nn") {
    std::vector<unsigned int> hidden;
    for (unsigned int i = 1; i < 11; i++)
      hidden.push_back(i);

    for (auto& hu : hidden) {
      std::cout << "cga HU " << hu << std::endl;
      ts::exp::cross_validate_ga(info, hu, id, 5, 10, 5000);
    }


  } else if (type == "ga-hmm") {
    po::options_description desc("Use Hidden Markov Model.");
    desc.add_options()
      ("hidden-states", po::value<unsigned int>(), "Number of hidden states");

    po::variables_map vm;
    po::command_line_parser parser(argc, argv);
    parser.allow_unregistered().options(desc);

    po::store(parser.run(), vm);
    po::notify(vm);

    if (!ts::tools::checkArgument(vm, desc, "hidden-states")) return -1;

    unsigned int hs = vm["hidden-states"].as<unsigned int>();
    ts::exp::cross_validate_hmm_ga(info, 4, hs, id, 5, 10, 5000);

  } else if (type == "ghmm") {

    po::options_description desc("Use Gaussian Emission Hidden Markov Model.");
    desc.add_options()
      ("hidden-states-y", po::value<unsigned int>(),
       "Number of hidden states for class y")

      ("hidden-states-n", po::value<unsigned int>(),
       "Number of hidden states for class n");

    po::variables_map vm;
    po::command_line_parser parser(argc, argv);
    parser.allow_unregistered().options(desc);

    po::store(parser.run(), vm);
    po::notify(vm);

    if (!ts::tools::checkArgument(vm, desc, "hidden-states-y")) return -1;
    if (!ts::tools::checkArgument(vm, desc, "hidden-states-n")) return -1;

    unsigned int hu_y = vm["hidden-states-y"].as<unsigned int>();
    unsigned int hu_n = vm["hidden-states-n"].as<unsigned int>();

    ts::exp::cross_validate_ghmm_em(info, hu_y, hu_n, id);

  } else if (type == "discrete-hmm") {

    po::options_description desc("Use Discrete Hidden Markov Model.");
    desc.add_options()
      ("hidden-states-y", po::value<unsigned int>(),
       "Number of hidden states for class y")

      ("hidden-states-n", po::value<unsigned int>(),
       "Number of hidden states for class n");

    po::variables_map vm;
    po::command_line_parser parser(argc, argv);
    parser.allow_unregistered().options(desc);

    po::store(parser.run(), vm);
    po::notify(vm);

    if (!ts::tools::checkArgument(vm, desc, "hidden-states-y")) return -1;
    if (!ts::tools::checkArgument(vm, desc, "hidden-states-n")) return -1;

    unsigned int hu_y = vm["hidden-states-y"].as<unsigned int>();
    unsigned int hu_n = vm["hidden-states-n"].as<unsigned int>();

    ts::exp::cross_validate_discrete_hmm_em(info, hu_y, hu_n, id);
  }

  return 0;
}
