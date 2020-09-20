#ifndef __DATA_FEATURE_VECTOR_H_
#define __DATA_FEATURE_VECTOR_H_


#include "data/storable.hpp"
#include "data/foreign_key.hpp"
#include "data/data_point.hpp"

namespace track_select {
  namespace data {
    class FeatureVector : public Vector,  public DataPoint {

      template<class data_type>
      void load(const pqxx::tuple& data_point);

    protected:
      unsigned int size() const;
      pqxx::binarystring data() const;
      FeatureVector(const pqxx::tuple& t);
    public:
      FeatureVector();
      FeatureVector(unsigned int dim);
      FeatureVector(const FeatureVector&);
      FeatureVector(const Vector&);

      virtual ~FeatureVector();
      virtual FeatureVector& operator=(const FeatureVector&);
      virtual FeatureVector& operator=(const Vector&);

      virtual bool operator==(const FeatureVector&) const;
      virtual bool operator!=(const FeatureVector&) const;

      virtual void setDim(unsigned int d);

      static FeatureVector read(database_id id, pqxx::work& work);
    };

    template<class data_type>
    void FeatureVector::load(const pqxx::tuple& data_point) {
      pqxx::binarystring data(data_point["data"]);
      const data_type * d = (const data_type*)data.data();
      for (unsigned int i = 0; i < dim(); i++) (*this)(i) = d[i];

    }


  }
}

#endif
