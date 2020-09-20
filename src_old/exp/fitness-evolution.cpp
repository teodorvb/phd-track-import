#include "exp/experiment.hpp"
#include "objective/ffnn_classify.hpp"
#include "algorithm.hpp"

#include <track-select>
#include <string>
#include <iostream>
#include <stdexcept>
#include <pqxx/pqxx>
#include <algorithm>

using namespace track_select;

namespace track_select {
  namespace exp {
    void fitness_evolution_ga(std::string info,
                              unsigned int hidden_units,
                              unsigned int data_set_id,
                              unsigned int iterations) {
      Experiment e(100);

      data::FeatureVectorSet train;
      {
        pqxx::connection conn("");
        pqxx::work w(conn);
        train = data::FeatureVectorSet::read(data_set_id, w);
      }

      e.setInfo(funcs::splitToLines80(info));

      e.addProperty("optimizer", "continuous genetic algorithm")
        .addProperty("optimizer.population", 40)
        .addProperty("optimizer.fitness", objective::FFNNClassify::ACCURACY)

        .addProperty("optimizer.initialisation-sigma", 0.1)
        .addProperty("optimizer.mutation-sigma", 1)
        .addProperty("optimizer.adaptive-mutation-scale", 12)
        .addProperty("optimizer.adaptive-mutation-bias", -6)
        .addProperty("optimizer.iterations", iterations)

        .addProperty("classifier", "feed forward neural network")
        .addProperty("classifier.hidden-units", hidden_units)
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

          data::FeatureVectorSet train = *(data::FeatureVectorSet*)exp->
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
          ga();

          optimizer::OptimizerReport train_log = ga.report();
          for (auto& item : train_log) {
            exp->addLog("Fitness", item.fitness(), thread_id);
            for (auto& pair : item.properties())
              exp->addLog(pair.first, pair.second, thread_id);
          }

        });

      pqxx::connection conn("");
      pqxx::work w(conn);

      e.store(w);
      w.commit();
    }


    void fitness_evolution_pso(std::string info,
                               unsigned int hidden_units,
                               unsigned int data_set_id,
                               unsigned int iterations) {
      Experiment e(100);

      data::FeatureVectorSet train;
      {
        pqxx::connection conn("");
        pqxx::work w(conn);
        train = data::FeatureVectorSet::read(data_set_id, w);
      }

      e.setInfo(funcs::splitToLines80(info));

      e.addProperty("optimizer", "particle swarm optimisation")
        .addProperty("optimizer.population", 40)
        .addProperty("optimizer.fitness", objective::FFNNClassify::ACCURACY)

        .addProperty("optimizer.initialisation-sigma", 20)
        .addProperty("optimizer.theta", 0.9)
        .addProperty("optimizer.alpha", 1)
        .addProperty("optimizer.beta", 0.6)
        .addProperty("optimizer.convergence-threshold", 0.02)
        .addProperty("optimizer.disperse-scale", 1000)

        .addProperty("optimizer.iterations", iterations)

        .addProperty("classifier", "feed forward neural network")
        .addProperty("classifier.hidden-units", hidden_units)
        .addDataSet("Training Set", &train);

      e.run([](Experiment *exp, unsigned int thread_id, std::mutex&) {
          real population;
          objective::FFNNClassify::FitnessType fitness;

          real initialisation_sigma;
          real theta;
          real alpha;
          real beta;
          real convergence_threshold;
          real disperse_scale;

          real iterations;
          real hu;


          std::stringstream ss;
          ss << exp->property("optimizer.population") << " "
             << exp->property("optimizer.fitness")  << " "

             << exp->property("optimizer.initialisation-sigma")  << " "
             << exp->property("optimizer.theta") << " "
             << exp->property("optimizer.alpha") << " "
             << exp->property("optimizer.beta") << " "
             << exp->property("optimizer.convergence-threshold") << " "
             << exp->property("optimizer.disperse-scale") << " "

             << exp->property("optimizer.iterations")  << " "
             << exp->property("classifier.hidden-units")  << " ";

          ss >> population
             >> fitness

             >> initialisation_sigma
             >> theta
             >> alpha
             >> beta
             >> convergence_threshold
             >> disperse_scale

             >> iterations
             >> hu;

          data::FeatureVectorSet train = *(data::FeatureVectorSet*)exp->
            dataSet("Training Set");


          objective::FFNNClassify train_obj(population, hu, fitness, train);



          train_obj.setProperty("initialisation sigma", initialisation_sigma);
          train_obj.setProperty("theta", theta);
          train_obj.setProperty("alpha", alpha);
          train_obj.setProperty("beta", beta);
          train_obj.setProperty("convergence threshold", convergence_threshold);
          train_obj.setProperty("disperse scale", disperse_scale);
          train_obj.setProperty("iterations", iterations);

          optimizer::PSO pso(train_obj);
          pso();

          optimizer::OptimizerReport train_log = pso.report();
          for (auto& item : train_log) {
            exp->addLog("Fitness", item.fitness(), thread_id);
            for (auto& pair : item.properties())
              exp->addLog(pair.first, pair.second, thread_id);
          }

        });

      pqxx::connection conn("");
      pqxx::work w(conn);

      e.store(w);
      w.commit();
    }


  }
}

int main() {

  std::string info("Records the best fitness for each generation.");

  //  std::cout << "Working on cga" << std::endl;
  //  track_select::exp::fitness_evolution_ga(info, 10, 54, 20000);

  std::cout << "Working on pso" << std::endl;
  track_select::exp::fitness_evolution_pso(info, 10, 54, 20000);


  return 0;
}
