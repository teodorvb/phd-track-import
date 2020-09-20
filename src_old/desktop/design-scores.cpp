#include <track-select>
#include "data/feature_vector_set.hpp"
#include "data/sequence_set.hpp"
#include <fstream>
#include <iostream>
#include <string>
#include <boost/tokenizer.hpp>
#include "plot.hpp"
#include <iomanip>
#include "tools/timer.hpp"
#include "algorithm.hpp"
#include "objective/common.hpp"
#include <set>

namespace ts = track_select;
/* columns are data */
ts::Matrix corr(ts::Matrix& data) {
  ts::Matrix centered = data.colwise() - data.rowwise().mean();
  ts::Matrix covar = (centered * centered.transpose()) / (data.cols() -1);

  ts::Matrix corr = covar.cwiseProduct((covar.diagonal() *
                                        covar.diagonal().transpose())
                                       .cwiseSqrt().cwiseInverse());
  std::cout << corr << std::endl << std::endl;

  return corr;

}

template<class inputIt>
ts::Vector calcFrame(inputIt begin, inputIt end) {
  ts::Vector frame(8);
  ts::real size = end - begin;

  // Intensity mean
  ts::real mean = 0;
  for (auto it = begin; it != end; it++)
    mean += (*it)(0);
  mean /= size;

  // Intensity standard deviation
  ts::real stdev = 0;
  for (auto it = begin; it != end; it++)
    stdev += std::pow((*it)(0) - mean, 2);

  // Intensity auto-correlation
  ts::real ac = 0;
  for (auto it = begin; it != end - 1; it++)
    ac += ((*it)(0) - mean)*((*(it + 1))(0) - mean);
  ac /= stdev;

  stdev /= size;

  frame(0) = mean;
  frame(1) = std::sqrt(stdev);
  frame(2) = ac;

  // velocities
  std::vector<ts::real> velocities;
  for (unsigned int i = 0; i < size -1; i++) {
    ts::Vector v(2);
    v(0) = (*(begin + i + 1))(1) - (*(begin + i))(1);
    v(1) = (*(begin + i + 1))(2) - (*(begin + i))(2);
    velocities.push_back((v*10).norm());
  }

  // velocity mean
  ts::real v_mean = 0;
  for (auto& v : velocities)
    v_mean += v;
  v_mean/= size -1;

  // velocity standard deviation
  ts::real v_stdev = 0;
  for (auto& v : velocities)
    v_stdev += std::pow(v - v_mean, 2);
  v_stdev = std::sqrt(v_stdev/(size-1));

  frame(3) = v_mean;
  frame(4) = v_stdev;

  // background mean
  ts::real bg_mean = 0;
  for (auto it = begin; it != end; it++)
    bg_mean += (*it)(3);
  bg_mean /= size;

  frame(5) = bg_mean;


  // Mean Square Displacement
  ts::real msd = 0;
  for (auto it = begin + 1; it != end; it++)
    msd += std::pow((*begin)(1) - (*it)(1), 2) +
      std::pow((*begin)(2) - (*it)(2), 2);
  msd /= size -1;

  frame(6) = msd;

  // Mean direction
  ts::real alpha_mean = 0;
  for (auto it = begin + 1; it != end; it++)
    alpha_mean += std::atan2( (*it)(2) - (*(it-1))(2),
                              (*it)(1) - (*(it-1))(1));
  alpha_mean /= size -1;
  frame(7) = alpha_mean;

  // // Mean Dist
  // ts::real mean_dist = 0;
  // for (auto it = begin; it != end; it++)
  //   mean_dist += std::sqrt(std::pow((*it)(1), 2) + std::pow((*it)(2), 2));
  // frame(8) = mean_dist;

  // // Ratio of interpolated frames to all frames
  // ts::real intr = 0;
  // for (auto it = begin; it != end; it++)
  //   intr += (*it)(4);

  //  frame(9) = intr/size;

  return frame;
}


ts::data::SequenceSet transform(const ts::data::SequenceSet& input) {
  ts::data::SequenceSet data;

  unsigned int window = 15;

  for (auto& raw_sequence : input) {
    ts::data::Sequence sequence;
    sequence.setCategory(raw_sequence.category());
    for (unsigned int i = 0; i < raw_sequence.size() - window; i+= window)
      sequence << calcFrame(raw_sequence.begin() + i,
                            raw_sequence.begin() + i + window);
    data << sequence;
  }

  return data;
}

void classify(int argc, char** argv) {

  unsigned int data_set_id;
  std::stringstream ss;
  ss << argv[1] << " "
     << argv[2] << " "
     << argv[3] << " ";


  unsigned int hidden_states_n;
  unsigned int hidden_states_y;

  ts::real gmm_init_sigma = 0.1;
  ts::real gmm_init_mean_samples = 10;

  ss >> data_set_id
     >> hidden_states_y
     >> hidden_states_n;

  std::cout << "classifying data sets " << data_set_id << std::endl;
  ts::data::SequenceSet data;
  {
    pqxx::connection conn("");
    pqxx::work w(conn);
    data = transform(ts::data::SequenceSet::read(data_set_id, w));
    //    data = ts::data::SequenceSet::read(data_set_id, w);
  }

  ts::algorithm::random_shuffle(data.begin(), data.end());

  auto cls = ts::objective::gHMMtrain(data.begin(), data.begin() + data.size()/1.5,
                                      {hidden_states_n, hidden_states_y},
                                      gmm_init_sigma,
                                      gmm_init_mean_samples);
  ts::real log_p_yes = 0;
  ts::real log_p_no = 0;

  for (auto& s : data)
    if (s.category())
      log_p_yes++;
    else
      log_p_no++;

  ts::real total = log_p_yes + log_p_no;
  log_p_yes = std::log(log_p_yes/total);
  log_p_no = std::log(log_p_no/total);

  ts::Matrix conf;

  conf = ts::Matrix::Zero(2, 2);
  for (auto it = data.begin(); it != (data.begin() + data.size()/1.5); it++) {
    try {
      ts::real p_yes = cls[1].p(it->begin(), it->end()) + log_p_yes;
      ts::real p_no = cls[0].p(it->begin(), it->end()) + log_p_no;
      conf(p_yes > p_no,  it->category())++;
    } catch (ts::ErrorNotFinite e) {
      std::cerr << e.what() << std::endl;
    }
  }
  std::cout << "Train F1 " << ts::objective::f1_score(conf) << std::endl;

  conf = ts::Matrix::Zero(2, 2);
  for (auto it = data.begin() + data.size()/1.5; it != data.end(); it++) {
    try {
      ts::real p_yes = cls[1].p(it->begin(), it->end()) + log_p_yes;
      ts::real p_no = cls[0].p(it->begin(), it->end()) + log_p_no;
      conf(p_yes > p_no,  it->category())++;
    } catch (ts::ErrorNotFinite e) {
      std::cerr << e.what() << std::endl;
    }

  }
  std::cout << "Test F1 " << ts::objective::f1_score(conf) << std::endl;

  std::cout << conf << std::endl;
}



void plot_hist(int argc, char** argv) {

  unsigned int id;
  std::stringstream ss;
  ss << argv[1];
  ss >> id;

  ts::data::SequenceSet yes;
  ts::data::SequenceSet no;
  {
    pqxx::connection c;
    pqxx::work w(c);

    ts::data::SequenceSet trf = transform(ts::data::SequenceSet::read(id, w));

    for (auto& p : trf)
      if (p.category())
        yes << p;
      else
        no << p;
  }

  {
    std::stringstream script;
    script << "$data <<EOF" << std::endl;
    for (auto& sequence : yes)
      for (auto& frame : sequence)
        script << frame.transpose() << std::endl;
    script << "EOF" << std::endl;


    script
      << "bin(x,width)=width*floor(x/width)" << std::endl

      << "set terminal qt size 1200, 900" << std::endl
      << "set title 'YES'" << std::endl
      << "set multiplot layout 2,5 rowsfirst" << std::endl

      << "set xlabel 'Intensity Mean'" << std::endl
      << "plot $data using (bin($1,0.1)):(1.0) smooth freq with boxes notitle"
      << std::endl

      << "set xrange [0:1] " << std::endl
      << "set xlabel 'Intensity Standard Deviation'" << std::endl
      << "plot $data using (bin($2,0.01)):(1.0) smooth freq with boxes notitle"
      << std::endl

      << "set autoscale" << std::endl

      << "set xlabel 'Intensity AutoCorrelation'" << std::endl
      << "plot $data using (bin($3,0.05)):(1.0) smooth freq with boxes notitle"
      << std::endl

      << "set xlabel 'Velocity Mean'" << std::endl
      << "plot $data using (bin($4,0.05)):(1.0) smooth freq with boxes notitle"
      << std::endl

      << "set xlabel 'Velocity Standard Deviation'" << std::endl
      << "plot $data using (bin($5,0.05)):(1.0) smooth freq with boxes notitle"
      << std::endl

      << "set xlabel 'Backgorund Mean'" << std::endl
      << "plot $data using (bin($6,0.01)):(1.0) smooth freq with boxes notitle"
      << std::endl

      << "set xrange [0:0.2] " << std::endl
      << "set xlabel 'MSD'" << std::endl
      << "plot $data using (bin($7,0.001)):(1.0) smooth freq with boxes notitle"
      << std::endl

      << "set autoscale" << std::endl
      << "set xlabel 'Direction'" << std::endl
      << "plot $data using (bin($8,0.05)):(1.0) smooth freq with boxes notitle"
      << std::endl

      << "set xlabel 'Mean Dist'" << std::endl
      << "plot $data using (bin($9,0.2)):(1.0) smooth freq with boxes notitle"
      << std::endl


      << "set xlabel 'Interpolated'" << std::endl
      << "plot $data using (bin($10,0.001)):(1.0) smooth freq with boxes notitle"
      << std::endl


      << "unset multiplot" << std::endl;


    ts::plot::gnuplot(script.str());
  }


  {
    std::stringstream script;
    script << "$data <<EOF" << std::endl;
    for (auto& sequence : no)
      for (auto& frame : sequence)
        script << frame.transpose() << std::endl;
    script << "EOF" << std::endl;


    script
      << "bin(x,width)=width*floor(x/width)" << std::endl

      << "set terminal qt size 1200, 900" << std::endl
      << "set title 'NO'" << std::endl
      << "set multiplot layout 2,5 rowsfirst" << std::endl

      << "set xlabel 'Intensity Mean'" << std::endl
      << "plot $data using (bin($1,0.1)):(1.0) smooth freq with boxes notitle"
      << std::endl

      << "set xrange [0:1] " << std::endl
      << "set xlabel 'Intensity Standard Deviation'" << std::endl
      << "plot $data using (bin($2,0.01)):(1.0) smooth freq with boxes notitle"
      << std::endl

      << "set autoscale" << std::endl

      << "set xlabel 'Intensity AutoCorrelation'" << std::endl
      << "plot $data using (bin($3,0.05)):(1.0) smooth freq with boxes notitle"
      << std::endl

      << "set xlabel 'Velocity Mean'" << std::endl
      << "plot $data using (bin($4,0.05)):(1.0) smooth freq with boxes notitle"
      << std::endl

      << "set xlabel 'Velocity Standard Deviation'" << std::endl
      << "plot $data using (bin($5,0.05)):(1.0) smooth freq with boxes notitle"
      << std::endl

      << "set xlabel 'Backgorund Mean'" << std::endl
      << "plot $data using (bin($6,0.01)):(1.0) smooth freq with boxes notitle"
      << std::endl

      << "set xrange [0:0.2] " << std::endl
      << "set xlabel 'MSD'" << std::endl
      << "plot $data using (bin($7,0.001)):(1.0) smooth freq with boxes notitle"
      << std::endl

      << "set autoscale" << std::endl
      << "set xlabel 'Direction'" << std::endl
      << "plot $data using (bin($8,0.05)):(1.0) smooth freq with boxes notitle"
      << std::endl

      << "set xlabel 'Mean Dist'" << std::endl
      << "plot $data using (bin($9,0.2)):(1.0) smooth freq with boxes notitle"
      << std::endl


      << "set xlabel 'Interpolated'" << std::endl
      << "plot $data using (bin($10,0.001)):(1.0) smooth freq with boxes notitle"
      << std::endl


      << "unset multiplot" << std::endl;


    ts::plot::gnuplot(script.str());
  }


}

void plot_correlation(int argc, char** argv) {

  unsigned int id;
  std::stringstream ss;
  ss << argv[1];
  ss >> id;

  pqxx::connection c;
  pqxx::work w(c);

  ts::data::SequenceSet raw = transform(ts::data::SequenceSet::read(id, w));
  unsigned int data_size = 0;

  for (auto& seq : raw)
    data_size += seq.size();

  ts::Matrix data(raw.dim(), data_size);
  unsigned int index = 0;
  for (auto& seq : raw)
    for (auto& frame : seq)
      data.col(index++) = frame;

  std::stringstream script;
  script << "$data <<EOD" << std::endl
         << corr(data) << std::endl
         << "EOD" << std::endl

         << "set bmargin at screen 0.2 " << std::endl
         << "set xtics rotate by 90 offset 0, -5 ('mean I' 0, 'sigma I' 1, "
    "'AC I' 2, 'mean V' 3, 'sigma V' 4, 'mean bg' 5, 'msd' 6, 'mean alpha' 7,"
    "'mean dist' 8, 'interp' 9)" << std::endl

         << "set ytics ( 'mean I' 0, 'sigma I' 1, 'AC I' 2, 'mean V' 3, "
    "'sigma V' 4, 'mean bg' 5, 'msd' 6, 'mean alpha' 7, 'mean dist' 8, 'interp' 9)" << std::endl

         << "set cbrange [-1:1]" << std::endl
         << "plot $data matrix with image" << std::endl;

  ts::plot::gnuplot(script.str());
}



int main(int argc, char** argv) {
  pqxx::connection conn;
  pqxx::work w(conn);
  ts::data::FeatureVectorSet data = ts::data::FeatureVectorSet::read(1, w);

  ts::nn::FeedForwardNetwork net({
      std::make_tuple(ts::Matrix::Random(200, data.dim()),
                      ts::Vector::Random(200),
                      ts::nn::FeedForwardNetwork::SIGMOID_ACTIVATION),
      std::make_tuple(ts::Matrix::Random(2, 200),
                      ts::Vector::Random(2),
                      ts::nn::FeedForwardNetwork::LINEAR_ACTIVATION)});

  net.setError(ts::nn::log_softmax);
  net.setDError(ts::nn::d_log_softmax);
  net.setMaxEpoch(3000);
  std::vector<std::pair<ts::Vector, ts::Vector>> training;


  for (auto& d : data) {
    ts::Vector target = ts::Vector::Zero(2);
    target(d.category()) = 1;
    training.push_back(std::make_pair(d, target));
  }


  net.train(training.begin() , training.end());

  ts::Matrix conf = ts::Matrix::Zero(2, 2);
  for (auto& d : data) {
    ts::Vector y = net(d);
    conf(d.category(), y(1) > y(0))++;
  }

  std::cout << conf << std::endl;

  return 0;
}
