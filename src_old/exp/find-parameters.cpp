#include "exp/experiment.hpp"
#include "objective/ffnn_classify.hpp"
#include "objective/test_function.hpp"
#include "objective/hmm.hpp"
#include "algorithm.hpp"

#include <track-select>
#include <string>
#include <iostream>
#include <stdexcept>
#include <pqxx/pqxx>
#include <algorithm>

namespace ts = track_select;

namespace track_select {
  namespace exp {

    real meanDist(const optimizer::OptimizerReportItem& item) {
      Vector mean = Vector::Zero(item.vector().rows());
      real count = 0;
      for (auto& key_val : item.data()) {
        if (key_val.first.substr(0, 9) == "Particle ") {
          mean += key_val.second;
          count++;
        }
      }

      mean /= count;

      real mean_dist = 0;
      for (auto& key_val : item.data()) {
        if (key_val.first.substr(0, 9) == "Particle ") {
          mean_dist += (key_val.second - mean).norm();
        }
      }

      return mean_dist / count;
    }

    void extractResult2D(const optimizer::OptimizerReport& report,
                         exp::Experiment* exp,
                         unsigned int thread_id) {

      // for (auto& item : report) {
      //   exp->addLog("Fitness", item.fitness(), thread_id);
      //   exp->addLog("Average Distance Mean", meanDist(item), thread_id);
      //   exp->addLog("Mean Fitness", item.property("Mean Fitness"), thread_id);
      //   exp->addLog("Standard Deviation Fitness",
      //               item.property("Standard Deviation Fitness"), thread_id);
      // }


      optimizer::OptimizerReportItem best = report.best();

      exp->addResult("Fitness", best.fitness(), thread_id);
      // for (unsigned int i = 0; i < best.vector().size(); i++)
      //   exp->addLog("Historically Best Particle", best.vector()[i],
      //               thread_id);

    }


    void extractResultCls(const optimizer::OptimizerReport& report,
                          exp::Experiment* exp,
                          unsigned int thread_id) {

      extractResult2D(report, exp, thread_id);
      // for (auto& item : report)
      //   exp->addLog("Accuracy", item.property("Accuracy"), thread_id);

      exp->addResult("Accuracy", report.best().property("Accuracy"), thread_id);
    }


    void ga2D(unsigned int iterations, real sigma_init, real sigma_mutate,
              objective::TestFunction::FitnessType fitness,
              real mutation_scale,
              real mutation_bias,
              std::ostream& log) {

      log << "ga2d\t"
          << "itr: " << iterations << "\t"
          << "init: " << sigma_init << "\t"
          << "mutate: " << sigma_mutate << "\t"
          << "obj: " << fitness << std::endl;

      Experiment e(100);

      e.setInfo("Find good parameters for optimisation algorithm. Applied on 2D"
                " test function.");

      e.addProperty("optimizer", "continuous genetic algorithm")
        .addProperty("optimizer.population", 40)
        .addProperty("optimizer.fitness", fitness)

        .addProperty("optimizer.initialisation-sigma", sigma_init)
        .addProperty("optimizer.mutation-sigma", sigma_mutate)
        .addProperty("optimizer.adaptive-mutation-scale", mutation_scale)
        .addProperty("optimizer.adaptive-mutation-bias", mutation_bias)
        .addProperty("optimizer.iterations", iterations)
        .addProperty("optimizer.vector-space-dim", 2);

      e.run([](Experiment *exp, unsigned int thread_id, std::mutex&) {
          real population;
          objective::TestFunction::FitnessType fitness;
          real initialisation_sigma;
          real mutation_sigma;
          real adaptive_mutation_scale;
          real adaptive_mutation_bias;
          real iterations;



          std::stringstream ss;
          ss << exp->property("optimizer.population") << " "
             << exp->property("optimizer.fitness")  << " "
             << exp->property("optimizer.initialisation-sigma")  << " "
             << exp->property("optimizer.mutation-sigma")  << " "
             << exp->property("optimizer.adaptive-mutation-scale")  << " "
             << exp->property("optimizer.adaptive-mutation-bias")  << " "
             << exp->property("optimizer.iterations");

          ss >> population
             >> fitness
             >> initialisation_sigma
             >> mutation_sigma
             >> adaptive_mutation_scale
             >> adaptive_mutation_bias
             >> iterations;


          objective::TestFunction obj(population, fitness);

          obj.setProperty("initialisation sigma", initialisation_sigma);
          obj.setProperty("mutation sigma", mutation_sigma);
          obj.setProperty("adaptive mutation scale",
                          adaptive_mutation_scale);

          obj.setProperty("adaptive mutation bias",
                          adaptive_mutation_bias);
          obj.setProperty("iterations", iterations);



          optimizer::ContinuousGa ga(obj);
          try {
            ga();
            exp->addResult("broken", 0, thread_id);
          } catch (std::runtime_error e) {
            exp->addResult("broken", 1, thread_id);
          }

          extractResult2D(ga.report(), exp, thread_id);

        });

      pqxx::connection conn("");
      pqxx::work w(conn);

      e.store(w);
      w.commit();
    }

    void pso2D(real sigma_init, real theta, real alpha,
               objective::TestFunction::FitnessType fitness,
               std::ostream& log) {
      log << "pso2D\t"
          << "init: " << sigma_init << "\t"
          << "theta: " << theta << "\t"
          << "alpha: " << alpha << "\t"
          << "obj: " << fitness << std::endl;

      Experiment e(100);

      e.setInfo("Find good parameters for optimisation algorithm. Applied on 2D "
                "test function.");

      e.addProperty("optimizer", "particle swarm optimisation")
        .addProperty("optimizer.population", 40)
        .addProperty("optimizer.fitness", fitness)
        .addProperty("optimizer.initialisation-sigma", sigma_init)
        .addProperty("optimizer.theta", theta)
        .addProperty("optimizer.alpha", alpha)

        .addProperty("optimizer.maximum-iterations", 20000)
        .addProperty("optimizer.convergence-threshold", std::pow(10, -5))

        .addProperty("optimizer.vector-space-dim", 2);

      e.run([](Experiment *exp, unsigned int thread_id, std::mutex&) {
          real population;
          objective::TestFunction::FitnessType fitness;
          real initialisation_sigma;
          real theta;
          real alpha;
          unsigned int maximum_iterations;
          real convergence_threshold;


          std::stringstream ss;
          ss << exp->property("optimizer.population") << " "
             << exp->property("optimizer.fitness")  << " "
             << exp->property("optimizer.initialisation-sigma")  << " "
             << exp->property("optimizer.theta") << " "
             << exp->property("optimizer.alpha") << " "
             << exp->property("optimizer.maximum-iterations") << " "
             << exp->property("optimizer.convergence-threshold");

          ss >> population
             >> fitness
             >> initialisation_sigma
             >> theta
             >> alpha
             >> maximum_iterations
             >> convergence_threshold;

          objective::TestFunction train_obj(population, fitness);

          train_obj.setProperty("initialisation sigma", initialisation_sigma);
          train_obj.setProperty("theta", theta);
          train_obj.setProperty("alpha", alpha);
          train_obj.setProperty("maximum iterations", maximum_iterations);
          train_obj.setProperty("convergence threshold", convergence_threshold);
          train_obj.setProperty("disperse scale", 1); //it's unused.

          optimizer::PSO pso(train_obj);
          try {
            pso();
            exp->addResult("broken", 0, thread_id);
          } catch(std::runtime_error e) {
            exp->addResult("broken", 1, thread_id);
          }
          exp->addResult("Iterations", pso.report().size(), thread_id);
          extractResult2D(pso.report(), exp, thread_id);
        });

      pqxx::connection conn("");
      pqxx::work w(conn);

      e.store(w);
      w.commit();
    }

    void ga_classify_simulated(unsigned int hidden_units,
                               unsigned int data_set_id,
                               unsigned int iterations,
                               real sigma_init,
                               real sigma_mutate,
                               real mutation_scale,
                               real mutation_bias,
                               std::ostream& log) {
      log << "GA Classify Simulated\t"
          << "hu: " << hidden_units << "\t"
          << "data id: " << data_set_id << "\t"
          << "itr: " << iterations << "\t"
          << "init: " << sigma_init << "\t"
          << "mutate: " << sigma_mutate << "\t"
          << "mutate scale: " << mutation_scale << "\t"
          << "mutate bias: " << mutation_bias << std::endl;

      Experiment e(100);

      data::FeatureVectorSet train;
      {
        pqxx::connection conn("");
        pqxx::work w(conn);
        train = data::FeatureVectorSet::read(data_set_id, w);
      }

      e.setInfo("Find good parameters for optimisation algorithm. Applied on "
                "classifier.");
      unsigned int v_dim = objective::FFNNClassify::
        calcParamsCount(train.dim(), hidden_units,
                        objective::calcCategoriesCount(train));

      e.addProperty("optimizer", "continuous genetic algorithm")
        .addProperty("optimizer.population", 40)
        .addProperty("optimizer.fitness", objective::FFNNClassify::ACCURACY)

        .addProperty("optimizer.initialisation-sigma", sigma_init)
        .addProperty("optimizer.mutation-sigma", sigma_mutate)
        .addProperty("optimizer.adaptive-mutation-scale", mutation_scale)
        .addProperty("optimizer.adaptive-mutation-bias", mutation_bias)
        .addProperty("optimizer.iterations", iterations)

        .addProperty("classifier", "feed forward neural network")
        .addProperty("classifier.hidden-units", hidden_units)
        .addProperty("classifier.vector-space-dim", v_dim)
        .addDataSet("Training Set", &train);

      e.run([](Experiment *exp, unsigned int thread_id, std::mutex&) {
          real population;
          objective::FFNNClassify::FitnessType fitness;
          real initialisation_sigma;
          real mutation_sigma;
          real adaptive_mutation_scale;
          real adaptive_mutation_bias;
          real iterations;
          real hu;


          std::stringstream ss;
          ss << exp->property("optimizer.population") << " "
             << exp->property("optimizer.fitness")  << " "
             << exp->property("optimizer.initialisation-sigma")  << " "
             << exp->property("optimizer.mutation-sigma")  << " "
             << exp->property("optimizer.adaptive-mutation-scale")  << " "
             << exp->property("optimizer.adaptive-mutation-bias")  << " "
             << exp->property("optimizer.iterations")  << " "
             << exp->property("classifier.hidden-units")  << " ";

          ss >> population
             >> fitness
             >> initialisation_sigma
             >> mutation_sigma
             >> adaptive_mutation_scale
             >> adaptive_mutation_bias
             >> iterations
             >> hu;

          const data::FeatureVectorSet& train = *(data::FeatureVectorSet*)exp->
            dataSet("Training Set");


          objective::FFNNClassify train_obj(population, hu, fitness, train);

          train_obj.setProperty("initialisation sigma", initialisation_sigma);
          train_obj.setProperty("mutation sigma", mutation_sigma);
          train_obj.setProperty("adaptive mutation scale",
                                adaptive_mutation_scale);

          train_obj.setProperty("adaptive mutation bias",
                                adaptive_mutation_bias);
          train_obj.setProperty("iterations", iterations);



          optimizer::ContinuousGa ga(train_obj);
          try {
            ga();
            exp->addResult("broken", 0, thread_id);
          } catch (std::runtime_error e) {
            exp->addResult("broken", 1, thread_id);
          }

          extractResultCls(ga.report(), exp, thread_id);
        });

      pqxx::connection conn("");
      pqxx::work w(conn);

      e.store(w);
      w.commit();
    }


    void pso_classify_simulated(unsigned int hidden_units,
                                unsigned int data_set_id,
                                real sigma_init,
                                real theta,
                                real alpha,
                                std::ostream& log) {
      log << "PSO classify simulated\t"
          << "hu: " << hidden_units << "\t"
          << "data set id: " << data_set_id << "\t"
          << "init: " << sigma_init << "\t"
          << "theta: " << theta << "\t"
          << "alpha: " << alpha << std::endl;

      Experiment e(100);

      data::FeatureVectorSet train;
      {
        pqxx::connection conn("");
        pqxx::work w(conn);
        train = data::FeatureVectorSet::read(data_set_id, w);
      }

      e.setInfo("Find good parameters for optimisation algorithm. Applied on "
                "classifier.");

      unsigned int v_space = objective::FFNNClassify::
        calcParamsCount(train.dim(), hidden_units,
                        objective::calcCategoriesCount(train));


      e.addProperty("optimizer", "particle swarm optimisation")
        .addProperty("optimizer.population", 40)
        .addProperty("optimizer.fitness", objective::FFNNClassify::ACCURACY)

        .addProperty("optimizer.initialisation-sigma", sigma_init)
        .addProperty("optimizer.theta", theta)
        .addProperty("optimizer.alpha", alpha)
        .addProperty("optimizer.convergence-threshold", std::pow(10, -5))
        .addProperty("optimizer.maximum-iterations", 1000)

        .addProperty("classifier", "feed forward neural network")
        .addProperty("classifier.hidden-units", hidden_units)
        .addProperty("classifier.vector-space-dim", v_space)
        .addDataSet("Training Set", &train);

      e.run([](Experiment *exp, unsigned int thread_id, std::mutex&) {
          real population;
          objective::FFNNClassify::FitnessType fitness;
          real initialisation_sigma;
          real theta;
          real alpha;
          real convergence_threshold;
          real max_iterations;
          real hu;


          std::stringstream ss;
          ss << exp->property("optimizer.population") << " "
             << exp->property("optimizer.fitness")  << " "
             << exp->property("optimizer.initialisation-sigma")  << " "
             << exp->property("optimizer.theta") << " "
             << exp->property("optimizer.alpha") << " "
             << exp->property("optimizer.convergence-threshold") << " "
             << exp->property("optimizer.maximum-iterations") << " "
             << exp->property("classifier.hidden-units")  << " ";

          ss >> population
             >> fitness
             >> initialisation_sigma
             >> theta
             >> alpha
             >> convergence_threshold
             >> max_iterations
             >> hu;

          const data::FeatureVectorSet& train = *(data::FeatureVectorSet*)exp->
            dataSet("Training Set");

          objective::FFNNClassify train_obj(population, hu, fitness, train);


          train_obj.setProperty("initialisation sigma", initialisation_sigma);
          train_obj.setProperty("theta", theta);
          train_obj.setProperty("alpha", alpha);
          train_obj.setProperty("convergence threshold", convergence_threshold);
          train_obj.setProperty("maximum iterations", max_iterations);
          train_obj.setProperty("disperse scale", 1);

          optimizer::PSO pso(train_obj);
          try {
            pso();
            exp->addResult("broken", 0, thread_id);
          } catch (std::runtime_error e) {
            exp->addResult("broken", 1, thread_id);
          }
          exp->addResult("Iterations", pso.report().size(), thread_id);
          extractResultCls(pso.report(), exp, thread_id);
        });

      pqxx::connection conn("");
      pqxx::work w(conn);

      e.store(w);
      w.commit();
    }


    void ga_classify_simulated_hmm(unsigned int hidden_units,
                                   unsigned int hidden_states,
                                   unsigned int data_set_id,
                                   unsigned int iterations,
                                   real sigma_init,
                                   real sigma_mutate) {
      std::cout << "GA HMM: HU " << hidden_units << " HS " << hidden_states
                << " DS " << data_set_id << " IT " << iterations
                << " SI " << sigma_init << " MS " << sigma_mutate << std::endl;

      Experiment e(100);

      data::SequenceSet train;
      {
        pqxx::connection conn("");
        pqxx::work w(conn);
        train = data::SequenceSet::read(data_set_id, w);
      }

      e.setInfo("Find good parameters for optimisation algorithm. Applied on "
                "classifier.");

        e.addProperty("optimizer", "continuous genetic algorithm")
        .addProperty("optimizer.population", 40)
        .addProperty("optimizer.fitness", objective::FFNNClassify::ACCURACY)

        .addProperty("optimizer.initialisation-sigma", sigma_init)
        .addProperty("optimizer.mutation-sigma", sigma_mutate)

        .addProperty("optimizer.adaptive-mutation-scale", 12)
        .addProperty("optimizer.adaptive-mutation-bias", -6)
        .addProperty("optimizer.iterations", iterations)

        .addProperty("classifier", "hidden markov model")
        .addProperty("classifier.hidden-units", hidden_units)
        .addProperty("classifier.hidden-states", hidden_states)
        .addDataSet("Training Set", &train);

      e.run([](Experiment *exp, unsigned int thread_id, std::mutex&) {
          real population;
          objective::FFNNClassify::FitnessType fitness;
          real initialisation_sigma;
          real mutation_sigma;
          real adaptive_mutation_scale;
          real adaptive_mutation_bias;
          real iterations;
          real hu;
          real hs;

          std::stringstream ss;
          ss << exp->property("optimizer.population") << " "
             << exp->property("optimizer.fitness")  << " "
             << exp->property("optimizer.initialisation-sigma")  << " "
             << exp->property("optimizer.mutation-sigma")  << " "
             << exp->property("optimizer.adaptive-mutation-scale")  << " "
             << exp->property("optimizer.adaptive-mutation-bias")  << " "
             << exp->property("optimizer.iterations")  << " "
             << exp->property("classifier.hidden-units")  << " "
             << exp->property("classifier.hidden-states") << " ";

          ss >> population
             >> fitness
             >> initialisation_sigma
             >> mutation_sigma
             >> adaptive_mutation_scale
             >> adaptive_mutation_bias
             >> iterations
             >> hu
             >> hs;

          const data::SequenceSet& train = *(data::SequenceSet*)exp->
            dataSet("Training Set");


          objective::HMM train_obj(population, hu, hs, fitness, train);

          train_obj.setProperty("initialisation sigma", initialisation_sigma);
          train_obj.setProperty("mutation sigma", mutation_sigma);
          train_obj.setProperty("adaptive mutation scale",
                                adaptive_mutation_scale);

          train_obj.setProperty("adaptive mutation bias",
                                adaptive_mutation_bias);
          train_obj.setProperty("iterations", iterations);



          optimizer::ContinuousGa ga(train_obj);
          try {
            ga();
            exp->addResult("broken", 0, thread_id);
          } catch (ErrorNotFinite e) {
            exp->addResult("broken", 2, thread_id);
          } catch (std::runtime_error e) {
            exp->addResult("broken", 1, thread_id);
          }

          extractResultCls(ga.report(), exp, thread_id);

        });

      std::cout << "  Work done. Will store in DB" << std::endl;
      pqxx::connection conn("");
      pqxx::work w(conn);

      e.store(w);
      w.commit();
    }


    void pso_classify_simulated_hmm(unsigned int hidden_units,
                                    unsigned int hidden_states,
                                    unsigned int data_set_id,
                                    real sigma_init,
                                    real theta,
                                    real alpha) {

      std::cout << "PSO HMM: HU " << hidden_units << " HS " << hidden_states
                << " DS " << data_set_id << " SI " << sigma_init << " T "
                << theta << " A " << alpha << std::endl;

      Experiment e(100);

      data::SequenceSet train;
      {
        pqxx::connection conn("");
        pqxx::work w(conn);
        train = data::SequenceSet::read(data_set_id, w);
      }

      e.setInfo("Find good parameters for optimisation algorithm. Applied on "
                "classifier.");

      e.addProperty("optimizer", "particle swarm optimisation")
        .addProperty("optimizer.population", 40)
        .addProperty("optimizer.fitness", objective::FFNNClassify::ACCURACY)

        .addProperty("optimizer.initialisation-sigma", sigma_init)
        .addProperty("optimizer.theta", theta)
        .addProperty("optimizer.alpha", alpha)
        .addProperty("optimizer.convergence-threshold", std::pow(1, -5))

        .addProperty("optimizer.maximum-iterations", 1000)

        .addProperty("classifier", "hidden markov model")
        .addProperty("classifier.hidden-units", hidden_units)
        .addProperty("classifier.hidden-states", hidden_states)
        .addDataSet("Training Set", &train);

      e.run([](Experiment *exp, unsigned int thread_id, std::mutex&) {
          real population;
          objective::FFNNClassify::FitnessType fitness;
          real initialisation_sigma;

          real theta;
          real alpha;
          real tau;

          real max_iterations;
          real hu;
          real hs;

          std::stringstream ss;
          ss << exp->property("optimizer.population") << " "
             << exp->property("optimizer.fitness")  << " "

             << exp->property("optimizer.initialisation-sigma")  << " "
             << exp->property("optimizer.theta") << " "
             << exp->property("optimizer.alpha") << " "

             << exp->property("optimizer.convergence-threshold") << " "
             << exp->property("optimizer.maximum-iterations") << " "

             << exp->property("classifier.hidden-units")  << " "
             << exp->property("classifier.hidden-states") << " ";

          ss >> population
             >> fitness
             >> initialisation_sigma
             >> theta
             >> alpha
             >> tau
             >> max_iterations
             >> hu
             >> hs;

          const data::SequenceSet& train = *(data::SequenceSet*)exp->
            dataSet("Training Set");


          objective::HMM train_obj(population, hu, hs, fitness, train);

          train_obj.setProperty("initialisation sigma", initialisation_sigma);
          train_obj.setProperty("theta", theta);
          train_obj.setProperty("alpha", alpha);
          train_obj.setProperty("convergence threshold", tau);
          train_obj.setProperty("disperse scale", 1); // not used

          train_obj.setProperty("maximum iterations", max_iterations);

          optimizer::PSO ga(train_obj);
          try {
            ga();
            exp->addResult("broken", 0, thread_id);
          } catch (ErrorNotFinite e) {
            exp->addResult("broken", 2, thread_id);
          } catch (std::runtime_error e) {
            exp->addResult("broken", 1, thread_id);
          }

          exp->addResult("Iterations", ga.report().size(), thread_id);
          extractResultCls(ga.report(), exp, thread_id);

        });

      pqxx::connection conn("");
      pqxx::work w(conn);

      e.store(w);
      w.commit();
    }


  }
}


void psoParams(std::function<void (ts::real init,
                                   ts::real theta,
                                   ts::real alpha)> exec);
void gaParams(std::function<void (ts::real init,
                                  ts::real mutate)> exec);

void psoParamsRefined(std::function<void (ts::real init,
                                          ts::real theta,
                                          ts::real alpha)> exec);
void gaParamsRefined(std::function<void (ts::real init,
                                         ts::real mutate)> exec);

int main(int argc, char** argv) {

  // /* =========== PSO on 2D function ============ */
  // psoParams(std::bind(ts::exp::pso2D,
  //                     std::placeholders::_1,
  //                     std::placeholders::_2,
  //                     std::placeholders::_3,
  //                     ts::objective::TestFunction::MICHALEWICZ,
  //                     std::ref(std::cout)));

  // psoParams(std::bind(ts::exp::pso2D,
  //                     std::placeholders::_1,
  //                     std::placeholders::_2,
  //                     std::placeholders::_3,
  //                     ts::objective::TestFunction::SHEKEL_SYMETRIC,
  //                     std::ref(std::cout)));

  // psoParams(std::bind(ts::exp::pso2D,
  //                     std::placeholders::_1,
  //                     std::placeholders::_2,
  //                     std::placeholders::_3,
  //                     ts::objective::TestFunction::SHEKEL_ASYMETRIC,
  //                     std::ref(std::cout)));

  // psoParams(std::bind(ts::exp::pso2D,
  //                     std::placeholders::_1,
  //                     std::placeholders::_2,
  //                     std::placeholders::_3,
  //                     ts::objective::TestFunction::GRIEWANK,
  //                     std::ref(std::cout)));

  // /* ========== CGA on 2d function ============= */

  // gaParams(std::bind(ts::exp::ga2D, 1000,
  //                    std::placeholders::_1,
  //                    std::placeholders::_2,
  //                    ts::objective::TestFunction::MICHALEWICZ,
  //                     2.29,
  //                     0,
  //                    std::ref(std::cout)));

  // gaParams(std::bind(ts::exp::ga2D, 1000,
  //                    std::placeholders::_1,
  //                    std::placeholders::_2,
  //                    ts::objective::TestFunction::SHEKEL_SYMETRIC,
  //                    9.19,
  //                    -4.59,
  //                    std::ref(std::cout)));

  // gaParams(std::bind(ts::exp::ga2D, 1000,
  //                    std::placeholders::_1,
  //                    std::placeholders::_2,
  //                    ts::objective::TestFunction::SHEKEL_ASYMETRIC,
  //                    4.59,
  //                    -4.59,
  //                    std::ref(std::cout)));

  // gaParams(std::bind(ts::exp::ga2D, 1000,
  //                    std::placeholders::_1,
  //                    std::placeholders::_2,
  //                    ts::objective::TestFunction::GRIEWANK,
  //                    4.59,
  //                    -4.59,
  //                    std::ref(std::cout)));

  // /* ============ PSO on classifier =============== */

  // psoParams(std::bind(ts::exp::pso_classify_simulated, 10, 1,
  //                     std::placeholders::_1,
  //                     std::placeholders::_2,
  //                     std::placeholders::_3,
  //                     std::ref(std::cout)));

  // psoParams(std::bind(ts::exp::pso_classify_simulated, 10, 2,
  //                     std::placeholders::_1,
  //                     std::placeholders::_2,
  //                     std::placeholders::_3,
  //                     std::ref(std::cout)));

  // /* =========== CGA on classifier ============== */
  // gaParams(std::bind(ts::exp::ga_classify_simulated, 10, 1, 1000,
  //                     std::placeholders::_1,
  //                     std::placeholders::_2,
  //                     24,
  //                     -18,
  //                     std::ref(std::cout)));

  // gaParams(std::bind(ts::exp::ga_classify_simulated, 10, 2, 1000,
  //                     std::placeholders::_1,
  //                     std::placeholders::_2,
  //                     24,
  //                     -18,
  //                     std::ref(std::cout)));


  /* hidden units, hidden states, dataset id */

  // gaParamsRefined(std::bind(ts::exp::ga_classify_simulated_hmm, 2, 2, 3, 1000,
  //                           std::placeholders::_1,
  //                           std::placeholders::_2));

  // psoParamsRefined(std::bind(ts::exp::pso_classify_simulated_hmm, 2, 2, 3,
  //                            std::placeholders::_1,
  //                            std::placeholders::_2,
  //                            std::placeholders::_3));


  gaParamsRefined(std::bind(ts::exp::ga_classify_simulated_hmm, 20, 2, 4, 1000,
                            std::placeholders::_1,
                            std::placeholders::_2));

  // psoParamsRefined(std::bind(ts::exp::pso_classify_simulated_hmm, 20, 2, 4,
  //                            std::placeholders::_1,
  //                            std::placeholders::_2,
  //                            std::placeholders::_3));

  return 0;
}




void psoParamsRefined(std::function<void (ts::real init,
                                          ts::real theta,
                                          ts::real alpha)> exec) {

  ts::msmm::CustomRange range = ts::optimizer::PSO::alphaRange(0.9);
  std::vector<unsigned int> init = {1, 5, 10, 15, 20};

  for (auto& i : init)
    exec(i, 0.9, range.low() + range.length()/2);

}


void gaParamsRefined(std::function<void (ts::real init,
                                         ts::real mutate)> exec) {

  std::vector<unsigned int> init = {1, 5, 10};
  std::vector<unsigned int> mutate = {1, 2, 5, 10};

  for (auto & i : init)
    for (auto& m : mutate)
      exec(i, m);
}


void psoParams(std::function<void (ts::real init,
                                   ts::real theta,
                                   ts::real alpha)> exec) {

  std::vector<ts::real> thetas = {0, 0.5, 0.9};
  std::vector<ts::real> init_sigmas = {0.1, 0.5, 1, 5, 10, 20};

  for (auto& init : init_sigmas) {
    for (auto& theta : thetas) {

      ts::msmm::CustomRange range = ts::optimizer::PSO::alphaRange(theta);
      ts::real alpha_step = (range.length())/ 5;

      for (ts::real alpha = range.low() + alpha_step;
           alpha < range.high(); alpha += alpha_step) {
        exec(init, theta, alpha);
      }
    }
  }

}


void gaParams(std::function<void (ts::real init,
                                  ts::real mutate)> exec) {
  std::vector<ts::real> sigmas_init = {0.1, 1, 5, 10, 20, 50, 100};
  std::vector<ts::real> sigmas_mitate = {0.1, 1, 5, 10, 20, 50, 100};

  for (auto init : sigmas_init)
    for (auto mutate : sigmas_mitate)
      exec(init, mutate);

}
