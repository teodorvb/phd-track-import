#ifndef __OBJECTIVE_TEST_FUNCTION_H_
#define __OBJECTIVE_TEST_FUNCTION_H_

#include <track-select>
#define PI 3.14159265359

namespace track_select {
  namespace objective {
    class TestFunction : public optimizer::ObjectiveFunc {
    public:
      enum FitnessType {
        MICHALEWICZ,
        SHEKEL_SYMETRIC,
        SHEKEL_ASYMETRIC,
        GRIEWANK,
      };

      inline real michalewicz(const Vector& params) const;
      inline real shekel_symetric(const Vector& params) const;
      inline real shekel_asymetric(const Vector& params) const;
      inline real griewank(const Vector& params) const;
    private:
      FitnessType type_;
    public:
      TestFunction(unsigned int p_size, FitnessType type);
      TestFunction(const TestFunction& obj);
      virtual ~TestFunction();

      virtual TestFunction& operator=(const TestFunction& obj);

      virtual std::vector<optimizer::OptimizerReportItem>
      operator()(const Matrix& pop) const;

      virtual optimizer::OptimizerReportItem
      operator()(const Vector& params) const;

      virtual TestFunction* clone() const;

      inline FitnessType type() const;
    };


    std::ostream& operator<<(std::ostream& out,
                             const TestFunction::FitnessType& type);


    std::istream& operator>>(std::istream& in,
                             TestFunction::FitnessType& type);


    /* ============= Inline Functions ================ */


    real TestFunction::michalewicz(const Vector& params) const {
      real x = std::abs(params(0)) - PI*std::floor(std::abs(params(0))/PI);
      real y = std::abs(params(1)) - PI*std::floor(std::abs(params(1))/PI);

      return std::sin(x) * std::pow(std::sin(std::pow(x, 2)/PI), 20) +
        std::sin(y) * std::pow(std::sin(2* std::pow(y, 2)/PI), 20);
    }

    real TestFunction::shekel_symetric(const Vector& params) const {
      real x = params(0);
      real y = params(1);

      return std::pow(1 + std::pow(x - 10, 2) + std::pow(y - 10, 2), -1) +
        std::pow(1 + std::pow(x + 10, 2) + std::pow(y + 10, 2), -1);
    }

    real TestFunction::shekel_asymetric(const Vector& params) const {
      real x = params(0);
      real y = params(1);

      return std::pow(1 + std::pow(x - 10, 2) + std::pow(y - 10, 2), -1) +
        std::pow(1 + std::pow(x + 10, 2) + std::pow(y + 10, 2), -1) +
        std::pow(1 + std::pow(x - 10, 2) + std::pow(y + 10, 2), -1) +
        std::pow(1 + std::pow(x + 10, 2) + std::pow(y - 10, 2), -1) +
        std::pow(.5 + std::pow(x + 25, 2) + std::pow(y + 25, 2), -1);
    }

    real TestFunction::griewank(const Vector& params) const {
      Vector p = params - Vector::Ones(params.rows())*15;

      real sum = (p.cwiseAbs2()/4000).sum();
      real prod = 1;
      for (unsigned int i = 0;  i < p.size(); i++)
        prod *= std::cos(p(i)/std::sqrt(i + 1));
      return prod - sum - 1;
    }

    TestFunction::FitnessType TestFunction::type() const {
      return type_;
    }
  }
}
#endif
