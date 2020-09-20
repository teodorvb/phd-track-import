#include "data/data_point.hpp"

namespace track_select {
  namespace  data {
    DataPoint::DataPoint()
      : source_data_set_set_(false),
        source_track_id_set_(false),
        source_range_set_(false),

        category_set_(false),
        dim_set_(false) {}

    DataPoint::DataPoint(unsigned int dim)
      : source_data_set_set_(false),
        source_track_id_set_(false),
        source_range_set_(false),

        category_set_(false),
        dim_set_(true),
        dim_(dim) {}


    DataPoint::DataPoint(const DataPoint& obj)
      : Storable((const Storable&)obj),
        ForeignKey((const ForeignKey&)obj),

        source_data_set_set_(obj.source_data_set_set_),
        source_data_set_(obj.source_data_set_),

        source_track_id_set_(obj.source_track_id_set_),
        source_track_id_(obj.source_track_id_),

        source_range_set_(obj.source_data_set_set_),
        source_range_(obj.source_range_),

        category_set_(obj.category_set_),
        category_(obj.category_),

        dim_set_(obj.dim_set_),
        dim_(obj.dim_) {}

    DataPoint::DataPoint(const pqxx::tuple& data_point)
      : Storable(data_point["id"].as<database_id>()),
        source_data_set_set_(false),
        source_track_id_set_(false),
        source_range_set_(false),
        category_set_(false),
        dim_set_(true),
        dim_(data_point["frame_dim"].as<unsigned int>()){

      if (!data_point["source_data_set"].is_null())
        setSourceDataSet(data_point["source_data_set"].as<std::string>());

      if (!data_point["source_track_id"].is_null())
        setSourceTrackId(data_point["source_track_id"].as<unsigned int>());

      if (!data_point["source_range_low"].is_null() &&
          !data_point["source_range_high"].is_null())
        setSourceRange(msmm::CustomRange(data_point["source_range_low"].as<real>(),
                                         data_point["source_range_high"].as<real>()));

      if (!data_point["category"].is_null())
        setCategory(data_point["category"].as<unsigned short>());
    }


    DataPoint::~DataPoint() {}

    DataPoint& DataPoint::operator=(const DataPoint& obj) {
      Storable::operator=((const Storable&)obj);
      ForeignKey::operator=((const ForeignKey&)obj);

      source_data_set_set_ = obj.source_data_set_set_;
      source_data_set_ = obj.source_data_set_;

      source_track_id_set_ = obj.source_track_id_set_;
      source_track_id_ = obj.source_track_id_;

      source_range_set_ = obj.source_range_set_;
      source_range_ = obj.source_range_;

      category_set_ = obj.category_set_;
      category_ = obj.category_;

      dim_set_ = obj.dim_set_;
      dim_ = obj.dim_;
      return *this;
    }

    bool DataPoint::operator==(const DataPoint& obj) const {
      if (source_data_set_set_ != obj.source_data_set_set_)
        return false;

      if (source_track_id_set_ != obj.source_track_id_set_)
        return false;

      if (category_set_ != obj.category_set_)
        return false;

      if (source_range_set_ != obj.source_range_set_)
        return false;

      if (dim_set_ != obj.dim_set_)
        return false;

      if (source_data_set_set_ && source_data_set_ != obj.source_data_set_)
        return false;
      if (source_track_id_set_ && source_track_id_ != obj.source_track_id_)
        return false;
      if (source_range_set_ && source_range_ != obj.source_range_)
        return false;
      if (category_set_ && category_ != obj.category_)
        return false;
      if (dim_set_ && dim_ != obj.dim_)
        return false;

      return true;
    }




    bool DataPoint::sourceDataSetSet() const {
      return source_data_set_set_;
    }

    std::string DataPoint::sourceDataSet() const {
      if (!sourceDataSetSet())
        throw ErrorAttributeNotSet("DataPoint::sourceDataSet");

      return source_data_set_;
    }

    void DataPoint::setSourceDataSet(const std::string& set) {
      source_data_set_ = set;
      source_data_set_set_ = true;
    }



    bool DataPoint::sourceTrackIdSet() const {
      return source_track_id_set_;
    }

    unsigned int DataPoint::sourceTrackId() const {
      if (!sourceTrackIdSet())
        throw ErrorAttributeNotSet("DataPoint::sourceTrackId");

      return source_track_id_;
    }


    void DataPoint::setSourceTrackId(unsigned int id) {
      source_track_id_ = id;
      source_track_id_set_ = true;
    }


    bool DataPoint::sourceRangeSet() const {
      return source_range_set_;
    }

    const msmm::CustomRange& DataPoint::sourceRange() const {
      if (!sourceRangeSet())
        throw ErrorAttributeNotSet("DataPoint::source_range_");
      return source_range_;
    }

    void DataPoint::setSourceRange(const msmm::CustomRange& range) {
      source_range_ = range;
      source_range_set_ = true;
    }

    bool DataPoint::categorySet() const {
      return category_set_;
    }

    unsigned short DataPoint::category() const {
      if (!categorySet())
        throw ErrorAttributeNotSet("DataPoint::category_");

      return category_;
    }

    void DataPoint::setCategory(unsigned short c) {
      category_ = c;
      category_set_ = true;
    }


    bool DataPoint::dimSet() const {
      return dim_set_;
    }


    unsigned int DataPoint::dim() const {
      if (!dimSet())
        throw ErrorAttributeNotSet("DataPoint::dim_");

      return dim_;
    }

    void DataPoint::setDim(unsigned int d) {
      if (dimSet())
        throw ErrorAttributeAlreadySet("DataPoint::dim_");

      dim_ = d;
      dim_set_ = true;
    }

    database_id DataPoint::store(pqxx::work& work) const {
      work.conn().prepare("insert_sequence",
                          "insert into sequence "
                          "(id,"
                          " sequence_set_id,"
                          " track_length,"
                          " frame_dim,"
                          " data,"
                          " data_size,"
                          " data_type_size) "
                          "values($1, $2, $3, $4, $5, $6, $7);");

      database_id id = work.exec("select nextval('sequence_id_seq')")
        [0][0].as<database_id>();

      work.prepared("insert_sequence")
        (id)
        (foreignKeyId())
        (size())
        (dim())
        (data())
        (size()*dim())
        (sizeof(real)).exec();


      if (categorySet()) {
        work.conn().prepare("update_sequence_category",
                            "update sequence set category = $1 "
                            "where id = $2");
        work.prepared("update_sequence_category")
          (category())
          (id).exec();
      }

      if (sourceDataSetSet()) {
        work.conn().prepare("update_sequence_source_data_set",
                            "update sequence set source_data_set = $1 "
                            "where id = $2");
        work.prepared("update_sequence_source_data_set")
          (sourceDataSet())
          (id).exec();

      }

      if (sourceTrackIdSet()) {
        work.conn().prepare("update_sequence_track_id",
                            "update sequence set source_track_id = $1 "
                            "where id = $2");
        work.prepared("update_sequence_track_id")
          (sourceTrackId())
          (id).exec();
      }

      if (sourceRangeSet()) {
        work.conn().prepare("update_sequence_range",
                            "update sequence "
                            "set (source_range_low, source_range_high) = "
                            "($1, $2) "
                            "where id = $3");
        const msmm::CustomRange& range = sourceRange();
        work.prepared("update_sequence_range")
          (range.low())
          (range.high())
          (id).exec();
      }

      return id;
    }


  }
}
