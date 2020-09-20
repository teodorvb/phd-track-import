#ifndef __HELPERS_H_
#define __HELPERS_H_

#define GNUXX11 yes

#include<vector>
#include<string>
#include<set>
#include <datamodel.hpp>

unsigned int trackLabel(const std::vector<unsigned int>& labels, unsigned int l);
bool trackInGDR(const std::set<std::pair<int, int>>& gdrs, int low, int high);
std::string serializeLevels(const MSMMDataModel::Track & track);



std::string getLevels(MSMMDataModel::Track track,
		      MSMMDataModel::LevelOptions::LevelDetectionVersion version,
		      const MSMMProject::Project& project);
std::string getScores(MSMMDataModel::Track track,
		      std::vector<std::string> exported_scores,
		      MSMMDataModel::LevelOptions::LevelDetectionVersion version,
		      const MSMMProject::Project& project);

std::string getTrackCase( const MSMMDataModel::Track& track,
			  const MSMMProject::Project& project,
			  const std::set<std::pair<int,int>> gdrs);


std::vector<std::string> exported_scores =  {
  "change_probability",
  "duration",
  "first_frame",
  "fragmentation",
  "level1duration",
  "level1points",
  "level2duration",
  "level2points",
  "leveliness",
  "longest_range",
  "nlevels",
  "nsteps",
  "ratio_real_points",
  "sec_longest",
  "sec_most_pts",
  "shortest_range",
  "stepness",
  "count_sigma_mls_12",
  "cts_loc_sn_l1",
  "cts_loc_sn_l2",
  "cts_slm_l1",
  "cts_slm_l2",
  "cts_stdev_lvl_1",
  "cts_stdev_lvl_2",
  "distinctiveness",
  "levelrchisq",
  "maxovermlsig",
  "maxstepfrac",
  "maxstepprob",
  "meanlocalsigmacounts_1",
  "meanovermlsig",
  "sigmacounts_1",
  "significance",
  "sigovermlsig",
  "stepratio_lvl12",
  "displacement",
  "dr12",
  "pos_density_12",
  "pos_error_1",
  "pos_error_2",
  "pos_sigma_mls_12",
  "pos_sigma_locmean_12",
  "semdr12",
  "sigdr12",
  "total_pos_density",
  "cts_mean_lvl_1",
  "cts_mean_lvl_2"
};
#endif
