#include "objective/classify.hpp"
#include "objective/common.hpp"

namespace track_select {
  namespace objective {

    Classify::Classify(unsigned int p_size,
                       unsigned int v_size,
                       Classify::FitnessType f_type,
                       unsigned int data_dim,
                       unsigned int categories)
      : ObjectiveFunc(p_size, v_size),
        fitness_type_(f_type),
        input_dim_(data_dim),
        categories_count_(categories),
        parallel_exec_(false),
        generation_counter_(0) {}

    Classify::Classify(const Classify& obj)
      : ObjectiveFunc((const ObjectiveFunc&)obj),
        fitness_type_(obj.fitness_type_),
        input_dim_(obj.input_dim_),
        categories_count_(obj.categories_count_),
        parallel_exec_(obj.parallel_exec_),
        generation_counter_(obj.generation_counter_) {}

    Classify& Classify::operator=(const Classify& obj) {
      ObjectiveFunc::operator=((const ObjectiveFunc&)obj);
      fitness_type_ = obj.fitness_type_;
      input_dim_ = obj.input_dim_;
      categories_count_ = obj.categories_count_;
      parallel_exec_ = obj.parallel_exec_;
      generation_counter_ = obj.generation_counter_;

      return *this;
    }

    std::vector<optimizer::OptimizerReportItem>
    Classify::operator()(const Matrix& pop) const {
      if (!std::isfinite(pop.sum()))
        throw ErrorNotFinite("Classify::operator() pop");
      std::cout.flush();
      unsigned int p_size = populationSize();

      std::vector<optimizer::OptimizerReportItem> items(p_size);

      auto calc = [=](unsigned int me, optimizer::OptimizerReportItem& item) {
        item = (*this)(Vector(pop.col(me)));
      };

      if (parallelExec()) {
        std::vector<std::thread> threads;
        for (unsigned int i = 0; i < p_size; i++)
          threads.push_back(std::thread(calc, i, std::ref(items[i])));

        for (auto & t : threads)
          t.join();
      } else {
        for (unsigned int i = 0; i < p_size; i++)
          calc(i, items[i]);
      }

      return items;
    }

    optimizer::OptimizerReportItem
    Classify::buildItem(const std::vector<std::tuple<unsigned short, Vector>>
                        &cls) const {
      optimizer::OptimizerReportItem item;


      Matrix confusion = Matrix::Zero(categoriesCount(), categoriesCount());
      for (const auto& c : cls) {
        Matrix::Index max_index;
        std::get<1>(c).maxCoeff(&max_index);
        confusion(std::get<0>(c), max_index)++;
      }

      item.setProperty("Accuracy", confusion.trace()/confusion.sum());
      real res = 0;
      if (type() == ACCURACY) {
        res = confusion.trace()/confusion.sum();
      } else if (type() == LOGPROB) {
        for (auto& c : cls) {
          res += std::log(std::get<1>(c)(std::get<0>(c)));
        }
        res /= cls.size();

      } else if (type() == F1_SCORE) {
        f1_score(confusion);

      } else if (type() == MATTEWS) {
        real tp = confusion(msmm::YES,msmm::YES);
        real fp = confusion(msmm::NO, msmm::YES);
        real fn = confusion(msmm::YES, msmm::NO);
        real tn = confusion(msmm::NO, msmm::NO);

        res = (tp*tn - fp * fn) / std::sqrt((tp+fp)*(tp+fn)*(tn+fp)*(tn+fn));

      } else {
        std::stringstream ss;
        ss << "Unsupported fitness " << type();
        throw std::runtime_error(ss.str());
      }

      if (std::isfinite(res))
        item.setFitness(res);
      else {
        item.setFitness(-10);
      }

      confusion.resize(std::pow(categoriesCount(), 2), 1);
      item.setData("Confusion Matrix", confusion);

      return item;
    }

    std::ostream& operator<<(std::ostream& out, Classify::FitnessType& type) {
      switch (type) {
      case Classify::ACCURACY:
        out << "accuracy";
        break;
      case Classify::F1_SCORE:
        out << "f1-score";
        break;
      case Classify::MATTEWS:
        out << "mattews";
        break;
      case Classify::PRECISION_ONLY:
        out << "precision_only";
        break;
      case Classify::PRECISION_PRIORITY:
        out << "precision_priority";
        break;
      case Classify::SENSITIVITY_SPECIFICITY:
        out << "sensitivity_specificity";
        break;
      case Classify::LOGPROB:
        out << "logprob";
        break;
      default:
        std::runtime_error("Unknow Fitness");
      }

      return out;
    }

    std::istream& operator>>(std::istream& in, Classify::FitnessType& type) {
      std::string data;
      in >> data;

      if (data == "accuracy") {
        type = Classify::ACCURACY;

      } else if (data == "f1-score") {
        type = Classify::F1_SCORE;

      } else if (data == "mattews") {
        type = Classify::MATTEWS;

      } else if (data == "precision_only") {
        type = Classify::PRECISION_ONLY;

      } else if (data == "precision_priority") {
        type = Classify::PRECISION_PRIORITY;

      } else if (data == "sensitivity_specificity") {
        type = Classify::SENSITIVITY_SPECIFICITY;

      } else if (data == "logprob") {
        type = Classify::LOGPROB;

      } else {
        throw std::runtime_error("cannot read Classify::FitnessType"
                                 " from istream: " + data);
      }

      return in;
    }


    void Classify::setParallelExec(bool exec) {
      parallel_exec_ = exec;
    }

  }
}
