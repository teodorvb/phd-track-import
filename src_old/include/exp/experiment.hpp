#ifndef __EXP_EXPERIMENT_H_
#define __EXP_EXPERIMENT_H_

#include <track-select>
#include <map>
#include <vector>
#include <string>
#include <mutex>
#include <tuple>
#include <stdexcept>
#include <thread>

#include <pqxx/pqxx>
#include "data/data_set.hpp"

namespace track_select{
  namespace exp {

    class ErrorExperimentPropertyNotSet : public std::runtime_error {
    public:
      explicit ErrorExperimentPropertyNotSet(const char * str);
      explicit ErrorExperimentPropertyNotSet(const std::string& str);
    };

    class Experiment  : public data::Storable {
    public:

      friend std::ostream& operator<<(std::ostream& out, Experiment& e);

    private:
      typedef std::vector<std::map<std::string, std::vector<real> > > experimentLog;
      std::map<std::string, std::string> properties_;
      // The results is a multiset of triples (key, value, thread_id)
      std::vector<std::tuple<std::string, real, unsigned int> > results_;

      std::vector<std::tuple<std::string, std::string, unsigned int>> plots_;

      experimentLog logs_;
      std::map<std::string, Storable* > data_sets_;

      std::string info_;

      unsigned int samples_;

      std::mutex log_mutex_;
      std::mutex result_mutex_;
      std::mutex property_mutex_;
      std::mutex data_set_mutex_;
      std::mutex info_mutex_;
      std::mutex plot_mutex_;
      std::mutex run_mutex_;
    public:
      template<class key_type>
      Experiment& addProperty(std::string, key_type v);

      Experiment& addDataSet(std::string k, Storable* data);
      Experiment& setInfo(std::string info);
      Experiment& addResult(std::string k, real v, unsigned int tid);
      Experiment& addLog(std::string k, real v, unsigned int tid);
      Experiment& addPlot(std::string k, std::string val, unsigned int tid);
      const Storable* dataSet(std::string key) const;


      /** \breif Notify number of samples and number of threads
       *  \param samples - number of times to repeat the experiment
       *  \param threads - how many threads to use to run the experiment
       *  \note If threads is larger then sampels it will be changed to be equal
       *  to samples.
       */
      Experiment(unsigned int samples);
      virtual ~Experiment();

      virtual Experiment&
      run(void (*exp)(Experiment*, unsigned int, std::mutex&));

      virtual database_id store(pqxx::work& work) const;
      virtual std::string info() const;
      virtual std::string fingerPrint() const;
      virtual unsigned int samples() const;
      virtual std::string property(std::string k) const;

    };

    template<class key_type>
    Experiment& Experiment::addProperty(std::string k, key_type v) {
      std::stringstream ss;
      ss << v;

      property_mutex_.lock();
      properties_[k] = ss.str();
      property_mutex_.unlock();

      return *this;
    }


  }
}

#endif //__EXP_EXPERIMENT_H_
