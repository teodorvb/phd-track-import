#ifndef __OBJECTIVE_CLASSIFY_H_
#define __OBJECTIVE_CLASSIFY_H_
#include <track-select>
#include <set>

namespace track_select {
  namespace objective {
    template<class data_container>
    unsigned int calcCategoriesCount(const data_container& data) {
      std::set<unsigned short> categories;
      for (auto& sample : data)
        categories.insert(sample.category());
      return categories.size();
    }

    class Classify : public optimizer::ObjectiveFunc {
    public:
      enum FitnessType {
        ACCURACY,
        F1_SCORE,
        MATTEWS,
        PRECISION_ONLY,
        PRECISION_PRIORITY,
        SENSITIVITY_SPECIFICITY,
        LOGPROB
      };

    private:
      FitnessType fitness_type_;
      unsigned int input_dim_;
      unsigned int categories_count_;
      bool parallel_exec_;
      mutable unsigned int generation_counter_;
    public:
      Classify(unsigned int p_size,
               unsigned int v_size,
               FitnessType fitness_type,
               unsigned int input_dim,
               unsigned int categories_count);
      Classify(const Classify& obj);

      virtual Classify& operator=(const Classify& obj);

      virtual std::vector<optimizer::OptimizerReportItem>
      operator()(const Matrix& pop) const;

      virtual optimizer::OptimizerReportItem
      operator()(const Vector& params) const = 0;

      virtual optimizer::OptimizerReportItem
      buildItem(const std::vector<std::tuple<unsigned short, Vector>> &cls) const;

      virtual void setParallelExec(bool exec);

      inline FitnessType type() const;
      inline unsigned int categoriesCount() const;
      inline unsigned int inputDim() const;
      inline bool parallelExec() const;

      virtual Classify* clone() const = 0;
    };

    std::ostream& operator<<(std::ostream& out, Classify::FitnessType& type);
    std::istream& operator>>(std::istream& in, Classify::FitnessType& type);


    Classify::FitnessType Classify::type() const {
      return fitness_type_;
    }

    unsigned int Classify::categoriesCount() const {
      return categories_count_;
    }

    unsigned int Classify::inputDim() const {
      return input_dim_;
    }

    bool Classify::parallelExec() const {
      return parallel_exec_;
    }

  }
}

#endif
