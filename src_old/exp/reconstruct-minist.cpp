#include <iostream>
#include "exp/experiment.hpp"
#include "data/feature_vector_set.hpp"
#include "plot.hpp"
#include "algorithm.hpp"

#define PROCESSORS 20



namespace track_select {

  namespace exp {

    void reconstruct(Experiment* self, unsigned int sample_id, std::mutex&) {
      unsigned int samples_per_thread = 200;

      std::stringstream ss;
      real rbm_learning_rate;
      real rbm_velocity;
      real rbm_cost;
      unsigned int rbm_k_steps;
      unsigned int rbm_max_epoch;

      real fine_tune_learning_rate;
      real fine_tune_velocity;
      real fine_tune_cost;
      unsigned int fine_tune_max_epoch;

      unsigned int rbm_count;


      std::vector<unsigned int> rbm_visible_units_num;
      std::vector<unsigned int> rbm_hidden_units_num;
      std::vector<nn::RBM::ActivationType> rbm_visible_units_type;
      std::vector<nn::RBM::ActivationType> rbm_hidden_units_type;

      ss << self->property("training-algorithm.rbm.learning-rate") << " "
         << self->property("training-algorithm.rbm.velocity") << " "
         << self->property("training-algorithm.rbm.cost") << " "
         << self->property("training-algorithm.rbm.k-steps") << " "
         << self->property("training-algorithm.rbm.max-epoch") << " "

         << self->property("training-algorithm.fine-tuning.learning-rate") << " "
         << self->property("training-algorithm.fine-tuning.velocity") << " "
         << self->property("training-algorithm.fine-tuning.cost") << " "
         << self->property("training-algorithm.fine-tuning.max-epoch") << " "

         << self->property("rbm.num") << " ";

      ss >> rbm_learning_rate
         >> rbm_velocity
         >> rbm_cost
         >> rbm_k_steps
         >> rbm_max_epoch

         >> fine_tune_learning_rate
         >> fine_tune_velocity
         >> fine_tune_cost
         >> fine_tune_max_epoch

         >> rbm_count;

      for (unsigned int i = 0; i < rbm_count; i++) {
        std::string rbm_id = std::to_string(i);
        ss << self->property("rbm" + rbm_id + ".hidden-units.num") << " "
           << self->property("rbm" + rbm_id + ".hidden-units.type") << " "

           << self->property("rbm" + rbm_id + ".visible-units.num") << " "
           << self->property("rbm" + rbm_id + ".visible-units.type") << " ";
      }

      for (unsigned int i = 0; i < rbm_count; i++) {
        real units_num;
        nn::RBM::ActivationType units_type;

        ss >> units_num;
        ss >> units_type;

        rbm_hidden_units_num.push_back(units_num);
        rbm_hidden_units_type.push_back(units_type);

        ss >> units_num;
        ss >> units_type;

        rbm_visible_units_num.push_back(units_num);
        rbm_visible_units_type.push_back(units_type);
      }

      const data::FeatureVectorSet& train =
        *(const data::FeatureVectorSet*)self->dataSet("Training Set");
      const data::FeatureVectorSet& test =
        *(const data::FeatureVectorSet*)self->dataSet("Testing Set");


      std::vector<nn::RBM> rbms;
      for (unsigned int i = 0; i < rbm_hidden_units_type.size(); i++)
        rbms.push_back(nn::RBM(rbm_hidden_units_num[i],
                               rbm_hidden_units_type[i],
                               rbm_visible_units_num[i],
                               rbm_visible_units_type[i]));


      for (auto& rbm : rbms) {
        rbm.setLearningRate(rbm_learning_rate);
        rbm.setCost(rbm_cost);
        rbm.setVelocity(rbm_velocity);
        rbm.setMaxEpoch(rbm_max_epoch);
        rbm.setSamplesPerThread(samples_per_thread);
        rbm.setKSteps(rbm_k_steps);
        rbm.setSigmoidActivationSigma(0); // no randomness
      }

      std::vector<real> errors = nn::rbmTrain(rbms, train.begin(), train.end());

      for (unsigned int i = 0; i < errors.size(); i++)
        self->addResult("Train RBM " + std::to_string(i) + " error",
                        errors[i], sample_id);

      nn::FeedForwardNetwork autoencoder = nn::rbmExtractAutoEncoder(rbms);

      std::vector<std::pair<Vector, Vector>> ac_training_data;
      for (const Vector& item : train)
        ac_training_data.push_back(std::make_pair(item, item));

      std::vector<std::pair<Vector, Vector>> ac_testing_data;
      for (const Vector& item : test)
        ac_testing_data.push_back(std::make_pair(item, item));

      autoencoder.setLearningRate(fine_tune_learning_rate);
      autoencoder.setVelocity(fine_tune_velocity);
      autoencoder.setCost(fine_tune_cost);
      autoencoder.setSamplesPerThread(samples_per_thread);
      autoencoder.setMaxEpoch(fine_tune_max_epoch);
      autoencoder.setError(nn::cross_entropy);
      autoencoder.setDError(nn::d_cross_entropy);
      real sqr_dist;

      // Calculate training error
      sqr_dist = 0.0;
      for (auto& input_target : ac_training_data)
        sqr_dist += nn::dist(input_target.second,
                             autoencoder(input_target.first)).sum();

      sqr_dist /= ac_training_data.size();
      self->addResult("Train Reconstruction Error Before Fine Tuning",
                      sqr_dist, sample_id);

      // Calculate testing error
      sqr_dist = 0.0;
      for (auto& input_target : ac_testing_data)
        sqr_dist += nn::dist(input_target.second,
                             autoencoder(input_target.first)).sum();

      sqr_dist /= ac_testing_data.size();
      self->addResult("Test Reconstruction Error Before Fine Tuning",
                      sqr_dist, sample_id);

      unsigned int plot_data_size = 5;

      // Plot reconstructions from trainig set
      {
        std::vector<std::pair<Vector, Vector>> to_plot;
        for (unsigned int i = 0; i < plot_data_size; i++)
          to_plot.push_back(std::make_pair(ac_training_data[i].first,
                                           autoencoder(ac_training_data[i].first)));

        std::string uuid = algorithm::makeUUID();
        std::stringstream plot_name;
        plot_name << "recontruct-train.sample_id-" << sample_id

                  << ".rbm-max-epoch-" << rbm_max_epoch
                  << ".hidden-unit-type-"
                  << self->property("rbm0.hidden-units.type")
                  << "." << uuid
                  << ".before-fine-tune";
        plot::image_reconstruction(to_plot, plot_name.str());
        self->addPlot("Reconstruct Training Set Before Fine Tune", uuid, sample_id);
      }

      //Plot reconstructions from testing set
      {
        std::vector<std::pair<Vector, Vector>> to_plot;
        for (unsigned int i = 0; i < plot_data_size; i++)
          to_plot.push_back(std::make_pair(ac_testing_data[i].first,
                                           autoencoder(ac_testing_data[i].first)));
        std::string uuid = algorithm::makeUUID();
        std::stringstream plot_name;
        plot_name << "recontruct-test.sample_id-" << sample_id

                  << ".rbm-max-epoch-" << rbm_max_epoch
                  << ".hidden-unit-type-"
                  << self->property("rbm0.hidden-units.type")
                  << "." << uuid
                  << ".before-fine-tune";
        plot::image_reconstruction(to_plot, plot_name.str());
        self->addPlot("Reconstruct Testing Set Before Fine Tune", uuid, sample_id);
      }

      autoencoder.train(ac_training_data.begin(), ac_training_data.end());

      // Calculate training error
      sqr_dist = 0.0;
      for (auto& input_target : ac_training_data)
        sqr_dist += nn::dist(input_target.second,
                             autoencoder(input_target.first)).sum();

      sqr_dist /= ac_training_data.size();
      self->addResult("Train Reconstruction Error After Fine Tuning",
                      sqr_dist, sample_id);


      // Calculate testing error
      sqr_dist = 0.0;
      for (auto& input_target : ac_testing_data)
        sqr_dist += nn::dist(input_target.second,
                             autoencoder(input_target.first)).sum();

      sqr_dist /= ac_testing_data.size();
      self->addResult("Test Reconstruction Error After Fine Tuning",
                      sqr_dist, sample_id);


      // Plot reconstruction from training set
      {
        std::vector<std::pair<Vector, Vector>> to_plot;
        for (unsigned int i = 0; i < plot_data_size; i++)
          to_plot.push_back(std::make_pair(ac_training_data[i].first,
                                           autoencoder(ac_training_data[i].first)));

        std::string uuid = algorithm::makeUUID();
        std::stringstream plot_name;
        plot_name << "reconstruct-train.sample_id-" << sample_id

                  << ".rbm-max-epoch-" << rbm_max_epoch
                  << ".hidden-unit-type-"
                  << self->property("rbm0.hidden-units.type")
                  << "." << uuid
                  << ".epoch-" << rbm_max_epoch <<  ".after-fine-tune";
        plot::image_reconstruction(to_plot, plot_name.str());
        self->addPlot("Reconstruct Training Set After Fine Tune", uuid, sample_id);
      }

      // Plot reconstruction from testing set
      {
        std::vector<std::pair<Vector, Vector>> to_plot;
        for (unsigned int i = 0; i < plot_data_size; i++)
          to_plot.push_back(std::make_pair(ac_testing_data[i].first,
                                           autoencoder(ac_testing_data[i].first)));

        std::string uuid = algorithm::makeUUID();
        std::stringstream plot_name;
        plot_name << "recontruct-test.sample_id-" << sample_id

                  << ".rbm-max-epoch-" << rbm_max_epoch
                  << ".hidden-unit-type-"
                  << self->property("rbm0.hidden-units.type")
                  << "." << uuid
                  << ".after-fine-tune";
        plot::image_reconstruction(to_plot, plot_name.str());
        self->addPlot("Reconstruct Testing Set After Fine Tune", uuid, sample_id);
      }


    }

    void reconstruct_minist_paper(unsigned int max_epoch) {
      Experiment exp(10);
      exp.setInfo("Dimensionality reduction for hand wirtten symbols (MINIST) "
                  "using auto-encoder");
      exp.addProperty("training-algorithm.pre-training", "contrastive-divergence");
      exp.addProperty("training-algorithm.fine-tuning", "error-backpropagation");

      exp.addProperty("training-algorithm.rbm.learning-rate", 0.1);
      exp.addProperty("training-algorithm.rbm.velocity", 0.9);
      exp.addProperty("training-algorithm.rbm.cost", 0.0002);
      exp.addProperty("training-algorithm.rbm.k-steps", 1);
      exp.addProperty("training-algorithm.rbm.max-epoch", max_epoch);

      exp.addProperty("training-algorithm.fine-tuning.learning-rate", 0.0001);
      exp.addProperty("training-algorithm.fine-tuning.velocity", 0.0);
      exp.addProperty("training-algorithm.fine-tuning.cost", 0.0);
      exp.addProperty("training-algorithm.fine-tuning.max-epoch", max_epoch);

      exp.addProperty("rbm.num", 4);

      exp.addProperty("rbm0.visible-units.num", 784);
      exp.addProperty("rbm0.visible-units.type", nn::RBM::SIGMOID_ACTIVATION);

      exp.addProperty("rbm0.hidden-units.num", 1000);
      exp.addProperty("rbm0.hidden-units.type", nn::RBM::BINARY_ACTIVATION);


      exp.addProperty("rbm1.visible-units.num", 1000);
      exp.addProperty("rbm1.visible-units.type", nn::RBM::SIGMOID_ACTIVATION);

      exp.addProperty("rbm1.hidden-units.num", 500);
      exp.addProperty("rbm1.hidden-units.type", nn::RBM::BINARY_ACTIVATION);


      exp.addProperty("rbm2.visible-units.num", 500);
      exp.addProperty("rbm2.visible-units.type", nn::RBM::SIGMOID_ACTIVATION);

      exp.addProperty("rbm2.hidden-units.num", 250);
      exp.addProperty("rbm2.hidden-units.type", nn::RBM::BINARY_ACTIVATION);


      exp.addProperty("rbm3.visible-units.num", 250);
      exp.addProperty("rbm3.visible-units.type", nn::RBM::SIGMOID_ACTIVATION);

      exp.addProperty("rbm3.hidden-units.num", 30);
      exp.addProperty("rbm3.hidden-units.type", nn::RBM::SIGMOID_ACTIVATION);

      data::FeatureVectorSet train;
      data::FeatureVectorSet test;
      {
        pqxx::connection conn("");
        pqxx::work w(conn);
        train = data::FeatureVectorSet::read(139, w);
        test = data::FeatureVectorSet::read(140, w);

        algorithm::mapToRange(train.begin(), train.end(), train.begin(), 0.1, 0.9);
        algorithm::mapToRange(test.begin(), test.end(), test.end(), 0.1, 0.9);
      }

      exp.addDataSet("Training Set", &train);
      exp.addDataSet("Testing Set", &test);

      exp.run(reconstruct);

      pqxx::connection conn("");
      pqxx::work w(conn);
      exp.store(w);
      w.commit();

    }


    void reconstruct_minist_sigmoid(unsigned int max_epoch) {
      Experiment exp(10);
      exp.setInfo("Dimensionality reduction for hand wirtten symbols (MINIST) "
                  "using auto-encoder");
      exp.addProperty("training-algorithm.pre-training", "contrastive-divergence");
      exp.addProperty("training-algorithm.fine-tuning", "error-backpropagation");

      exp.addProperty("training-algorithm.rbm.learning-rate", 0.1);
      exp.addProperty("training-algorithm.rbm.velocity", 0.9);
      exp.addProperty("training-algorithm.rbm.cost", 0.0002);
      exp.addProperty("training-algorithm.rbm.k-steps", 1);
      exp.addProperty("training-algorithm.rbm.max-epoch", max_epoch);

      exp.addProperty("training-algorithm.fine-tuning.learning-rate", 0.0001);
      exp.addProperty("training-algorithm.fine-tuning.velocity", 0.0);
      exp.addProperty("training-algorithm.fine-tuning.cost", 0.0);
      exp.addProperty("training-algorithm.fine-tuning.max-epoch", max_epoch);

      exp.addProperty("rbm.num", 4);

      exp.addProperty("rbm0.visible-units.num", 784);
      exp.addProperty("rbm0.visible-units.type", nn::RBM::SIGMOID_ACTIVATION);

      exp.addProperty("rbm0.hidden-units.num", 1000);
      exp.addProperty("rbm0.hidden-units.type", nn::RBM::SIGMOID_ACTIVATION);


      exp.addProperty("rbm1.visible-units.num", 1000);
      exp.addProperty("rbm1.visible-units.type", nn::RBM::SIGMOID_ACTIVATION);

      exp.addProperty("rbm1.hidden-units.num", 500);
      exp.addProperty("rbm1.hidden-units.type", nn::RBM::SIGMOID_ACTIVATION);


      exp.addProperty("rbm2.visible-units.num", 500);
      exp.addProperty("rbm2.visible-units.type", nn::RBM::SIGMOID_ACTIVATION);

      exp.addProperty("rbm2.hidden-units.num", 250);
      exp.addProperty("rbm2.hidden-units.type", nn::RBM::SIGMOID_ACTIVATION);


      exp.addProperty("rbm3.visible-units.num", 250);
      exp.addProperty("rbm3.visible-units.type", nn::RBM::SIGMOID_ACTIVATION);

      exp.addProperty("rbm3.hidden-units.num", 30);
      exp.addProperty("rbm3.hidden-units.type", nn::RBM::SIGMOID_ACTIVATION);

      data::FeatureVectorSet train;
      data::FeatureVectorSet test;
      {
        pqxx::connection conn("");
        pqxx::work w(conn);
        train = data::FeatureVectorSet::read(139, w);
        test = data::FeatureVectorSet::read(140, w);

        for (Vector& vec : train)
          vec = vec.unaryExpr([](real x) -> real {
              return (x + 1.0) / 260.0;
            });

        for (Vector& vec : test)
          vec = vec.unaryExpr([](real x) -> real {
              return (x + 1.0) / 260.0;
            });

      }

      exp.addDataSet("Training Set", &train);
      exp.addDataSet("Testing Set", &test);

      exp.run(reconstruct);

      pqxx::connection conn("");
      pqxx::work w(conn);
      exp.store(w);
      w.commit();

    }

  }
}

int main() {

  for (unsigned int epoch : {100, 500, 1000, 1500}) {
    std::cout << "Running experiment as it is in the paper (well almost)"
              << " with " << epoch << " epoch" << std::endl;
    track_select::exp::reconstruct_minist_paper(epoch);

    std::cout << "Running experiment with only sigmoid units"
              << " with " << epoch << " epoch" << std::endl;
    track_select::exp::reconstruct_minist_sigmoid(epoch);
  }

  return 0;
}
