#include <track-select>
#include <fstream>
#include <iostream>
#include <string>
#include <boost/tokenizer.hpp>
#include <iomanip>

#include "plot.hpp"
#include "algorithm.hpp"
#include "data/feature_vector_set.hpp"
#include "data/sequence_set.hpp"
#include "objective/common.hpp"
#include "tools/timer.hpp"

#include <flimp_tools.hpp>
#include <datamodel.hpp>
#include <msmm_project.hpp>
#include <counter++.h>
#include <datamodel/frames.hpp>
#include <datamodel/combi_score.hpp>
#include <analysis.hpp>


namespace ts = track_select;



int main(int argc, char** argv) {
  std::ifstream in("ci");

  std::string dataset;
  int id;
  float ci;
  std::map<std::string, std::vector<std::tuple<int, float>>> reference;

  while(in >> dataset >> id >> ci) {
    reference[dataset].push_back(std::make_tuple(id, ci));
  }
  in.close();

  std::ofstream out("corr.csv");
  std::ofstream log("corr.log");

  for (auto& ds : reference) {

    MSMMProject::Project project(ds.first);
    CppUtil::FracCounter counter;
    project.set_counter(counter);

    const MSMMDataModel::ImFrameStack frames = project.stacks().proc_frames();
    MSMMDataModel::ChannelConfig cc = project.get_channel_config();
    if (cc.n() != 1)
      throw std::runtime_error("Number of channels different from 1");
    MSMMDataModel::ChId channel_id = cc.getId(0);


    for (auto& t : ds.second) {
      MSMMDataModel::Track track = project.get_tracks()[std::get<0>(t)];
      track.calc_scores(cc, frames, project.get_options().spot_fwhm);

      try {
        out << std::get<1>(t) << "," << track.get_score("semdr12") << ","
            << track.get_score("pos_error_1") << "," << track.get_score("pos_error_2") << std::endl;
      } catch (std::runtime_error e) {
        log << "Error: data_set_id " << ds.first << " track_id " << std::get<0>(t) << " does not have score" << std::endl;
      }
    }
  }
  out.close();
  log.close();
}
