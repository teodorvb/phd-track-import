#ifndef __DATA_DATA_SET_H_
#define __DATA_DATA_SET_H_
#include "data/storable.hpp"
#include<mutex>

namespace track_select {
  namespace data {
    class ErrorNullClass {};

    template <class data_point>
    class DataSet : public Storable {
    public:
      typedef typename std::vector<data_point> data_container;

      /** \breif Constant forward iterator of for the container. */
      typedef typename data_container::const_iterator const_iterator;
      typedef typename data_container::iterator iterator;



    private:
      DataSet<data_point>& addDataPointWithId(const data_point& point);
      std::mutex lock_;
      data_container data_points_;

      bool info_set_;
      std::string info_;

      bool frame_info_set_;
      std::vector<std::string> frame_info_;

      bool dim_set_;
      unsigned int dim_;


      void setInfoUnsafe(const std::string& info);
      void setFrameInfoUnsafe(const std::vector<std::string>& frame_info);
      void setDimUnsafe(unsigned int dim);

      DataSet(database_id id,
              const std::string& info,
              const std::vector<std::string>& frame_info);

    public:
      DataSet();
      DataSet(const DataSet& obj);
      DataSet(const std::string& info);
      DataSet(const std::string& info, const std::vector<std::string>& frame_info);

      ~DataSet();
      DataSet& operator=(const DataSet & obj);

      /** @name Attributes */
      /** @{ */
      /** @breif Human readable information about the data set.
       *  @note It is must be set before storage
       */
      const std::string& info() const;

      /** @breif Information of what each dimension represents. It is optional
       *  argument, since the data point could be image. Then it is clear that
       *  all dimensions represent pixel values
       */
      const std::vector<std::string>& frameInfo() const;

      /** @breif The dimensionality of each data point. If it is a vector set
       *  then this is the dimensionality of the each vector. If it is a
       *  sequence set then this is the dimensionality of each sequence
       */
      unsigned int dim() const;

      /** @breif The size of the data set. It is determined by the number of
       *  data points.
       */
      unsigned int size() const;


      bool dimSet() const;
      /** @breif Sets dimensionality
       *  @note Thread safe. */
      void setDim(unsigned int dim);

      /** @breif Sets dimensionality of not set. Otherwise does nothing
       * @note Thread safe. */
      void setDimIfNotSet(unsigned int dim);

      bool infoSet() const;
      /** @breif Sets dataset info.
       *  @note Thread safe. */
      void setInfo(const std::string& info);

      /* @breif Sets info if not set. Otherwise does nothing
       * @note Thread Safe */
      void setInfoIfNotSet(const std::string& info);

      bool frameInfoSet() const;

      /** @breif Sets information about each dimension of the frames.
       *  @note Thread safe. */
      void setFrameInfo(const std::vector<std::string>& frame_info);

      /** @breif Sets information about each dimension of the frames if it is not set.
       *   Otherwise it does nothing.
       *  @note Thread safe. */
      void setFrameInfoIfNotSet(const std::vector<std::string>& frame_info);

      /** @} */

      /** @name Container Access */
      /** @{ */
      const_iterator begin() const;
      const_iterator end() const;
      const_iterator cbegin() const;
      const_iterator cend() const;
      iterator begin();
      iterator end();

      data_point& operator[](unsigned int index);
      const data_point& operator[](unsigned int index) const;
      /** @} */

      /** @name Container Modifiers */
      /** @{ */

      /** @breif Adds a data point to the container. It uses std::mutex to
       *   guarantee thread safety. */
      DataSet<data_point>& operator<<(const data_point& point);

      iterator erase(iterator el);
      iterator erase(iterator begin, iterator end);
      /** @} */

      /** @name Storage Access */
      /** @{ */
      database_id store(pqxx::work& work) const;
      static DataSet<data_point> read(database_id id, pqxx::work& work);
      /** @} */

      static void zero_class_check(const DataSet<data_point>& v_set);


    };
  }
}

#endif
