#include "exp/experiment.hpp"
#include "data/feature_vector_set.hpp"
#include "algorithm.hpp"
#include "plot.hpp"

#include <track-select>
#include <string>
#include <iostream>
#include <stdexcept>
#include <pqxx/pqxx>
#include <algorithm>

namespace track_select {
  namespace exp {

    void experiment(Experiment *exp, unsigned int thread_id, std::mutex& m) {
      std::string prefix  = "/mnt/data/images/";

      unsigned int rbms_count;
      {
        std::stringstream ss;
        ss << exp->property("rbms.count") << std::endl;
        ss >> rbms_count;
      }

      std::vector<Vector> training_set;

      data::FeatureVectorSet& train
        = (data::FeatureVectorSet&)(*exp->dataSet("Training Set"));
      data::FeatureVectorSet& test
        = (data::FeatureVectorSet&)(*exp->dataSet("Testing Set"));

      for (auto& sample : train)
        training_set.push_back((Vector&)sample);

      algorithm::random_shuffle(training_set.begin(), training_set.end());
      std::vector<nn::FeedForwardNetwork::layer> encoder_layers;
      std::vector<nn::FeedForwardNetwork::layer> decoder_layers;

      for (unsigned int i = 0; i < rbms_count; i++) {
        nn::RBM::ActivationType visible_units_type;
        nn::RBM::ActivationType hidden_units_type;
        unsigned int visible_units_count;
        unsigned int hidden_units_count;

        real learning_rate;
        real velocity;
        real cost;
        unsigned int max_epoch;
        unsigned int k_steps;

        {

          std::stringstream keys;
          keys << "rbm" << i << ".visible-units.type" << std::endl
               << "rbm" << i << ".hidden-units.type" << std::endl
               << "rbm" << i << ".visible-units.count" << std::endl
               << "rbm" << i << ".hidden-units.count" << std::endl

               << "rbm" << i << ".learning-rate" << std::endl
               << "rbm" << i << ".velocity" << std::endl
               << "rbm" << i << ".cost" << std::endl
               << "rbm" << i << ".max-epoch" << std::endl
               << "rbm" << i << ".k-steps" << std::endl;

          std::stringstream ss;


          for (unsigned int j = 0; j < 9; j++) {
            std::string key;
            keys >> key;
            ss << exp->property(key) << std::endl;
          }

          ss >> visible_units_type
             >> hidden_units_type
             >> visible_units_count
             >> hidden_units_count
             >> learning_rate
             >> velocity
             >> cost
             >> max_epoch
             >> k_steps;
        }


        if (visible_units_type == nn::RBM::GAUSSIAN_ACTIVATION &&
            hidden_units_type == nn::RBM::BINARY_ACTIVATION) {

          std::string key;
          real gaussian_activation_sigma;

          {
            std::stringstream ss;
            std::stringstream keys;
            keys << "rbm" << i << ".gaussian-activation-sigma" << std::endl;
            keys >> key;

            ss << exp->property(key) << std::endl;
            ss >> gaussian_activation_sigma;
          }

          nn::RBMGaussianBinary rbm(hidden_units_count, visible_units_count);

          rbm.setLearningRate(learning_rate);
          rbm.setVelocity(velocity);
          rbm.setCost(cost);
          rbm.setMaxEpoch(max_epoch);
          rbm.setKSteps(k_steps);
          rbm.setGaussianActivationSigma(gaussian_activation_sigma);

          /* Train rbm and sample hidden units */

          real error = rbm.train(training_set.begin(), training_set.end());
          for (auto& sample : training_set)
            sample = rbm.hiddenExpectation(sample);

          {
            std::stringstream ss;
            ss << "rbm" << i << ".error";
            exp->addResult(ss.str(), error, thread_id);
          }


          encoder_layers
            .push_back(std::make_tuple(rbm.weights(),
                                       rbm.hiddenBiases(),
                                       nn::FeedForwardNetwork::SIGMOID_ACTIVATION));

          decoder_layers
            .push_back(std::make_tuple(rbm.weights().transpose(),
                                       rbm.visibleBiases(),
                                       nn::FeedForwardNetwork::LINEAR_ACTIVATION));



        } else if (visible_units_type == nn::RBM::SIGMOID_ACTIVATION &&
                   hidden_units_type == nn::RBM::SIGMOID_ACTIVATION) {

          std::stringstream ss;
          std::stringstream keys;
          std::string key;
          real sigmoid_activation_sigma;

          keys << "rbm" << i << ".sigmoid-activation-sigma" << std::endl;
          keys >> key;

          ss << exp->property(key) << std::endl;
          ss >> sigmoid_activation_sigma;

          nn::RBMSigmoid rbm(hidden_units_count, visible_units_count);

          rbm.setLearningRate(learning_rate);
          rbm.setVelocity(velocity);
          rbm.setCost(cost);
          rbm.setMaxEpoch(max_epoch);
          rbm.setKSteps(k_steps);
          rbm.setSigmoidActivationSigma(sigmoid_activation_sigma);

          /* Train rbm and sample hidden units */

          real error = rbm.train(training_set.begin(), training_set.end());
          for (auto& sample : training_set)
            sample = rbm.hiddenExpectation(sample);

          {
            std::stringstream ss;
            ss << "rbm" << i << ".error";
            exp->addResult(ss.str(), error, thread_id);
          }


          encoder_layers
            .push_back(std::make_tuple(rbm.weights(),
                                       rbm.hiddenBiases(),
                                       nn::FeedForwardNetwork::SIGMOID_ACTIVATION));

          decoder_layers
            .push_back(std::make_tuple(rbm.weights().transpose(),
                                       rbm.visibleBiases(),
                                       nn::FeedForwardNetwork::SIGMOID_ACTIVATION));


        } else if (visible_units_type == nn::RBM::SIGMOID_ACTIVATION &&
                   hidden_units_type == nn::RBM::BINARY_ACTIVATION) {

          std::stringstream ss;
          std::stringstream keys;
          std::string key;
          real sigmoid_activation_sigma;

          keys << "rbm" << i << ".sigmoid-activation-sigma" << std::endl;
          keys >> key;

          ss << exp->property(key) << std::endl;
          ss >> sigmoid_activation_sigma;

          nn::RBMSigmoidBinary rbm(hidden_units_count, visible_units_count);

          rbm.setLearningRate(learning_rate);
          rbm.setVelocity(velocity);
          rbm.setCost(cost);
          rbm.setMaxEpoch(max_epoch);
          rbm.setKSteps(k_steps);
          rbm.setSigmoidActivationSigma(sigmoid_activation_sigma);

          /* Train rbm and sample hidden units */

          real error = rbm.train(training_set.begin(), training_set.end());
          for (auto& sample : training_set)
            sample = rbm.hiddenExpectation(sample);

          {
            std::stringstream ss;
            ss << "rbm" << i << ".error";
            exp->addResult(ss.str(), error, thread_id);
          }


          encoder_layers
            .push_back(std::make_tuple(rbm.weights(),
                                       rbm.hiddenBiases(),
                                       nn::FeedForwardNetwork::SIGMOID_ACTIVATION));

          decoder_layers
            .push_back(std::make_tuple(rbm.weights().transpose(),
                                       rbm.visibleBiases(),
                                       nn::FeedForwardNetwork::SIGMOID_ACTIVATION));

        } else {
          std::stringstream error_message;
          error_message << "RBM with "
                        << visible_units_type
                        << " visible units and "
                        << hidden_units_type
                        << " hidden units is not implemented";
          throw ErrorNotImplemented(error_message.str());
        }

      } // endfor

      std::reverse(decoder_layers.begin(), decoder_layers.end());

      nn::FeedForwardNetwork encoder(encoder_layers);
      nn::FeedForwardNetwork decoder(decoder_layers);

      /* Calculate trainign and testing reconstruction error */

      real err;

      err = 0;
      for (auto& point : train)
        err += (point - decoder(encoder(point))).norm();
      err /= train.size();
      exp->addResult("Training Reconstruction Error", err, thread_id);

      err = 0;
      for (auto& point : test)
        err += (point - decoder(encoder(point))).norm();
      err /= test.size();
      exp->addResult("Testing Reconstruction Error", err, thread_id);

      /* Plotting training reconstructions */
      {
        std::vector<std::pair<Vector, Vector>> images;
        std::map<unsigned short, Vector> rec;

        unsigned int index = train.size() - 1;
        while (rec.size() <10 && index) {
          try {
            rec.at(train[index].category());
          } catch (std::out_of_range e) {
            rec[train[index].category()] = (Vector&)train[index];
          }
          index--;
        }

        for (auto& p : rec)
          images.push_back(std::make_pair(p.second, decoder(encoder(p.second))));

        std::string uuid = algorithm::makeUUID();
        plot::image_reconstruction(images, prefix + uuid);
        exp->addPlot("Train Image Reconstruction", uuid, thread_id);
      }


      /* Plotting testing reconstructions */
      {
        std::vector<std::pair<Vector, Vector>> images;
        std::map<unsigned short, Vector> rec;

        unsigned int index = test.size() - 1;
        while (rec.size() <10 && index) {
          try {
            rec.at(test[index].category());
          } catch (std::out_of_range e) {
            rec[test[index].category()] = (Vector&)test[index];
          }
          index--;
        }

        for (auto& p : rec)
          images.push_back(std::make_pair(p.second, decoder(encoder(p.second))));

        std::string uuid = algorithm::makeUUID();
        plot::image_reconstruction(images, prefix + uuid);
        exp->addPlot("Test Image Reconstruction", uuid, thread_id);
      }

    }

  } // exp
} // track_select


namespace ts = track_select;


void setExperiment1(unsigned int train_id, unsigned int test_id) {

  ts::data::FeatureVectorSet train;
  ts::data::FeatureVectorSet test;
  {
    pqxx::connection conn;
    pqxx::work w(conn);
    train = ts::data::FeatureVectorSet::read(train_id, w);
    test = ts::data::FeatureVectorSet::read(test_id, w);
  }

  ts::exp::Experiment e(1);
  e.setInfo("Apply auto-encoder on MINST data set");
  e.addDataSet("Training Set", &train);
  e.addDataSet("Testing Set", &test);

  /* layer 1 */
  e.addProperty("rbms.count", 4)
    .addProperty("rbm0.visible-units.type", ts::nn::RBM::GAUSSIAN_ACTIVATION)
    .addProperty("rbm0.hidden-units.type", ts::nn::RBM::BINARY_ACTIVATION)
    .addProperty("rbm0.visible-units.count", train.dim())
    .addProperty("rbm0.hidden-units.count", 1000)

    .addProperty("rbm0.learning-rate", 0.01)
    .addProperty("rbm0.velocity", 0.9)
    .addProperty("rbm0.cost", 0.0005)
    .addProperty("rbm0.max-epoch", 2000)
    .addProperty("rbm0.k-steps", 1)
    .addProperty("rbm0.gaussian-activation-sigma", 10)


    /* layer 2 */
    .addProperty("rbm1.visible-units.type", ts::nn::RBM::SIGMOID_ACTIVATION)
    .addProperty("rbm1.hidden-units.type", ts::nn::RBM::SIGMOID_ACTIVATION)
    .addProperty("rbm1.visible-units.count", 1000)
    .addProperty("rbm1.hidden-units.count", 500)

    .addProperty("rbm1.learning-rate", 0.1)
    .addProperty("rbm1.velocity", 0.9)
    .addProperty("rbm1.cost", 0.0005)
    .addProperty("rbm1.max-epoch", 2000)
    .addProperty("rbm1.k-steps", 1)
    .addProperty("rbm1.sigmoid-activation-sigma", 0.1)

    /* layer 3 */

    .addProperty("rbm2.visible-units.type", ts::nn::RBM::SIGMOID_ACTIVATION)
    .addProperty("rbm2.hidden-units.type", ts::nn::RBM::SIGMOID_ACTIVATION)
    .addProperty("rbm2.visible-units.count", 500)
    .addProperty("rbm2.hidden-units.count", 250)

    .addProperty("rbm2.learning-rate", 0.1)
    .addProperty("rbm2.velocity", 0.9)
    .addProperty("rbm2.cost", 0.0005)
    .addProperty("rbm2.max-epoch", 2000)
    .addProperty("rbm2.k-steps", 1)
    .addProperty("rbm2.sigmoid-activation-sigma", 0.1)

    /* layer 4 */
    .addProperty("rbm3.visible-units.type", ts::nn::RBM::SIGMOID_ACTIVATION)
    .addProperty("rbm3.hidden-units.type", ts::nn::RBM::SIGMOID_ACTIVATION)
    .addProperty("rbm3.visible-units.count", 250)
    .addProperty("rbm3.hidden-units.count", 30)

    .addProperty("rbm3.learning-rate", 0.1)
    .addProperty("rbm3.velocity", 0.9)
    .addProperty("rbm3.cost", 0.0005)
    .addProperty("rbm3.max-epoch", 2000)
    .addProperty("rbm3.k-steps", 1)
    .addProperty("rbm3.sigmoid-activation-sigma", 0.1);

  e.run(&ts::exp::experiment);

  {
    pqxx::connection conn;
    pqxx::work w(conn);
    e.store(w);
    w.commit();
  }

}


void setExperiment2(unsigned int train_id, unsigned int test_id) {

  ts::data::FeatureVectorSet train;
  ts::data::FeatureVectorSet test;
  {
    pqxx::connection conn;
    pqxx::work w(conn);
    train = ts::data::FeatureVectorSet::read(train_id, w);
    test = ts::data::FeatureVectorSet::read(test_id, w);
  }

  ts::algorithm::mapToRange(train.begin(), train.end(), train.begin());
  ts::algorithm::mapToRange(test.begin(), test.end(), test.begin());

  ts::exp::Experiment e(1);
  e.setInfo("Apply auto-encoder on MINST data set");
  e.addDataSet("Training Set", &train);
  e.addDataSet("Testing Set", &test);

  /* layer 1 */
  e.addProperty("rbms.count", 4)
    .addProperty("rbm0.visible-units.type", ts::nn::RBM::SIGMOID_ACTIVATION)
    .addProperty("rbm0.hidden-units.type", ts::nn::RBM::SIGMOID_ACTIVATION)
    .addProperty("rbm0.visible-units.count", train.dim())
    .addProperty("rbm0.hidden-units.count", 1000)

    .addProperty("rbm0.learning-rate", 0.1)
    .addProperty("rbm0.velocity", 0.9)
    .addProperty("rbm0.cost", 0.0005)
    .addProperty("rbm0.max-epoch", 2000)
    .addProperty("rbm0.k-steps", 1)
    .addProperty("rbm0.sigmoid-activation-sigma", 0.1)


    /* layer 2 */
    .addProperty("rbm1.visible-units.type", ts::nn::RBM::SIGMOID_ACTIVATION)
    .addProperty("rbm1.hidden-units.type", ts::nn::RBM::SIGMOID_ACTIVATION)
    .addProperty("rbm1.visible-units.count", 1000)
    .addProperty("rbm1.hidden-units.count", 500)

    .addProperty("rbm1.learning-rate", 0.1)
    .addProperty("rbm1.velocity", 0.9)
    .addProperty("rbm1.cost", 0.0005)
    .addProperty("rbm1.max-epoch", 2000)
    .addProperty("rbm1.k-steps", 1)
    .addProperty("rbm1.sigmoid-activation-sigma", 0.1)

    /* layer 3 */

    .addProperty("rbm2.visible-units.type", ts::nn::RBM::SIGMOID_ACTIVATION)
    .addProperty("rbm2.hidden-units.type", ts::nn::RBM::SIGMOID_ACTIVATION)
    .addProperty("rbm2.visible-units.count", 500)
    .addProperty("rbm2.hidden-units.count", 250)

    .addProperty("rbm2.learning-rate", 0.1)
    .addProperty("rbm2.velocity", 0.9)
    .addProperty("rbm2.cost", 0.0005)
    .addProperty("rbm2.max-epoch", 2000)
    .addProperty("rbm2.k-steps", 1)
    .addProperty("rbm2.sigmoid-activation-sigma", 0.1)

    /* layer 4 */
    .addProperty("rbm3.visible-units.type", ts::nn::RBM::SIGMOID_ACTIVATION)
    .addProperty("rbm3.hidden-units.type", ts::nn::RBM::SIGMOID_ACTIVATION)
    .addProperty("rbm3.visible-units.count", 250)
    .addProperty("rbm3.hidden-units.count", 30)

    .addProperty("rbm3.learning-rate", 0.1)
    .addProperty("rbm3.velocity", 0.9)
    .addProperty("rbm3.cost", 0.0005)
    .addProperty("rbm3.max-epoch", 2000)
    .addProperty("rbm3.k-steps", 1)
    .addProperty("rbm3.sigmoid-activation-sigma", 0.1);


  e.run(&ts::exp::experiment);

  {
    pqxx::connection conn;
    pqxx::work w(conn);
    e.store(w);
    w.commit();
  }

}



void setExperiment3(unsigned int train_id, unsigned int test_id) {


  ts::data::FeatureVectorSet train;
  ts::data::FeatureVectorSet test;
  {
    pqxx::connection conn;
    pqxx::work w(conn);
    train = ts::data::FeatureVectorSet::read(train_id, w);
    test = ts::data::FeatureVectorSet::read(test_id, w);
  }


  ts::exp::Experiment e(1);
  e.setInfo("Apply auto-encoder on MINST data set");
  e.addDataSet("Training Set", &train);
  e.addDataSet("Testing Set", &test);

  /* layer 1 */
  e.addProperty("rbms.count", 4)
    .addProperty("rbm0.visible-units.type", ts::nn::RBM::GAUSSIAN_ACTIVATION)
    .addProperty("rbm0.hidden-units.type", ts::nn::RBM::BINARY_ACTIVATION)
    .addProperty("rbm0.visible-units.count", train.dim())
    .addProperty("rbm0.hidden-units.count", 1000)

    .addProperty("rbm0.learning-rate", 0.01)
    .addProperty("rbm0.velocity", 0.9)
    .addProperty("rbm0.cost", 0.0005)
    .addProperty("rbm0.max-epoch", 2000)
    .addProperty("rbm0.k-steps", 1)
    .addProperty("rbm0.gaussian-activation-sigma", 10)


    /* layer 2 */
    .addProperty("rbm1.visible-units.type", ts::nn::RBM::SIGMOID_ACTIVATION)
    .addProperty("rbm1.hidden-units.type", ts::nn::RBM::BINARY_ACTIVATION)
    .addProperty("rbm1.visible-units.count", 1000)
    .addProperty("rbm1.hidden-units.count", 500)

    .addProperty("rbm1.learning-rate", 0.1)
    .addProperty("rbm1.velocity", 0.9)
    .addProperty("rbm1.cost", 0.0005)
    .addProperty("rbm1.max-epoch", 2000)
    .addProperty("rbm1.k-steps", 1)
    .addProperty("rbm1.sigmoid-activation-sigma", 0.1)

    /* layer 3 */

    .addProperty("rbm2.visible-units.type", ts::nn::RBM::SIGMOID_ACTIVATION)
    .addProperty("rbm2.hidden-units.type", ts::nn::RBM::BINARY_ACTIVATION)
    .addProperty("rbm2.visible-units.count", 500)
    .addProperty("rbm2.hidden-units.count", 250)

    .addProperty("rbm2.learning-rate", 0.1)
    .addProperty("rbm2.velocity", 0.9)
    .addProperty("rbm2.cost", 0.0005)
    .addProperty("rbm2.max-epoch", 2000)
    .addProperty("rbm2.k-steps", 1)
    .addProperty("rbm2.sigmoid-activation-sigma", 0.1)

    /* layer 4 */
    .addProperty("rbm3.visible-units.type", ts::nn::RBM::SIGMOID_ACTIVATION)
    .addProperty("rbm3.hidden-units.type", ts::nn::RBM::SIGMOID_ACTIVATION)
    .addProperty("rbm3.visible-units.count", 250)
    .addProperty("rbm3.hidden-units.count", 30)

    .addProperty("rbm3.learning-rate", 0.1)
    .addProperty("rbm3.velocity", 0.9)
    .addProperty("rbm3.cost", 0.0005)
    .addProperty("rbm3.max-epoch", 2000)
    .addProperty("rbm3.k-steps", 1)
    .addProperty("rbm3.sigmoid-activation-sigma", 0.1);


  e.run(&ts::exp::experiment);

  {
    pqxx::connection conn;
    pqxx::work w(conn);
    e.store(w);
    w.commit();
  }

}

int main(int argc, char** argv) {
  unsigned int train_id = 7;
  unsigned int test_id = 8;

  std::thread t1(setExperiment1, train_id, test_id);
  std::thread t2(setExperiment2, train_id, test_id);
  std::thread t3(setExperiment3, train_id, test_id);

  t1.join();
  t2.join();
  t3.join();

  return 0;
}
