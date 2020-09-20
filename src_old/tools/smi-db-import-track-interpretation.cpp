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

#define IMAGE_CUT_SIZE 10
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

  if (track.get_levels().size() != 1) throw std::runtime_error("More than one level");


  std::stringstream levels_ss;
  levels_ss << "{";
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

  std::vector<std::tuple<std::string>> import_data;

  try {
    ts::algorithm::readCSV(reference_file, import_data);
  } catch (ts::ErrorFileRead e) {
    std::cerr << "Error reading file " << reference_file << std::endl;
    std::cerr << "Error message: " << e.what() << std::endl;
    return -1;
  }

  std::vector<std::string> source_data_sets;
  for (const auto & t : import_data)
    source_data_sets.push_back(std::get<0>(t));

  for (const auto & source_data_set : source_data_sets)
    std::cout << source_data_set << std::endl;
  
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
  for (const auto& source_data_set : source_data_sets) {
    pqxx::work w(conn);
    w.conn().prepare("insert_record",
		     "insert into smidata (dataset_id, source_data_set, source_track_id, track_start, "
		     "track_end, track_size, frame_dim_int, frame_dim_img, levels, data_int, data_img) "
		     "values($1, $2, $3, $4, $5, $6, $7, $8, $9, $10, $11);");


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

    /* Process each track in the project */
    for (const auto& tid_track : project.get_tracks()) {
      MSMMDataModel::TrackId tid = tid_track.first;
      MSMMDataModel::Track track = tid_track.second;

      
      /* Calculate scores and levels */
      MSMMDataModel::LevelOptions lo;
      lo.overwrite = true;
      lo.level_version = MSMMDataModel::LevelOptions::StdBG;
      track.set_level_options(lo);
      track.calc_scores(cc, frames, project.get_options().spot_fwhm);

      /* Not interested in tracks with less than 2 levels as they are rejected
       * automatically and are not subject of inspection 
       */
      if (track.get_levels().size() != 1) continue;
      if (track.get_levels().begin()->mean_counts.at(channel_id) < 10000) continue;

      for (const auto& f : track.get_levels()) {
	for (const auto& s : f.get_ranges()) {
	  std::cout << "[" <<  s.min() << ", " << s.max() << "] ";
	}
	std::cout << std::endl;
      }
	    
	

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

      	std::string levels = getLevels(track, MSMMDataModel::LevelOptions::Std, project);


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
      	  (track_size)
      	  (7)
      	  (std::pow(2*IMAGE_CUT_SIZE+1, 2))
      	  (levels)
      	  (base64_encode(write_binary(int_buffer, int_index)))
      	  (base64_encode(write_binary(img_buffer, img_index))).exec();

	std::cout << "record " << tid << " inserted " << std::endl;

      } catch(std::runtime_error e) {
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
