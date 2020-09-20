#define GNUXX11 1
#include <flimp_tools.hpp>
#include <datamodel.hpp>
#include <msmm_project.hpp>
#include <counter++.h>
#include <datamodel/frames.hpp>
#include <datamodel/combi_score.hpp>
#include <analysis.hpp>

#include <fstream>

int main(int argc, char** argv) {

  if (argc < 2) return 0;

  MSMMProject::Project prj(argv[1]);
  CppUtil::FracCounter counter;

  prj.set_counter(counter);


  Analysis::calc_feature_nearest_neighbour(prj);
  auto dist = prj.get_feat_NN_dist();

  const MSMMDataModel::ImFrameStack frames = prj.stacks().proc_frames();
  const MSMMDataModel::Features& features = prj.get_features();
  const MSMMDataModel::ChannelConfig& cc = prj.get_channel_config();

  std::ofstream out("data.csv");
  for (auto& frame : frames) {
    try {
      for (unsigned int i = 0; i < features.at(frame.getId()).size(); i++) {
        //        if (dist.at(frame.getId())[i] < 6) {
          auto feature = features.at(frame.getId())[i];
          if (feature.counts.size() != 1) throw std::runtime_error("Error more than 1 channel data");
          out << feature.counts.begin()->second << "," << feature.bg.begin()->second << std::endl;
          //        }
      }
    } catch (std::out_of_range e) {
      throw;
    }
  }
  out.close();

  return 0;
}
