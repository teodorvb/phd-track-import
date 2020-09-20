#include "data/data_set.hpp"
#include "data/feature_vector.hpp"
#include "data/sequence.hpp"

#include <cstddef>
#include <functional>
#include <algorithm>
#include <sstream>


namespace track_select {
  namespace data {
    template <class data_point>
    DataSet<data_point>::DataSet()
      : info_set_(false),
        frame_info_set_(false),
        dim_set_(false) {}

    template <class data_point>
    DataSet<data_point>::DataSet(const DataSet& obj)
      : Storable((const Storable&)obj),
        data_points_(obj.data_points_),

        info_set_(obj.info_set_),
        info_(obj.info_),

        frame_info_set_(obj.frame_info_set_),
        frame_info_(obj.frame_info_),

        dim_set_(obj.dim_set_),
        dim_(obj.dim_) {

      for (auto& point : data_points_)
        point.setOwner(this);
    }

    template <class data_point>
    DataSet<data_point>::DataSet(database_id id,
                                 const std::string& info,
                                 const std::vector<std::string>& frame_info)
      : Storable(id),
        info_set_(true),
        info_(info),

        frame_info_set_(true),
        frame_info_(frame_info),

        dim_set_(false) {}

    template <class data_point>
    DataSet<data_point>::DataSet(const std::string& info)
      : info_set_(true),
        info_(info),

        frame_info_set_(false),
        dim_set_(false) {}

    template <class data_point>
    DataSet<data_point>::DataSet(const std::string& info,
                                 const std::vector<std::string>& frame_info)
      : info_set_(true),
        info_(info),

        frame_info_set_(true),
        frame_info_(frame_info),
        dim_set_(false) {}

    template <class data_point>
    DataSet<data_point>::~DataSet() {}

    template <class data_point>
    DataSet<data_point>& DataSet<data_point>::operator=(const DataSet & obj) {
      Storable::operator=((const Storable&)obj);

      data_points_ = obj.data_points_;

      for (auto& point : data_points_)
        point.setOwner(this);

      info_set_ = obj.info_set_;
      info_ = obj.info_;

      frame_info_set_ = obj.frame_info_set_;
      frame_info_ = obj.frame_info_;

      dim_set_ = obj.dim_set_;
      dim_ = obj.dim_;

      return *this;
    }

    template <class data_point>
    void DataSet<data_point>::setDimUnsafe(unsigned int dim) {
      dim_ = dim;
      dim_set_ = true;
    }


    template <class data_point>
    void DataSet<data_point>::setInfoUnsafe(const std::string& info) {
      info_ = info;
      info_set_ = true;
    }

    template <class data_point>
    void DataSet<data_point>::setFrameInfoUnsafe(const std::vector<std::string>& fi) {
      if (!dimSet()) setDimUnsafe(fi.size());
      if (fi.size() != dim()) {
        std::stringstream ss;
        ss << "DataSet: info dim should be " << dim()
           << " however it is " << fi.size();
        throw ErrorInconsistentDim(ss.str());
      }


      frame_info_ = fi;
      frame_info_set_ = true;
    }


    template <class data_point>
    void DataSet<data_point>::setDim(unsigned int dim) {
      lock_.lock();
      setDimUnsafe(dim);
      lock_.unlock();
    }

    template <class data_point>
    void DataSet<data_point>::setInfo(const std::string& info) {
      lock_.lock();
      setInfoUnsafe(info);
      lock_.unlock();
    }


    template <class data_point>
    void DataSet<data_point>::setFrameInfo(const std::vector<std::string>& frame_info) {
      lock_.lock();
      setFrameInfoUnsafe(frame_info);
      lock_.unlock();
    }

    template<class data_point>
    void DataSet<data_point>::setDimIfNotSet(unsigned int dim) {
      lock_.lock();
      if (!dimSet()) setDimUnsafe(dim);
      lock_.unlock();
    }

    template<class data_point>
    void DataSet<data_point>::setInfoIfNotSet(const std::string& info) {
      lock_.lock();
      if (!infoSet()) setInfoUnsafe(info);
      lock_.unlock();
    }

    template<class data_point>
    void DataSet<data_point>::setFrameInfoIfNotSet(const std::vector<std::string>& i) {
      lock_.lock();
      if (!frameInfoSet()) setFrameInfoUnsafe(i);
      lock_.unlock();
    }


    template <class data_point>
    bool DataSet<data_point>::infoSet() const {
      return info_set_;
    }


    template <class data_point>
    const std::string& DataSet<data_point>::info() const {
      if (!infoSet())
        throw ErrorAttributeNotSet("DataSet::info_");
      return info_;
    }


    template <class data_point>
    bool DataSet<data_point>::frameInfoSet() const {
      return frame_info_set_;
    }

    template <class data_point>
    const std::vector<std::string>& DataSet<data_point>::frameInfo() const {
      if (!frameInfoSet())
        throw ErrorAttributeNotSet("DataSet::frame_info_");

      return frame_info_;
    }


    template <class data_point>
    DataSet<data_point>& DataSet<data_point>::operator<<(const data_point& dp) {
      lock_.lock();
      try {
        if (dim() != dp.dim()) {
          std::stringstream ss;
          ss << "DataSet: data dim should be " << dim()
             << " however it is " << dp.dim();
          throw ErrorInconsistentDim(ss.str());
        }
      } catch (ErrorAttributeNotSet e) {
        if (dimSet()) throw e;
        setDimUnsafe(dp.dim());
      }

      data_points_.push_back(dp);
      data_points_.back().setOwner(this);
      data_points_.back().clearId();

      lock_.unlock();
      return *this;

    }

    template <class data_point>
    DataSet<data_point>&
    DataSet<data_point>::addDataPointWithId(const data_point& dp) {
      try {
        if (dim() != dp.dim()) {
          std::stringstream ss;
          ss << "Set dim " << dim() << " seq dim: " << dp.dim();
          throw ErrorInconsistentDim(ss.str());
        }
      } catch (ErrorAttributeNotSet e) {
        if (dimSet()) throw e;
        setDim(dp.dim());
      }

      data_points_.push_back(dp);
      data_points_.back().setOwner(this);
      if (!data_points_.back().hasId())
        data_points_.back().setId(dp.id());
      return *this;
    }

    template <class data_point>
    unsigned int DataSet<data_point>::size() const {
      return data_points_.size();
    }


    template <class data_point>
    unsigned int DataSet<data_point>::dim() const {
      if (!dimSet()) throw ErrorAttributeNotSet("DataSet::dim_");
      return dim_;
    }

    template <class data_point>
    bool DataSet<data_point>::dimSet() const {
      return dim_set_;
    }


    template <class data_point>
    typename DataSet<data_point>::const_iterator DataSet<data_point>::begin() const {
      return (const_iterator)data_points_.cbegin();
    }

    template <class data_point>
    typename DataSet<data_point>::const_iterator DataSet<data_point>::end() const {
      return (const_iterator)data_points_.cend();
    }

    template <class data_point>
    typename DataSet<data_point>::const_iterator DataSet<data_point>::cbegin() const {
      return (const_iterator)data_points_.cbegin();
    }

    template <class data_point>
    typename DataSet<data_point>::const_iterator DataSet<data_point>::cend() const {
      return (const_iterator)data_points_.cend();
    }

    template <class data_point>
    typename DataSet<data_point>::iterator DataSet<data_point>::begin() {
      return (iterator)data_points_.begin();
    }

    template <class data_point>
    typename DataSet<data_point>::iterator DataSet<data_point>::end() {
      return (iterator)data_points_.end();
    }


    template <class data_point>
    typename DataSet<data_point>::iterator DataSet<data_point>::erase(iterator el) {
      lock_.lock();
      auto res = data_points_.erase((typename data_container::iterator)el);
      lock_.unlock();
      return res;
    }

    template <class data_point>
    typename DataSet<data_point>::iterator
    DataSet<data_point>::erase(iterator begin, iterator end) {
      lock_.lock();
      auto res = data_points_.erase((typename data_container::iterator)begin,
                                    (typename data_container::iterator)end);
      lock_.unlock();
      return res;
    }



    template <class data_point>
    data_point& DataSet<data_point>::operator[](unsigned int index) {
      return data_points_[index];
    }

    template <class data_point>
    const data_point& DataSet<data_point>::operator[](unsigned int index) const {
      return data_points_[index];
    }


    template <class data_point>
    database_id DataSet<data_point>::store(pqxx::work& work) const {
      if (hasId()) throw ErrorRecordAlreadyExists();
      pqxx::connection conn("");


      work.conn().prepare("insert_sequence_set", "insert into sequence_set \
 (id, info, frame_dim, frame_info, created_at)                          \
 values($1, $2, $3, $4, now() )");

      /* Create set id */
      database_id db_id = work.exec("select nextval('sequence_set_id_seq')")[0][0]
        .as<database_id>();

      if (frameInfoSet()) {
        std::stringstream ss;
        for (auto dim_info : frameInfo()) ss << dim_info << std::endl;

        work.prepared("insert_sequence_set")
          (db_id)
          (info())
          (dim())
          (ss.str()).exec();
      } else {
        work.prepared("insert_sequence_set")
          (db_id)
          (info())
          (dim())
          ().exec();
      }

      for (auto sequence : *this) {
        sequence.setForeignKeyId(db_id);
        sequence.store(work);
      }

      return db_id;
    }

    template <class data_point> DataSet<data_point>
    DataSet<data_point>::read(database_id id, pqxx::work& work) {

      work.conn().prepare("select_data_set",
                   "select * from sequence_set where id = $1");

      work.conn().prepare("select_data_point_ids",
                   "select id from sequence where sequence_set_id = $1 order by id");

      /* Refers by primary key and therefore the result is always 1 tuple */
      pqxx::result data_sets = work.prepared("select_data_set")(id).exec();
      if (!data_sets.size())
        throw ErrorRecordNotFound();

      DataSet<data_point> data_set;
      data_set.setId(id);
      data_set.setInfo(data_sets[0]["info"].as<std::string>());
      data_set.setDim(data_sets[0]["frame_dim"].as<unsigned int>());

      if (!data_sets[0]["frame_info"].is_null()) {
        std::stringstream frame_info_raw(data_sets[0]["frame_info"].as<std::string>());
        std::vector<std::string> frame_info;

        for (std::string line; std::getline(frame_info_raw, line); )
          frame_info.push_back(line);
        data_set.setFrameInfo(frame_info);
      }


      pqxx::result data_points = work.prepared("select_data_point_ids")(id).exec();

      for (auto record : data_points) {
        data_point p = data_point::read(record["id"].as<database_id>(), work);
        data_set.addDataPointWithId(p);
      }

      return data_set;
    }


    template <class data_point>
    void DataSet<data_point>::zero_class_check(const DataSet<data_point>& v_set)  {
      unsigned int y = 0;
      unsigned int n = 0;
      for (auto& point : v_set)
        if (point.category())
          y++;
        else
          n++;
      if (!y) throw ErrorNullClass();
      if (!n) throw ErrorNullClass();
    }


    template class DataSet<Sequence>;
    template class DataSet<FeatureVector>;
  }

}
