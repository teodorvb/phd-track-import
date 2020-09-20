#include "data/feature_vector.hpp"


namespace track_select {
  namespace data {
    FeatureVector::FeatureVector() {}

    FeatureVector::FeatureVector(unsigned int dim)
      : Vector(dim),
        DataPoint(dim) {}

    FeatureVector::FeatureVector(const Vector& vec)
      : Vector(vec),
        DataPoint(vec.rows()) {}



    FeatureVector::FeatureVector(const FeatureVector& obj)
      : Vector((const Vector&)obj),
        DataPoint((const DataPoint&)obj) {}

    FeatureVector::~FeatureVector() {}

    FeatureVector& FeatureVector::operator=(const FeatureVector& obj) {
      Vector::operator=((const Vector&)obj);
      DataPoint::operator=((const DataPoint&)obj);

      return *this;
    }

    FeatureVector& FeatureVector::operator=(const Vector& vec) {
      if (!dimSet()) setDim(vec.rows());
      if (dim() != vec.rows()) {
        std::stringstream ss;
        ss << "FeatureVector: dim of assignment should be " << dim()
           << " but it is " << vec.rows();
        throw ErrorInconsistentDim(ss.str());
      }
      Vector::operator=(vec);
      return *this;
    }

    bool FeatureVector::operator==(const FeatureVector& obj) const {
      return DataPoint::operator==((const DataPoint&)obj) &&
        Vector::operator==((const Vector&)obj);
    }


    bool FeatureVector::operator!=(const FeatureVector& obj) const {
      return !FeatureVector::operator==(obj);
    }

    void FeatureVector::setDim(unsigned int d) {
      DataPoint::setDim(d);
      Vector::resize(d, 1);
    }

    pqxx::binarystring FeatureVector::data() const {
      track_select::real * raw_data = new track_select::real[dim()];
      for (unsigned int i = 0; i < dim(); i++)
        raw_data[i] = (*this)(i);

      pqxx::binarystring res(raw_data, dim()*sizeof(track_select::real));
      delete[] raw_data;
      return res;
    }


    FeatureVector::FeatureVector(const pqxx::tuple& data_point)
      : Vector(data_point["frame_dim"].as<unsigned int>()),
        DataPoint(data_point) {

      unsigned int data_size = data_point["data_type_size"].as<unsigned int>();
      if (data_size == sizeof(float))
        load<float>(data_point);
      else if (data_size == sizeof(double))
        load<double>(data_point);
      else if (data_size ==  sizeof(long double))
        load<long double>(data_point);
      else {
        std::stringstream ss;
        ss << "Unknown data size "
           << data_point["data_type_size"].as<unsigned int>()
           << " : " << data_size << " : " << sizeof(long double);
        throw ErrorInconsistentDataType(ss.str());
      }

    }


    FeatureVector FeatureVector::read(database_id id, pqxx::work& work) {
      work.conn().prepare("select_data_point",
                          "select * from sequence where id = $1");

      pqxx::result data_points = work.prepared("select_data_point")(id).exec();
      if (!data_points.size()) throw ErrorRecordNotFound();

      return FeatureVector(data_points[0]);
    }


    unsigned int FeatureVector::size() const {
      return 1;
    }

  }
}
