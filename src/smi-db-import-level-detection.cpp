#include "io.hpp"
#include "helpers.hpp"


#include <msmm_project.hpp>
#include <counter++.h>
#include <datamodel/frames.hpp>
#include <datamodel/combi_score.hpp>
#include <analysis.hpp>
#include <pqxx/pqxx>

#define IMAGE_CUT_SIZE 5
/** Imports simulated tracks from hdf5 format into a database */


/** It takes 2 arguments:
 *  * The fist argument is the reference file with source data sets and source
 *    track ids. The format is <datasetid> <tracid> <gdr start> <gdr end> In 
 *    case GDR is missing it should be -1, -1, which mean entire interval.
 *
 *  * The second argument is the data set name as it should appear in the
 *    database. This description should be informative enough.
 */
int main(int argc, char** argv) {

  if (argc <3) return -1;
  if (argc >4) return -1;

  std::string reference_file(argv[1]);
  std::string info(argv[2]);
  bool skip_negative = (argc == 4);

  std::vector<std::tuple<std::string, unsigned int, int, int>> import_data;

  try {
    readCSV(reference_file, import_data);
  } catch (std::runtime_error e) {
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

  pqxx::connection conn;
  pqxx::work w(conn);
  
  w.conn().prepare("insert_dataset", "insert into smidata_desc (id, description, created_at) "
  		   "values ($1, $2, now())");

  w.conn().prepare("insert_record",
  		   "insert into smidata (dataset_id, source_data_set, source_track_id, track_start, "
  		   "track_end, category, track_size, frame_dim_int, frame_dim_img, "
  		   "levels, levels_old, levels_new, data_int, data_img)"
  		   "values($1, $2, $3, $4, $5, $6, $7, $8, $9, $10, $11, $12, $13, $14)");
  
  long int dataid = w.exec("select nextval('smidata_desc_id_seq')")[0][0]
    .as<long int>();

  w.prepared("insert_dataset")
    (dataid)
    (info).exec();

  /* Each data set is associated with list of tracks.
   * For each data set import all the tracks.
   */ 
  for (const auto& dataset_tracks : datasets_tracks) {
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
      unsigned int category = trackLabel(positive_tids, tid);
      if (skip_negative && !category)	continue;

      /* Calculate the scores based on the new level detection algorithm */
      unsigned int track_size = last_frameid - first_frameid + 1;

      const unsigned int int_size = track_size * 7;
      const unsigned int img_size = track_size * (std::pow(2*IMAGE_CUT_SIZE+1, 2));

      float * int_buffer = new float[int_size];
      unsigned int int_index = 0;
      
      float * img_buffer = new float[img_size];
      unsigned int img_index = 0;


      try {
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


      	/* Extract Levels */
	MSMMDataModel::LevelOptions lo;
	lo.overwrite = true;

	lo.level_version = MSMMDataModel::LevelOptions::NNet;
	track.set_level_options(lo);
	track.calc_scores(cc, frames, project.get_options().spot_fwhm);
       	std::string levels_new = serializeLevels(track);


	lo.level_version = MSMMDataModel::LevelOptions::StepProb;
	track.set_level_options(lo);
	track.calc_scores(cc, frames, project.get_options().spot_fwhm);
       	std::string levels_old = serializeLevels(track);


	lo.level_version = MSMMDataModel::LevelOptions::Std;
	track.set_level_options(lo);
	track.calc_scores(cc, frames, project.get_options().spot_fwhm);
       	std::string levels = serializeLevels(track);


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
      	if (img_index != img_size || int_index != int_size) {
      	  throw std::runtime_error("Missmatch between size and last index");
	}

      	w.prepared("insert_record")
      	  (dataid)
      	  (source_data_set)
      	  (tid)
      	  (track.first_non_interpolated_frameid())
      	  (track.last_non_interpolated_frameid())
      	  (category)
      	  (track_size)
      	  (7)
      	  (std::pow(2*IMAGE_CUT_SIZE+1, 2))
      	  (levels)
	  (levels_old)
	  (levels_new)
      	  (base64_encode(write_binary(int_buffer, int_index)))
      	  (base64_encode(write_binary(img_buffer, img_index))).exec();

      } catch(std::runtime_error e) {
      } catch(...) {
      }

      delete[] img_buffer;
      delete[] int_buffer;
    }
  }

  w.commit();			
  std::cout << dataid << std::endl;
  return 0;
}
