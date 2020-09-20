#include "objective/test_function.hpp"

namespace track_select {
  namespace objective {
    TestFunction::TestFunction(const TestFunction& obj)
      : ObjectiveFunc((const ObjectiveFunc&)obj),
        type_(obj.type_) {};

    TestFunction::TestFunction(unsigned int p_size, TestFunction::FitnessType t)
      : ObjectiveFunc(p_size, 2),
        type_(t) {}

    TestFunction::~TestFunction() {};

    TestFunction* TestFunction::clone() const {
      return new TestFunction(*this);
    }

    TestFunction& TestFunction::operator=(const TestFunction& obj) {
      ObjectiveFunc::operator=((const ObjectiveFunc&)obj);
      type_ = obj.type_;
      return *this;
    }

    std::vector<optimizer::OptimizerReportItem>
    TestFunction::operator()(const Matrix& pop) const {
      std::vector<optimizer::OptimizerReportItem> items;
      unsigned int p_size = populationSize();

      for (unsigned int i = 0; i < p_size; i++)
        items.push_back((*this)(Vector(pop.col(i))));

      return items;
    }


    optimizer::OptimizerReportItem
    TestFunction::operator()(const Vector& params) const {
      optimizer::OptimizerReportItem item;
      item.setVector(params);

      switch (type()) {

      case MICHALEWICZ:
        item.setFitness(michalewicz(params));
        break;

      case SHEKEL_SYMETRIC:
        item.setFitness(shekel_symetric(params));
        break;

      case SHEKEL_ASYMETRIC:
        item.setFitness(shekel_asymetric(params));
        break;
      case GRIEWANK:
        item.setFitness(griewank(params));
        break;

      default:
        std::stringstream ss;
        ss << "Fitness type" << type_ << " is unknown";
        throw std::runtime_error(ss.str());
      }

      return item;
    }


    std::ostream& operator<<(std::ostream& out,
                             const TestFunction::FitnessType& type) {
      switch (type) {
      case TestFunction::MICHALEWICZ:
        out << "michalewicz";
        break;
      case TestFunction::SHEKEL_SYMETRIC:
        out << "shekel_symetric";
        break;
      case TestFunction::SHEKEL_ASYMETRIC:
        out << "shekel_asymetric";
        break;
      case TestFunction::GRIEWANK:
        out << "griewank";
        break;

      default:
        std::stringstream ss;
        ss << "Fitness type" << type << " is unknown";
        throw std::runtime_error(ss.str());
      }

      return out;
    }

    std::istream& operator>>(std::istream& in,
                             TestFunction::FitnessType& type) {
      std::string str;
      in >> str;
      if (str == "michalewicz")
        type = TestFunction::MICHALEWICZ;

      else if (str == "shekel_symetric")
        type = TestFunction::SHEKEL_SYMETRIC;

      else if (str == "shekel_asymetric")
        type = TestFunction::SHEKEL_ASYMETRIC;
      else if (str == "griewank")
        type = TestFunction::GRIEWANK;

      else {
        std::stringstream ss;
        ss << "Fitness type" << str << " is unknown";
        throw std::runtime_error(ss.str());
      }

      return in;
    }


  }
}
