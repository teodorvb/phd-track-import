#ifndef __TEST_FIXTURES_H_
#define __TEST_FIXTURES_H_

#include <string>
#include <pqxx/pqxx>

namespace track_select {
  namespace test {

    class ErrorFixtureNotFound : std::runtime_error {
    public:
      explicit ErrorFixtureNotFound(const std::string& str);
      explicit ErrorFixtureNotFound(const char* str);
    };

    void buildFixtures(int argc, char** argv, pqxx::work& w);
    void destroyFixtures(int argc, char** argv, pqxx::work& w);
  }
}

#endif
