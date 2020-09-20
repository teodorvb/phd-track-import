#define GNUXX11 1
#include "import/smi.hpp"
#include "algorithm.hpp"
#include "tools/command_line.hpp"

#include <set>
#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>
#include <track-select>
#include <track-select-msmm>
#include <fstream>
#include <thread>
#include <mutex>
#include <fstream>

#include <datamodel.hpp>
#include <msmm_project.hpp>
#include <counter++.h>
#include <datamodel/frames.hpp>
#include <datamodel/combi_score.hpp>
#include <analysis.hpp>
#include <pqxx/pqxx>

#define IMAGE_CUT_SIZE 5
/** Imports analysed tracks from hdf5 format into a database */

namespace ts = track_select;

template <class data_type>                                                      
std::string write_binary(data_type* dt, unsigned int len) {                     
  unsigned int dsize = sizeof(data_type);                                       
  unsigned char* p = (unsigned char*)dt;                                        
                                                                                
  std::stringstream ss;                                                         
  for (unsigned int i = 0; i < dsize*len; i++)                                  
    ss << p[i];                                                                 

  return ss.str();                                                              
}
                                                                                

static const char b64_table[65] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static const char reverse_table[128] = {                                        
  64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,               
  64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,               
  64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 62, 64, 64, 64, 63,               
  52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 64, 64, 64, 64, 64, 64,               
  64,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,               
  15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 64, 64, 64, 64, 64,               
  64, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,               
  41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 64, 64, 64, 64, 64                
}; 


std::string base64_encode(const std::string &bindata)
{
  using ::std::string;                                                          
  using ::std::numeric_limits;                                                  
                                                                                
  if (bindata.size() > (numeric_limits<string::size_type>::max() / 4u) * 3u) {  
    throw ::std::length_error("Converting too large a string to base64.");      
  }                                                                             
                                                                                
  const ::std::size_t binlen = bindata.size();                                  
  // Use = signs so the end is properly padded.                                 
  string retval((((binlen + 2) / 3) * 4), '=');                                 
  ::std::size_t outpos = 0;                                                     
  int bits_collected = 0;                                                       
  unsigned int accumulator = 0;                                                 
  const string::const_iterator binend = bindata.end();                          
                                                                                
  for (string::const_iterator i = bindata.begin(); i != binend; ++i) {          
    accumulator = (accumulator << 8) | (*i & 0xffu);                            
    bits_collected += 8;                                                        
    while (bits_collected >= 6) {                                               
      bits_collected -= 6;                                                      
      retval[outpos++] = b64_table[(accumulator >> bits_collected) & 0x3fu];    
    }                                                                           
  }                                                                             
  if (bits_collected > 0) { // Any trailing bits that are missing.              
    assert(bits_collected < 6);                                                 
    accumulator <<= 6 - bits_collected;                                         
    retval[outpos++] = b64_table[accumulator & 0x3fu];                          
  }                                                                             
  assert(outpos >= (retval.size() - 2));                                        
  assert(outpos <= retval.size());                                              
  return retval;                                                                
}

std::string base64_decode(const ::std::string &ascdata) {
  using ::std::string;
  string retval;
  const string::const_iterator last = ascdata.end();
  int bits_collected = 0;
  unsigned int accumulator = 0;

  for (string::const_iterator i = ascdata.begin(); i != last; ++i) {
    const int c = *i;
    if (::std::isspace(c) || c == '=') {
      // Skip whitespace and padding. Be liberal in what you accept.
      continue;
    }
    if ((c > 127) || (c < 0) || (reverse_table[c] > 63)) {
      throw ::std::invalid_argument("This contains characters not legal in a base64 encoded string.");
    }
    accumulator = (accumulator << 6) | reverse_table[c];
    bits_collected += 6;
    if (bits_collected >= 8) {
      bits_collected -= 8;
      retval += static_cast<char>((accumulator >> bits_collected) & 0xffu);
    }
  }
  return retval;
}

std::vector<std::string> exported_scores = {
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


unsigned int trackLabel(const std::vector<unsigned int>& labels, unsigned int l) {
  return !(std::find(labels.begin(), labels.end(), l)== labels.end());
}

bool trackInGDR(const std::set<std::pair<int, int>>& gdrs, int low, int high) {
  for (const auto& p : gdrs)
    if (p.first <= low && p.second >= high)
      return true;

  return false;
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

/** It takes 2 arguments:
 *  * The fist argument is the reference file with source data sets and source
 *    track ids. The format is <datasetid> <tracid> <gdr start> <gdr end> In 
 *    case GDR is missing it should be -1, -1, which mean entire interval.
 *
 *  * The second argument is the data set name as it should appear in the
 *    database. This description should be informative enough.
 */
int main(int argc, char** argv) {

  if (argc != 3) return -1;
  
  std::string reference_file(argv[1]);
  std::string info(argv[2]);

  std::vector<std::tuple<std::string, unsigned int, int, int>> import_data;

  try {
    ts::algorithm::readCSV(reference_file, import_data);
  } catch (ts::ErrorFileRead e) {
    std::cerr << "Error reading file " << reference_file << std::endl;
    std::cerr << "Error message: " << e.what() << std::endl;
    return -1;
  }

  std::map<std::string, std::vector<unsigned int>> datasets_tracks;
  std::map<std::string, std::set<std::pair<int,int>>> datasets_gdrs;

  for (const auto& t : import_data) {
    datasets_tracks[std::get<0>(t)].push_back(std::get<1>(t));
    datasets_gdrs[std::get<0>(t)].insert(std::make_pair(std::get<2>(t), std::get<3>(t)));
  }

  MSMMDataModel::CombiScore plane = MSMMDataModel::GetCombiScore1();


  pqxx::connection conn;
  pqxx::work w_dataset(conn);
  
  w_dataset.conn().prepare("insert_dataset", "insert into smidata_desc (id, description, created_at) "
			   "values ($1, $2, now())");


  long int dataid = w_dataset.exec("select nextval('smidata_desc_id_seq')")[0][0]
    .as<long int>();

  w_dataset.prepared("insert_dataset")
    (dataid)
    (info).exec();

  w_dataset.commit();
  /* Each data set is associated with list of tracks.
   * For each data set import all the tracks.
   */ 
  for (const auto& dataset_tracks : datasets_tracks) {
    pqxx::work w(conn);
    w.conn().prepare("insert_record",
		     "insert into smidata (dataset_id, source_data_set, source_track_id, track_start, "
		     "track_end, category, levels_order, track_size, frame_dim_int, frame_dim_img, "
		     "scores, levels, data_int, data_img, lda_filter, levels_old, levels_new, scores_old, "
		     "scores_new) "
		     "values($1, $2, $3, $4, $5, $6, $7, $8, $9, $10, $11, $12, $13, $14, $15, $16, $17,"
		     "$18, $19);");


    const std::string& source_data_set = dataset_tracks.first;
    const std::vector<unsigned int>& positive_tids = dataset_tracks.second;

    // Prepare the project
    CppUtil::FracCounter counter;

    MSMMProject::Project project(source_data_set);
    project.set_counter(counter);

    const MSMMDataModel::ImFrameStack frames = project.stacks().proc_frames();
    MSMMDataModel::ChannelConfig cc = project.get_channel_config();
    if (cc.n() != 1)
      throw std::runtime_error("Number of channels different from 1");
    MSMMDataModel::ChId channel_id = cc.getId(0);

    unsigned int first_frameid = project.get_FrameId_range().first;
    unsigned int last_frameid = project.get_FrameId_range().second;

    /* If there is at least one track with has GDR the who data set (-1, -1)
     * set the global drift ranges for that data set to the entire duration
     * of the data set
     */
    for (const auto& p : datasets_gdrs.at(source_data_set)) {
      if (p.first == -1 && p.second == -1) {
	std::set<std::pair<int, int>> s;
	s.insert(std::make_pair((int)first_frameid, (int)last_frameid));
        datasets_gdrs.at(source_data_set) = s;
	break;
      }
    }

    /* Process each track in the project */
    for (const auto& tid_track : project.get_tracks()) {
      MSMMDataModel::TrackId tid = tid_track.first;
      MSMMDataModel::Track track = tid_track.second;

      if (track.size() < 20) continue;

      
      /* Calculate scores and levels */
      MSMMDataModel::LevelOptions lo;
      lo.overwrite = true;
      lo.level_version = MSMMDataModel::LevelOptions::StepProb;
      track.set_level_options(lo);
      track.calc_scores(cc, frames, project.get_options().spot_fwhm);

      /* Not interested in tracks with less than 2 levels as they are rejected
       * automatically and are not subject of inspection 
       */
      if (track.get_levels().size() < 2) continue;

      unsigned int track_size = last_frameid - first_frameid + 1;
      
      const unsigned int int_size = track_size * 7;
      const unsigned int img_size = track_size * (std::pow(2*IMAGE_CUT_SIZE+1, 2));

      float * int_buffer = new float[int_size];
      unsigned int int_index = 0;
      
      float * img_buffer = new float[img_size];
      unsigned int img_index = 0;
      unsigned int category = trackLabel(positive_tids, tid);

      try {

      	std::string track_case = getTrackCase(track, project, datasets_gdrs.at(source_data_set));

	float filter = 0;
	for (const auto& p : plane) filter += p.second * track.get_score(p.first);


      	std::string scores = getScores(track, exported_scores, MSMMDataModel::LevelOptions::Std, project);
      	std::string scores_old = getScores(track, exported_scores, MSMMDataModel::LevelOptions::StepProb, project);
      	std::string scores_new = getScores(track, exported_scores, MSMMDataModel::LevelOptions::NNet, project);

      	std::string levels = getLevels(track, MSMMDataModel::LevelOptions::Std, project);
      	std::string levels_old = getLevels(track, MSMMDataModel::LevelOptions::StepProb, project);
      	std::string levels_new = getLevels(track, MSMMDataModel::LevelOptions::NNet, project);

      	/** Extrapolated Tracks **/
      	bool keepgoing = true;
      	MSMMTracking::ExtrapolateTrack(track,
      				       project.stacks().proc_frames(),
      				       project.get_options().spot_fwhm,
      				       project.get_options().maskw,
      				       project.get_options().nbar,
      				       NULL,
      				       keepgoing,
      				       0,
      				       project.should_i_separate_channels());


      	/* Extract Feature Sequence */
      	for (auto frame_it = frames.begin(); frame_it != frames.end(); frame_it++) {

      	  MSMMDataModel::FrameId frame_id = frame_it->getId();
      	  if (frame_id < track.first_frameid()) continue;
      	  if (frame_id > track.last_frameid()) break;
	  
      	  int_buffer[int_index++] = frame_id;
      	  int_buffer[int_index++] = track[frame_id].counts.at(channel_id);
      	  int_buffer[int_index++] = track[frame_id].sigcounts.at(channel_id);
      	  int_buffer[int_index++] = track[frame_id].getX();
      	  int_buffer[int_index++] = track[frame_id].getY();
      	  int_buffer[int_index++] = track[frame_id].bg.at(channel_id);
      	  int_buffer[int_index++] = track[frame_id].interp;	
      	}


      	/* Extract Image Sequence */
      	for (auto frame_it = frames.begin(); frame_it != frames.end(); frame_it++) {

      	  MSMMDataModel::FrameId frame_id = frame_it->getId();
      	  if (frame_id < track.first_frameid()) continue;
      	  if (frame_id > track.last_frameid()) break;

      	  float x = track[frame_id].getX();
      	  float y = track[frame_id].getY();


      	  // Copy the image around the feature and represent it as vector.
      	  for (int i = -IMAGE_CUT_SIZE; i <= IMAGE_CUT_SIZE; i++)
      	    for (int j = -IMAGE_CUT_SIZE; j <= IMAGE_CUT_SIZE; j++)
      	      img_buffer[img_index++] = frame_it->getImage(channel_id).at(x+i, y+j);
      	} // endfor frame


      	/* Check for consistency and add to the data files */
      	if (img_index != img_size || int_index != int_size) 
      	  throw std::runtime_error("Missmatch between size and last index");

      	w.prepared("insert_record")
      	  (dataid)
      	  (source_data_set)
      	  (tid)
      	  (track.first_non_interpolated_frameid())
      	  (track.last_non_interpolated_frameid())
      	  (category)
      	  (track_case)
      	  (track_size)
      	  (7)
      	  (std::pow(2*IMAGE_CUT_SIZE+1, 2))
      	  (scores)
      	  (levels)
      	  (base64_encode(write_binary(int_buffer, int_index)))
      	  (base64_encode(write_binary(img_buffer, img_index)))
	  (filter)
	  (levels_old)
	  (levels_new)
	  (scores_old)
	  (scores_new).exec();

	std::cout << "record " << tid << " inserted " << std::endl;

      } catch(std::runtime_error e) {
	if (category) std::cout << e.what() << std::endl;
      } catch(...) {
      }

      delete[] img_buffer;
      delete[] int_buffer;
    }
    w.commit();
    std::cout << "records commited. Starting new dataset ..." << std::endl;
  }

  return 0;
}
