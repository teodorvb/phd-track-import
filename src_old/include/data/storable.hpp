#ifndef __DATA_STORABLE_H_
#define __DATA_STORABLE_H_

#include <track-select>
#include <pqxx/pqxx>
namespace track_select {
  namespace data {
    class Storable {

      bool has_id_;
      database_id id_;

    protected:
      /** \breif Sets the id of a data set. If data set already has id it will
       * throw exception.
       * \params id - a unique identifier for the data set.
       * \thorw IdAlreadyAssigned();
       */


    public:
      Storable();
      Storable(database_id id);
      Storable(const Storable& obj);
      virtual ~Storable();

      virtual Storable& setId(database_id id);
      /** Equivalence operator */
      bool operator==(const Storable& obj) const;

      /** Assigment */
      Storable& operator=(const Storable& obj);


      /** \breif The id is unique identifier for the data set. It is used to
       *  refer to particular data set extracted from storage.
       */
      virtual database_id id() const;

      /** \return true if id already assigned. Otherwise false
       */
      virtual bool hasId() const;
      virtual database_id store(pqxx::work& work) const = 0;
      virtual void clearId();

    };
  }
}
#endif
