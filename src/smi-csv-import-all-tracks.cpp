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


/** It takes 1 argument which is the full path of the data set
 */
int main(int argc, char** argv) {

  if (argc != 2) return -1;

  std::string dataset(argv[1]);


  // Prepare the project
  CppUtil::FracCounter counter;
  
  MSMMProject::Project project(dataset);
  project.set_counter(counter);

  const MSMMDataModel::ImFrameStack frames = project.stacks().proc_frames();
  MSMMDataModel::ChannelConfig cc = project.get_channel_config();
  if (cc.n() != 1)
    throw std::runtime_error("Number of channels different from 1");
  MSMMDataModel::ChId channel_id = cc.getId(0);

  unsigned int first_frameid = project.get_FrameId_range().first;
  unsigned int last_frameid = project.get_FrameId_range().second;


  /* Process each track in the project */
  for (const auto& tid_track : project.get_tracks()) {
    MSMMDataModel::TrackId tid = tid_track.first;
    MSMMDataModel::Track track = tid_track.second;

    /* Calculate the scores based on the new level detection algorithm */
    unsigned int track_size = last_frameid - first_frameid + 1;

    const unsigned int int_size = track_size * 7;
    const unsigned int img_size = track_size * (std::pow(2*IMAGE_CUT_SIZE+1, 2));

    float * int_buffer = new float[int_size];
    unsigned int int_index = 0;
      
    float * img_buffer = new float[img_size];
    unsigned int img_index = 0;

    std::stringstream ss_intensity;
    ss_intensity << "track_" << tid << "-intensity.csv";
    std::ofstream out_intensity(ss_intensity.str());
 
    std::stringstream ss_image;
    ss_image << "track_" << tid << "-image.csv";
    std::ofstream out_image(ss_image.str());
      
    out_intensity << "frame id,"
		  << "intensity, "
		  << "intensity error, "
		  << "pos_x, pos_y, "
		  << "background intensity, "
		  << "interpolated" << std::endl;
      
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

      /* Extract Feature Sequence */
      for (auto frame_it = frames.begin(); frame_it != frames.end(); frame_it++) {

	MSMMDataModel::FrameId frame_id = frame_it->getId();

	if (frame_id < track.first_frameid()) continue;
	if (frame_id > track.last_frameid()) break;
	  
	out_intensity << frame_id << ","
		      << track[frame_id].counts.at(channel_id) << ","
		      << track[frame_id].sigcounts.at(channel_id) << ","
		      << track[frame_id].getX() << ","
		      << track[frame_id].getY() << ","
		      << track[frame_id].bg.at(channel_id) << ","
		      << track[frame_id].interp << std::endl;
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
	  for (int j = -IMAGE_CUT_SIZE; j <= IMAGE_CUT_SIZE; j++) {
	    out_image << frame_it->getImage(channel_id).at(x+i, y+j);
	    if (i != IMAGE_CUT_SIZE || j != IMAGE_CUT_SIZE)
	      out_image <<",";
	  }
	  
	out_image << std::endl;
      } // endfor frame



    } catch(std::runtime_error e) {
    } catch(...) {
    }

  }


  return 0;
}
