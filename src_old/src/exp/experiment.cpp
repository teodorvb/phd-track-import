#include "exp/experiment.hpp"

namespace track_select {
  namespace exp {

    ErrorExperimentPropertyNotSet::
    ErrorExperimentPropertyNotSet(const char * str)
      : runtime_error(str) {}

    ErrorExperimentPropertyNotSet::
    ErrorExperimentPropertyNotSet(const std::string& str)
      : runtime_error(str) {}


    Experiment& Experiment::addDataSet(std::string k, data::Storable* data) {
      data_set_mutex_.lock();
      data_sets_[k] = data;
      data_set_mutex_.unlock();
      return *this;
    }

    Experiment& Experiment::setInfo(std::string info) {
        info_mutex_.lock();
        info_ = funcs::splitToLines80(info);
        info_mutex_.unlock();
        return *this;
      }

    Experiment& Experiment::addResult(std::string k, real v, unsigned int tid) {
      result_mutex_.lock();
      results_.push_back(std::make_tuple(k, v, tid));
      result_mutex_.unlock();

      return *this;
    }

    Experiment& Experiment::addLog(std::string k, real v, unsigned int tid) {
      log_mutex_.lock();
      logs_[tid][k].push_back(v);
      log_mutex_.unlock();
      return *this;
    }

    Experiment& Experiment::addPlot(std::string k, std::string v,
                                    unsigned int tid) {

      plot_mutex_.lock();
      plots_.push_back(std::make_tuple(k, v, tid));
      plot_mutex_.unlock();
      return *this;
    }

    const data::Storable* Experiment::dataSet(std::string key) const {
      return data_sets_.at(key);
    }


    /** \breif Notify number of samples and number of threads
     *  \param samples - number of times to repeat the experiment
     *  \param threads - how many threads to use to run the experiment
     *  \note If threads is larger then sampels it will be changed to be equal
     *  to samples.
     */
    Experiment::Experiment(unsigned int samples)
      : samples_(samples) {
      logs_ = experimentLog(samples);
    }

    Experiment::~Experiment() {}

    Experiment&
    Experiment::run(void (*exp)(Experiment*, unsigned int, std::mutex&)) {
      std::vector<std::thread> threads;

      for (unsigned int i = 0; i <  samples(); i++)
        threads.push_back(std::thread(exp, this, i, std::ref(run_mutex_)));

      for (auto& t : threads)
        t.join();

      return *this;
    }

    database_id Experiment::store(pqxx::work& work) const {


      work.conn().prepare("insert_experiment",
                          "insert into experiment \
(id, info, finger_print, created_at) \
values( $1, $2, $3, now())");

      work.conn().prepare("insert_property",
                          "insert into experiment_property \
(id, k, val, experiment_id) \
values( $1, $2, $3, $4)");

      work.conn().prepare("insert_result",
                          "insert into experiment_result \
(id, k, val, sample_id, experiment_id) \
values( $1, $2, $3, $4, $5)");

      work.conn().prepare("insert_plot",
                          "insert into experiment_plot \
(id, k, val, sample_id, experiment_id) \
values( $1, $2, $3, $4, $5)");

      work.conn().prepare("insert_log",
                          "insert into experiment_log \
(id, k, val, sample_id, data_size, data_type_size, experiment_id) \
values( $1, $2, $3, $4, $5, $6, $7 )");

      work.conn().prepare("insert_sequence_set",
                          "insert into experiment_sequence_set \
(id, k, experiment_id, sequence_set_id) \
values( $1, $2, $3, $4)");



      database_id db_id = work
        .exec("select nextval('experiment_id_seq')")[0][0].as<database_id>();

      work.prepared("insert_experiment")
        (db_id)
        (info())
        (fingerPrint()).exec();


      for (auto p : properties_) {
        database_id pid = work
          .exec("select nextval('experiment_property_id_seq')")[0][0]
          .as<database_id>();
        work.prepared("insert_property")
          (pid)
          (p.first)
          (p.second)
          (db_id).exec();
      }

      for (auto p : results_) {
        database_id rid = work.exec("select nextval('experiment_result_id_seq')")
          [0][0].as<database_id>();
        work.prepared("insert_result")
          (rid)
          (std::get<0>(p)) // Key
          (std::get<1>(p)) // Value
          (std::get<2>(p)) // ThreadId
          (db_id).exec();
      }

      for (auto p : plots_) {
        database_id pid = work.exec("select nextval('experiment_plot_id_seq')")
          [0][0].as<database_id>();
        work.prepared("insert_plot")
          (pid)
          (std::get<0>(p)) // Key
          (std::get<1>(p)) // Value
          (std::get<2>(p)) // ThreadId
          (db_id).exec();
      }


      for (unsigned int i = 0; i < logs_.size(); i++) {
        for (auto p : logs_[i]) {
          database_id log_id = work.exec("select nextval('experiment_log_id_seq')")
            [0][0].as<database_id>();

          unsigned int data_size = p.second.size();
          real * data = new real[data_size];

          for (unsigned int i = 0; i < data_size; i++)
            data[i] = p.second[i];

          pqxx::binarystring db_data((void*)data, data_size*sizeof(real));

          delete[] data;

          work.prepared("insert_log")
            (log_id)
            (p.first) // Key
            (db_data) // Value
            (i) // sample id
            (data_size)
            (sizeof(real))
            (db_id).exec();
        }
      }

      for (auto p : data_sets_) {
        if (!p.second->hasId())
          continue;
        database_id rid
          = work.exec("select nextval('experiment_sequence_set_id_seq')")
          [0][0].as<database_id>();

        work.prepared("insert_sequence_set")
          (rid)
          (p.first)
          (db_id)
          (p.second->id()).exec();

      }

      work.commit();
      return db_id;
    }


    std::string Experiment::info() const {
      return info_;
    }

    std::string Experiment::fingerPrint() const {
      std::string all_props;
      std::hash<std::string> digest;

      for (auto property : properties_) {
        all_props += property.first + " : " + property.second + "\n";
      }
      return std::to_string(digest(all_props));
    }

    unsigned int Experiment::samples() const {
      return samples_;
    }

    std::string Experiment::property(std::string k) const {
      try {
        return properties_.at(k);
      } catch (std::out_of_range e) {
        throw ErrorExperimentPropertyNotSet(k);
      }
    }

    std::ostream& operator<<(std::ostream& out, Experiment& e) {
      for (auto& tuple : e.results_)
        out << "(" << std::get<0>(tuple) << ","
            << std::get<1>(tuple) << ")" << std::endl;
      return out;
    }
  }
}
