#include "fixtures.hpp"
#include <boost/filesystem.hpp>
#include <fstream>
#include <boost/tokenizer.hpp>
#include <boost/algorithm/string.hpp>

namespace track_select {
  namespace test {
    ErrorFixtureNotFound::ErrorFixtureNotFound(const std::string& str)
      : runtime_error(str) {}
    ErrorFixtureNotFound::ErrorFixtureNotFound(const char* str)
      : runtime_error(str) {}


    void execQueriesFromPath(boost::filesystem::path& fixture_path, pqxx::work& work) {
      std::ifstream in(fixture_path.string().c_str());
      std::stringstream queries_buffer;
      queries_buffer << in.rdbuf();
      in.close();

      std::string queries = queries_buffer.str();

      boost::tokenizer<boost::char_separator<char>>
        tok(queries, boost::char_separator<char>(";"));

      for (const auto &query : tok) {
        if (std::all_of(query.begin(), query.end(), [](char c)->bool {
              return c == ' ' || c == '\n' || c == '\t';
            })) continue;
        work.exec(query);
      }

    }

    void buildFixtures(int argc, char** argv, pqxx::work& work) {
      if (argc < 2)
        throw ErrorFixtureNotFound("Fixture path not passed as argument");
      boost::filesystem::path p(argv[1]);
      p/= "build.sql";
      execQueriesFromPath(p, work);
    };

    void destroyFixtures(int argc, char** argv, pqxx::work& work) {
      if (argc < 2)
        throw ErrorFixtureNotFound("Fixture path not passed as argument");
      boost::filesystem::path p(argv[1]);
      p/= "destroy.sql";
      execQueriesFromPath(p, work);

    }
  }
}
