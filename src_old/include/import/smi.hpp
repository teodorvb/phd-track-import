#ifndef __IMPORT_SMI_H_
#define __IMPORT_SMI_H_

#include <set>
#include <track-select-msmm>
#include "data/sequence_set.hpp"
#include "data/feature_vector_set.hpp"


namespace track_select {
  namespace import {

    class ErrorMissingScore : public std::runtime_error {
    public:
      explicit ErrorMissingScore(const std::string& str);
      explicit ErrorMissingScore(const char* str);
    };

    typedef std::vector<std::tuple<std::string,
                                   unsigned int,
                                   unsigned short>> ImportData;
    typedef std::vector<std::tuple<std::string,
                                   unsigned int,
                                   unsigned int>> ImportGDR;

    void smiLabelled(const ImportData& import_data,
                     const ImportGDR& import_gdr,
                     const std::vector<std::string>& all_scores,

                     // Output arguments
                     data::SequenceSet& image,
                     data::SequenceSet& intensity,
                     data::FeatureVectorSet& score,

                     data::SequenceSet& extrapolated_images,
                     data::SequenceSet& extrapolated_tracks,

                     std::ostream& out);

    // typedef std::vector<std::tuple<std::string>> DataSetIds;
    // data::SequenceSet smiUnlabelled(const DataSetIds& ds_ids, bool extrapolated,
    //                                 std::ostream& log);

  } // data
} // track_select


#endif
