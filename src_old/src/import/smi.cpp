#define GNUXX11 1
#include "import/smi.hpp"

#include <flimp_tools.hpp>
#include <datamodel.hpp>
#include <msmm_project.hpp>
#include <counter++.h>
#include <datamodel/frames.hpp>
#include <datamodel/combi_score.hpp>
#include <analysis.hpp>

#include <fstream>
#include <set>
#include <vector>

#define MINIMUM_TRACK_LENGTH 20
#define MINIMUM_DISTANCE 6
#define IMAGE_CUT_SIZE 3

namespace track_select {
  namespace import {

    ErrorMissingScore::ErrorMissingScore(const std::string& str)
      : runtime_error(str) {}
    ErrorMissingScore::ErrorMissingScore(const char* str)
      : runtime_error(str) {}

    typedef std::vector<std::pair<msmm::CustomRange,
                                  MSMMDataModel::Track>>  SubTracks;

    typedef std::map<MSMMDataModel::TrackId,
                     std::pair<msmm::CustomRange,
                               MSMMDataModel::Track>> SubTracksMap;




    real meanXLastFrames(const MSMMDataModel::Track& subtrack) {
      real last_mean_x = 0;
      MSMMDataModel::Track::const_iterator end = subtrack.end();
      MSMMDataModel::Track::const_iterator begin = subtrack.end();
      for (unsigned int i = 0; i < 10; begin--, i++) {};
      for (MSMMDataModel::Track::const_iterator feature_it = begin;
           feature_it != end;
           feature_it++) {
        last_mean_x += feature_it->second.getX();
      }

      return last_mean_x / 10.0;
    }

    real meanYLastFrames(const MSMMDataModel::Track& subtrack) {
      real last_mean_y = 0;
      MSMMDataModel::Track::const_iterator end = subtrack.end();
      MSMMDataModel::Track::const_iterator begin = subtrack.end();
      for (unsigned int i = 0; i < 10; begin--, i++) {};
      for (MSMMDataModel::Track::const_iterator feature_it = begin;
           feature_it != end;
           feature_it++) {
        last_mean_y += feature_it->second.getY();
      }

      return last_mean_y / 10.0;
    }


    void smiLabelled(const ImportData& import_data,
                     const ImportGDR& import_gdr,
                     const std::vector<std::string>& scores,

                     // Output arguments
                     data::SequenceSet& images,
                     data::SequenceSet& tracks,
                     data::FeatureVectorSet& vectors,

                     data::SequenceSet& extrapolated_images,
                     data::SequenceSet& extrapolated_tracks,

                     std::ostream& out) {

      // images.setDimIfNotSet(std::pow(2*IMAGE_CUT_SIZE + 1, 2));
      // tracks.setFrameInfoIfNotSet({"Intensity",
      //       "Position X",
      //       "Position Y",
      //       "Background",
      //       "Interpolated"});
      // vectors.setFrameInfoIfNotSet(scores);

      // extrapolated_images.setDimIfNotSet(std::pow(2*IMAGE_CUT_SIZE + 1, 2));
      // extrapolated_tracks.setFrameInfoIfNotSet({"Intensity",
      //       "Position X",
      //       "Position Y",
      //       "Background",
      //       "Interpolated"});


      // std::map<std::string,
      //          std::map<unsigned int,
      //                   unsigned short>> data_sets_map;

      // for (const auto& r : import_data)
      //   data_sets_map[std::get<0>(r)][std::get<1>(r)] = std::get<2>(r);

      // std::map<std::string, std::multiset<msmm::CustomRange>> dataset_gdr;
      // for (const auto& r : import_gdr)
      //   dataset_gdr[std::get<0>(r)].insert(msmm::CustomRange(std::get<1>(r),
      //                                                        std::get<2>(r)));

      // for (auto& dataset_tracks : data_sets_map) {
      //   const std::string& source_data_set = dataset_tracks.first;
      //   const std::map<unsigned int, unsigned short>& trackids_categories =
      //     dataset_tracks.second;

      //   // Prepare the project
      //   CppUtil::FracCounter counter;
      //   MSMMProject::Project project(source_data_set);
      //   project.set_counter(counter);

      //   const MSMMDataModel::ImFrameStack frames = project.stacks().proc_frames();
      //   MSMMDataModel::ChannelConfig cc = project.get_channel_config();
      //   if (cc.n() != 1)
      //     throw std::runtime_error("Number of channels different from 1");
      //   MSMMDataModel::ChId channel_id = cc.getId(0);



      //   // Intersect the global drift ranges with the range of the experiment<
      //   std::multiset<msmm::CustomRange> global_drift_ranges;
      //   try {
      //     msmm::CustomRange p_range(project.get_FrameId_range().first,
      //                               project.get_FrameId_range().second);
      //     for (auto& range : dataset_gdr.at(source_data_set))
      //       global_drift_ranges.insert(range.intersect(p_range));
      //   } catch (std::out_of_range e) {
      //     out << e.what() << " Cant find " << source_data_set << std::endl;
      //     throw e;
      //   }

      //   // Access subtracks by track_id;
      //   SubTracksMap subtracks;
      //   {
      //     SubTracks stracks = get_sub_tracks(project, global_drift_ranges,
      //                                        MINIMUM_DISTANCE,
      //                                        MINIMUM_TRACK_LENGTH);
      //     for (auto& s : stracks)
      //       subtracks[std::get<1>(s).get_id()] = s;

      //   }

      //   // For each track_id convert the corresponding subtrack into a vector.
      //   for (auto& trackid_category : trackids_categories) {
      //     unsigned int source_track_id = trackid_category.first;
      //     unsigned short category = trackid_category.second;

      //     try {
      //       MSMMDataModel::Track& subtrack =
      //         std::get<1>(subtracks.at(source_track_id));
      //       msmm::CustomRange source_range =
      //         std::get<0>(subtracks.at(source_track_id));

      //       /* Extract Vector */
      //       data::FeatureVector vector(vectors.dim());
      //       vector.setSourceDataSet(source_data_set);
      //       vector.setSourceTrackId(source_track_id);
      //       vector.setSourceRange(source_range);
      //       vector.setCategory(category);

      //       for (unsigned int i = 0; i < scores.size(); i++)
      //         try {
      //           vector(i) = subtrack.get_score(scores[i]);
      //         } catch (std::runtime_error e) {
      //           throw ErrorMissingScore(scores[i]);
      //         }



      //       /* Extract Track */
      //       data::Sequence track(tracks.dim());
      //       track.setSourceDataSet(source_data_set);
      //       track.setSourceTrackId(source_track_id);
      //       track.setSourceRange(source_range);
      //       track.setCategory(category);

      //       real last_mean_x = meanXLastFrames(subtrack);
      //       real last_mean_y = meanYLastFrames(subtrack);

      //       for (auto frame_it = frames.begin();
      //            frame_it != frames.end();
      //            frame_it++) {
      //        MSMMDataModel::FrameId frame_id = frame_it->getId();
      //         if (frame_id < subtrack.first_frameid()) continue;
      //         if (frame_id > subtrack.last_frameid()) break;

      //         // Find feature coordinates for current frame
      //         MSMMDataModel::Feat feature = subtrack[frame_id];

      //         Vector frame(track.dim());
      //         frame(0) = feature.counts[channel_id];
      //         frame(1) = feature.getX() - last_mean_x;
      //         frame(2) = feature.getY() - last_mean_y;
      //         frame(3) = feature.bg[channel_id];
      //         frame(4) = feature.interp;
      //         track << frame;
      //       } //endfor frame

      //       /* Extract Image */
      //       data::Sequence image(images.dim());
      //       image.setSourceDataSet(source_data_set);
      //       image.setSourceTrackId(source_track_id);
      //       image.setSourceRange(source_range);
      //       image.setCategory(category);

      //       for (auto frame_it = frames.begin();
      //            frame_it != frames.end();
      //            frame_it++) {

      //         MSMMDataModel::FrameId frame_id = frame_it->getId();
      //         if (frame_id < subtrack.first_frameid()) continue;
      //         if (frame_id > subtrack.last_frameid()) break;

      //         // Find feature coordinates for current frame
      //         MSMMDataModel::Feat feature = subtrack[frame_id];


      //         // Copy the image around the feature and represent it as vector.
      //         Vector frame(images.dim());
      //         unsigned int dim_counter = 0;

      //         for (int i = -IMAGE_CUT_SIZE; i <= IMAGE_CUT_SIZE; i++)
      //           for (int j = -IMAGE_CUT_SIZE; j <= IMAGE_CUT_SIZE; j++)
      //             frame[dim_counter++] = frame_it->getImage(channel_id)
      //               .at(last_mean_x+i, last_mean_y+j);
      //         image << frame;
      //       } // endfor frame



      //       /** Extrapolated Tracks **/
      //       bool keepgoing = true;
      //       MSMMTracking::ExtrapolateTrack(subtrack,
      //                                      project.stacks().proc_frames(),
      //                                      project.get_options().spot_fwhm,
      //                                      project.get_options().maskw,
      //                                      project.get_options().nbar,
      //                                      NULL,
      //                                      keepgoing,
      //                                      0,
      //                                      project.should_i_separate_channels());

      //       /* Extract Track */
      //       data::Sequence extrapolated_track(extrapolated_tracks.dim());
      //       extrapolated_track.setSourceDataSet(source_data_set);
      //       extrapolated_track.setSourceTrackId(source_track_id);
      //       extrapolated_track.setSourceRange(source_range);
      //       extrapolated_track.setCategory(category);

      //       for (auto f_it = frames.begin(); f_it != frames.end(); f_it++) {
      //         MSMMDataModel::FrameId frame_id = f_it->getId();
      //         if (frame_id < subtrack.first_frameid()) continue;
      //         if (frame_id > subtrack.last_frameid()) break;

      //         // Find feature coordinates for current frame
      //         MSMMDataModel::Feat feature = subtrack[frame_id];

      //         Vector frame(extrapolated_track.dim());
      //         frame(0) = feature.counts[channel_id];
      //         frame(1) = feature.getX() - last_mean_x;
      //         frame(2) = feature.getY() - last_mean_y;
      //         frame(3) = feature.bg[channel_id];
      //         frame(4) = feature.interp;

      //         extrapolated_track << frame;
      //       } //endfor frame


      //       /* Extract Image */
      //       data::Sequence extrapolated_image(images.dim());
      //       extrapolated_image.setSourceDataSet(source_data_set);
      //       extrapolated_image.setSourceTrackId(source_track_id);
      //       extrapolated_image.setSourceRange(source_range);
      //       extrapolated_image.setCategory(category);

      //       for (auto f_it = frames.begin(); f_it != frames.end(); f_it++) {

      //         MSMMDataModel::FrameId frame_id = f_it->getId();
      //         if (frame_id < subtrack.first_frameid()) continue;
      //         if (frame_id > subtrack.last_frameid()) break;

      //         // Find feature coordinates for current frame
      //         MSMMDataModel::Feat feature = subtrack[frame_id];

      //         // Copy the frame around the feature and represent it as vector.
      //         Vector frame(images.dim());
      //         unsigned int dim_counter = 0;

      //         for (int i = -IMAGE_CUT_SIZE; i <= IMAGE_CUT_SIZE; i++)
      //           for (int j = -IMAGE_CUT_SIZE; j <= IMAGE_CUT_SIZE; j++)
      //             frame[dim_counter++] = f_it->getImage(channel_id)
      //               .at(last_mean_x+i, last_mean_y+j);
      //         extrapolated_image << frame;
      //       } // endfor frame

      //       extrapolated_tracks << extrapolated_track;
      //       extrapolated_images << extrapolated_image;


      //       images << image;
      //       vectors << vector;
      //       tracks << track;

      //     } catch (std::out_of_range e) {
      //       out << "Error: track_id " << source_track_id
      //           << " not found in project " << source_data_set << " " << e.what()
      //           << std::endl;
      //       continue;

      //     } catch (ErrorMissingScore e) {
      //       out << "Error: missing score " << e.what() << " subtrack with id "
      //           << source_track_id << " in range "
      //           << std::get<0>(subtracks.at(source_track_id))
      //           << " on data set " << source_data_set << std::endl;
      //       continue;

      //     }

      //   } // endfor track_id
      // } // endfor data_sets_map

    } // end smiLabelled

    // data::SequenceSet smiUnlabelled(const DataSetIds& ds_ids, bool extrapolated,
    //                                 std::ostream& log) {

    //   std::vector<std::string> data_sets;
    //   for (auto& ds_id : ds_ids) data_sets.push_back(std::get<0>(ds_id));

    //   data::SequenceSet tracks;

    //   tracks.setFrameInfo({"Intensity",
    //         "Position X",
    //         "Position Y",
    //         "Background",
    //         "Interpolated"});

    //   auto worker = [](const std::string& source_data_set,
    //                    std::ostream& log,
    //                    std::mutex& data_lock,
    //                    std::mutex& log_lock,
    //                    data::SequenceSet& tracks,
    //                    bool extrapolated) {
    //     // Prepare the project
    //     CppUtil::FracCounter counter;
    //     MSMMProject::Project project(source_data_set);
    //     project.set_counter(counter);

    //     const MSMMDataModel::ImFrameStack frames = project.stacks().proc_frames();
    //     MSMMDataModel::ChannelConfig cc = project.get_channel_config();
    //     if (cc.n() != 1)
    //       throw std::runtime_error("Number of channels different from 1");
    //     MSMMDataModel::ChId channel_id = cc.getId(0);


    //     // For each track_id convert the corresponding subtrack into a vector.

    //     for (auto& track_tuple : project.get_tracks()) {
    //       MSMMDataModel::Track& track = std::get<1>(track_tuple);
    //       unsigned int source_track_id = track.get_id();
    //       msmm::CustomRange source_range(track.first_frameid(), track.last_frameid());
    //       try {
    //         /* Extract Track */
    //         data::Sequence analysed_track(tracks.dim());
    //         analysed_track.setSourceDataSet(source_data_set);
    //         analysed_track.setSourceTrackId(source_track_id);
    //         analysed_track.setSourceRange(source_range);

    //         real last_mean_x = meanXLastFrames(track);
    //         real last_mean_y = meanYLastFrames(track);


    //         if (extrapolated) {
    //           bool keepgoing = true;
    //           MSMMTracking::ExtrapolateTrack(track,
    //                                          project.stacks().proc_frames(),
    //                                          project.get_options().spot_fwhm,
    //                                          project.get_options().maskw,
    //                                          project.get_options().nbar,
    //                                          NULL,
    //                                          keepgoing,
    //                                          0,
    //                                          project.should_i_separate_channels());
    //         }



    //         for (auto frame_it = frames.begin();
    //              frame_it != frames.end();
    //              frame_it++) {
    //           MSMMDataModel::FrameId frame_id = frame_it->getId();
    //           if (frame_id < track.first_frameid()) continue;
    //           if (frame_id > track.last_frameid()) break;

    //           // Find feature coordinates for current frame
    //           MSMMDataModel::Feat feature = track[frame_id];

    //           Vector frame(tracks.dim());
    //           frame(0) = feature.counts[channel_id];
    //           frame(1) = feature.getX() - last_mean_x;
    //           frame(2) = feature.getY() - last_mean_y;
    //           frame(3) = feature.bg[channel_id];
    //           frame(4) = feature.interp;

    //           analysed_track << frame;
    //         } //endfor frame

    //         data_lock.lock();
    //         tracks << analysed_track;
    //         data_lock.unlock();

    //       } catch (std::out_of_range e) {
    //         log_lock.lock();
    //         log << "Error: track_id " << source_track_id
    //             << " not found in project " << source_data_set << " " << e.what()
    //             << std::endl;
    //         log_lock.unlock();
    //         continue;

    //       } catch (ErrorMissingScore e) {
    //         log_lock.lock();
    //         log << "Error: missing score " << e.what() << " track with id "
    //             << source_track_id << " in range "
    //             << source_range
    //             << " on data set " << source_data_set << std::endl;
    //         log_lock.unlock();
    //         continue;
    //       }

    //     } // endfor track
    //   };

    //   std::mutex data_lock;
    //   std::mutex log_lock;
    //   unsigned int threads_num = 5;

    //   for (unsigned int i = 0; i < data_sets.size(); i+= threads_num) {
    //     std::vector<std::thread> threads;
    //     unsigned int t_size;
    //     if (threads_num + i > data_sets.size())
    //       t_size = data_sets.size() % threads_num;
    //     else
    //       t_size = threads_num;
    //     for (unsigned int j = 0; j < t_size; j++)
    //       threads.push_back(std::thread(worker,
    //                                     std::cref(data_sets[i + j]),
    //                                     std::ref(log),
    //                                     std::ref(data_lock),
    //                                     std::ref(log_lock),
    //                                     std::ref(tracks),
    //                                     extrapolated));
    //     for (auto& t : threads)
    //       t.join();
    //   }

    //   return tracks;
    // }

  } // import
} // track_select
