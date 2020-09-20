#include "data/sequence.hpp"

namespace track_select {

  namespace data {

    Sequence::Sequence() {};

    Sequence::Sequence(unsigned int d)
      : DataPoint(d) {}

    Sequence::Sequence(const Sequence& seq)
      : DataPoint((const DataPoint&)seq),
        eigen_data_(seq.eigen_data_) {
    }

    Sequence::~Sequence() {}


    Sequence& Sequence::operator=(const Sequence& obj) {
      DataPoint::operator=((const DataPoint&)obj);
      eigen_data_ = obj.eigen_data_;

      return *this;
    }

    bool Sequence::operator==(const Sequence& rs) const {

      if (!DataPoint::operator==((const DataPoint&)rs))
        return false;
      if (eigen_data_ != rs.eigen_data_)
        return false;

      return true;
    }


    const Vector& Sequence::operator[](unsigned int i) const {
      return eigen_data_[i];
    }


    Sequence& Sequence::operator<<(const Vector& frame) {
      if (!dimSet()) setDim(frame.rows());
      if (frame.rows() != dim()) {
        std::stringstream ss;
        ss << "Sequence: frame dim should be " << dim()
           << " however it is " << frame.rows();
        throw ErrorInconsistentDim(ss.str());
      }

      eigen_data_.push_back(frame);
      return *this;
    }

    pqxx::binarystring Sequence::data() const {
      real* raw_data = new real[size()*dim()];

      unsigned int index = 0;
      for (auto& frame : *this)
        for (unsigned int i = 0; i < frame.rows(); i++)
          raw_data[index++] = frame(i);


      pqxx::binarystring res(raw_data, size()*dim()*sizeof(real));
      delete [] raw_data;
      return res;
    }

    Sequence::Sequence(const pqxx::tuple& data_point)
      : DataPoint(data_point) {

      unsigned int data_size = data_point["data_type_size"].as<unsigned int>();

      if (data_size == sizeof(float))
        load<float>(data_point);
      else if (data_size == sizeof(double))
        load<double>(data_point);
      else if (data_size == sizeof(long double))
        load<long double>(data_point);
      else {
        std::stringstream ss;
        ss << "Unknown data size "
           << data_point["data_type_size"].as<unsigned int>();
        throw ErrorInconsistentDataType(ss.str());
      }

    }


    Sequence Sequence::read( database_id id, pqxx::work& work) {
      work.conn().prepare("select_data_point",
                          "select * from sequence where id = $1");

      pqxx::result data_points = work.prepared("select_data_point")(id).exec();
      if (!data_points.size()) throw ErrorRecordNotFound();

      return Sequence(data_points[0]);
    }




    unsigned int Sequence::size() const {
      return eigen_data_.size();
    }


    Sequence::const_iterator Sequence::begin() const {
      return eigen_data_.begin();
    }

    Sequence::const_iterator Sequence::end() const {
      return eigen_data_.end();
    }

    Sequence::iterator Sequence::begin() {
      return eigen_data_.begin();
    }

    Sequence::iterator Sequence::end() {
      return eigen_data_.end();
    }

    std::ostream& operator<<(std::ostream& out, const Sequence& s) {
      for (auto& f : s) out << f.transpose() << std::endl;
      return out;
    }

  } // data
} // track_select
