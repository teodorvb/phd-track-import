#include "helpers.hpp"

#include <datamodel.hpp>
#include <msmm_project.hpp>

unsigned int trackLabel(const std::vector<unsigned int>& labels, unsigned int l) {
  return !(std::find(labels.begin(), labels.end(), l)== labels.end());
}

bool trackInGDR(const std::set<std::pair<int, int>>& gdrs, int low, int high) {
  for (const auto& p : gdrs)
    if (p.first <= low && p.second >= high)
      return true;

  return false;
}


std::string serializeLevels(const MSMMDataModel::Track & track) {
  std::stringstream levels_ss;
  levels_ss << "{";
  if (track.get_levels().size() > 0) {

    for (int i = 0; i < (int)track.get_levels().size() -1; i++) {
      levels_ss << "{";

      for (int j = 0; j < (int)track.get_levels()[i].get_ranges().size() -1; j++) {
	levels_ss << "{"
		  << track.get_levels()[i].get_ranges()[j].min() << ","
		  << track.get_levels()[i].get_ranges()[j].max() << "},";
      }
      unsigned int j = track.get_levels()[i].get_ranges().size() - 1;
      levels_ss <<"{"
		<< track.get_levels()[i].get_ranges()[j].min() << ","
		<< track.get_levels()[i].get_ranges()[j].max() << "}";
	
      levels_ss << "},";
    }

    {
      int i = track.get_levels().size() -1;
      levels_ss << "{";
      for (int j = 0; j < (int)track.get_levels()[i].get_ranges().size() - 1; j++) {
	levels_ss << "{" << track.get_levels()[i].get_ranges()[j].min()
		  << "," << track.get_levels()[i].get_ranges()[j].max() << "},";
      }
      int j = (int)track.get_levels()[i].get_ranges().size() - 1;
      levels_ss << "{" << track.get_levels()[i].get_ranges()[j].min()
		<< "," << track.get_levels()[i].get_ranges()[j].max() << "}";

      levels_ss << "}";
    }

  }
  levels_ss << "}";

  return levels_ss.str();
}



std::string getLevels(MSMMDataModel::Track track,
		      MSMMDataModel::LevelOptions::LevelDetectionVersion version,
		      const MSMMProject::Project& project) {
  MSMMDataModel::LevelOptions lo;
  lo.overwrite = true;
  lo.level_version = version;
  track.set_level_options(lo);
  track.calc_scores(project.get_channel_config(),
		    project.stacks().proc_frames(),
		    project.get_options().spot_fwhm);

  if (!track.get_levels().size()) return std::string("{}");

  std::stringstream levels_ss;
  levels_ss << "{";
  if (track.get_levels().size() > 1) {
    for (int i = 0; i < (int)track.get_levels().size() -1; i++) {
      levels_ss << "{";
    
      for (int j = 0; j < (int)track.get_levels()[i].get_ranges().size() -1; j++) {
	levels_ss << "{"
		  << track.get_levels()[i].get_ranges()[j].min() << ","
		  << track.get_levels()[i].get_ranges()[j].max() << "},";
      }
      unsigned int j = track.get_levels()[i].get_ranges().size() - 1;
      levels_ss <<"{"
		<< track.get_levels()[i].get_ranges()[j].min() << ","
		<< track.get_levels()[i].get_ranges()[j].max() << "}";
	
      levels_ss << "},";
    }
  }

  {
    int i = track.get_levels().size() -1;
    levels_ss << "{";
    for (int j = 0; j < (int)track.get_levels()[i].get_ranges().size() - 1; j++) {
      levels_ss << "{" << track.get_levels()[i].get_ranges()[j].min()
		<< "," << track.get_levels()[i].get_ranges()[j].max() << "},";
    }
    int j = (int)track.get_levels()[i].get_ranges().size() - 1;
    levels_ss << "{" << track.get_levels()[i].get_ranges()[j].min()
	      << "," << track.get_levels()[i].get_ranges()[j].max() << "}";

    levels_ss << "}";
  }

  levels_ss << "}";
	
  return levels_ss.str();
}

std::string getScores(MSMMDataModel::Track track,
		      std::vector<std::string> exported_scores,
		      MSMMDataModel::LevelOptions::LevelDetectionVersion version,
		      const MSMMProject::Project& project) {
  MSMMDataModel::LevelOptions lo;
  lo.overwrite = true;
  lo.level_version = version;
  track.set_level_options(lo);
  track.calc_scores(project.get_channel_config(),
		    project.stacks().proc_frames(),
		    project.get_options().spot_fwhm);

  std::stringstream scores_ss;
  scores_ss << "<|";
	
  for (unsigned int i = 0; i < exported_scores.size(); i++) {
    scores_ss << "\"" << exported_scores[i]
	      << "\"" << " -> " << track.get_score(exported_scores[i]);
    if (i < exported_scores.size()-1)
      scores_ss << ",";
  }
  scores_ss << "|>";
  
  return scores_ss.str();
}

std::string getTrackCase( const MSMMDataModel::Track& track,
			  const MSMMProject::Project& project,
			  const std::set<std::pair<int,int>> gdrs) {
  unsigned int first_frameid = project.get_FrameId_range().first;
  unsigned int last_frameid = project.get_FrameId_range().second;

  MSMMDataModel::ChannelConfig cc = project.get_channel_config();
  if (cc.n() != 1)
    throw std::runtime_error("Number of channels different from 1");
  MSMMDataModel::ChId channel_id = cc.getId(0);

  std::vector<std::pair<std::vector<std::pair<int, int>>, float>> lvls;


  for (const auto& lvl : track.get_levels()) {
    std::vector<std::pair<int, int>> ranges;
    for (const auto& r : lvl.get_ranges()) {
      ranges.push_back(std::make_pair(r.min(), r.max()));
    }
    lvls.push_back(std::make_pair(ranges, lvl.mean_counts.at(channel_id)));
  }


  for (auto & lvl : lvls) {
    std::sort(lvl.first.begin(), lvl.first.end(), [=](const std::pair<int, int>& l, const std::pair<int, int>& r) {
		return l.second < r.second;
	      });
  }


  std::sort(lvls.begin(), lvls.end(), [=](const std::pair<std::vector<std::pair<int, int>>, float>& l,
					  const std::pair<std::vector<std::pair<int, int>>, float>& r) {
      return l.first.back().second < r.first.back().second;
    });

  
  if (last_frameid - track.last_frameid() > 3 &&
      lvls[lvls.size() - 1].second < lvls[lvls.size() - 2].second) {
    
    MSMMDataModel::FrameId step = lvls[lvls.size() -2].first.back().second;
    if (!trackInGDR(gdrs,  step - 10, step + 10)) throw std::runtime_error("track not in GDR");
    return "L1Last";
  }


  if (track.first_frameid() - first_frameid > 3 && 
      lvls[0].second < lvls[1].second) {

    MSMMDataModel::FrameId step = lvls[1].first.front().first;
    if (!trackInGDR(gdrs,  step - 10, step + 10)) throw std::runtime_error("track not in GDR");
    return "L1First";
  }

  throw std::runtime_error("Bad structure");
}
