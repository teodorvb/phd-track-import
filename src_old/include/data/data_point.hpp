#ifndef __DATA_DATA_POINT_H_
#define __DATA_DATA_POINT_H_

#include <string>
#include <stdexcept>
#include <track-select>
#include <track-select-msmm>
#include <pqxx/pqxx>
#include "data/storable.hpp"
#include "data/foreign_key.hpp"

namespace track_select {
  namespace data {
    class DataPoint : public Storable, public ForeignKey {

      bool source_data_set_set_;
      std::string source_data_set_;

      bool source_track_id_set_;
      unsigned int source_track_id_;

      bool source_range_set_;
      msmm::CustomRange source_range_;

      bool category_set_;
      unsigned short category_;

      bool dim_set_;
      unsigned int dim_;
    protected:
      virtual unsigned int size() const = 0;
      virtual pqxx::binarystring data() const = 0;
      DataPoint(const pqxx::tuple& t);
    public:
      DataPoint();
      DataPoint(unsigned int dim);
      DataPoint(const DataPoint&);

      virtual ~DataPoint();

      virtual DataPoint& operator=(const DataPoint&);
      virtual bool operator==(const DataPoint&) const;

      virtual bool sourceDataSetSet() const;
      virtual std::string sourceDataSet() const;
      virtual void setSourceDataSet(const std::string&);

      virtual bool sourceTrackIdSet() const;
      virtual unsigned int sourceTrackId() const;
      virtual void setSourceTrackId(unsigned int);

      virtual bool sourceRangeSet() const;
      virtual const msmm::CustomRange& sourceRange() const;
      virtual void setSourceRange(const msmm::CustomRange& range);

      virtual bool categorySet() const;
      virtual unsigned short category() const;
      virtual void setCategory(unsigned short c);

      virtual bool dimSet() const;
      virtual unsigned int dim() const;
      virtual void setDim(unsigned int);

      virtual database_id store(pqxx::work& w) const;
    };

  }
}


#endif
