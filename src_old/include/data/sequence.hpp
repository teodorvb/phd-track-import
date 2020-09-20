#ifndef __DATA_SEQUENCE_H_
#define __DATA_SEQUENCE_H_

#include "data/storable.hpp"
#include "data/foreign_key.hpp"
#include "data/data_point.hpp"

namespace track_select {

  namespace data {

    /** \breif Container for a raw sequence. Raw sequences can be transformed in
     * order to make the classification easier.
     */
    class Sequence : public DataPoint {

    public:

      typedef std::vector<Vector>::const_iterator const_iterator;
      typedef std::vector<Vector>::iterator iterator;

    private:
      std::vector<Vector> eigen_data_;

      template<class data_type>
      void load(const pqxx::tuple& data_point);

    protected:
      virtual pqxx::binarystring data() const;
      Sequence(const pqxx::tuple& t);
    public:
      /** Creates empty sequence */
      Sequence();

      /** \usage
       * \param seq reference to another Sequence.
       * \note This constructor creates clone of the object dp rather than just
       * copying the references. If the sequence is large this can be expensive.
       */
      Sequence(const Sequence& seq);

      /**\usage
       * \param track_id - the id of the track withing the data set. Track from
       * differnet data sets can have the same id.
       * \param dim - the dimensionality of the vectors in the sequence.
       */
      Sequence(unsigned int dim);


      virtual ~Sequence();

      /** \usage
       * \param seq reference to another Sequence.
       * \note Creates clone of the object seq rather than just copying the
       * references. If the object is very large this may be expenseve operation.
       */
      virtual Sequence& operator=(const Sequence& seq);

      /** \breif Tests two sequences for equality.
       * \note The order of the sequences is imporant.
       */
      virtual bool operator==(const Sequence& rs) const;



      /** \name Element access */
      /** @{ */

      /** \return a reference to the frame at position i. If i is out of bound
       * throws exception IndexOutOfBound()
       * \warning This is not copy of the data, but reference.
       */
      virtual const Vector& operator[](unsigned int i) const;

      /** @} */

      /** \breif Stores new frame to the sequence.
       * \param freme Reference to the frame to store. The data will be copyied by
       * value.
       * \notice The data in frame will be copyed by value.
       */
      virtual Sequence& operator<<(const Vector& frame);
      virtual unsigned int size() const;

      /** \breif Read a sequence from data base */
      static Sequence read(database_id id, pqxx::work& w);

      virtual const_iterator begin() const;
      virtual const_iterator end() const;

      virtual iterator begin();
      virtual iterator end();

    };

    template<class data_type>
    void Sequence::load(const pqxx::tuple& data_point) {
      unsigned int dim = data_point["frame_dim"].as<unsigned int>();
      unsigned int size = data_point["track_length"].as<unsigned int>();

      pqxx::binarystring data(data_point["data"]);

      unsigned int index = 0;
      const data_type* d = (const data_type*)data.data();

      for (unsigned int s_index = 0; s_index < size; s_index++) {
        Vector frame(dim);
        for (unsigned int f_index = 0; f_index < dim; f_index++)
          frame(f_index) = d[index++];
        (*this) << frame;
      }

    }

    std::ostream& operator<<(std::ostream& out, const Sequence& s);
  } // data
}  // track_select

#endif
