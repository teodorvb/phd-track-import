#include "algorithm.hpp"
#include <boost/lexical_cast.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

namespace track_select {
  namespace algorithm {
    boost::uuids::random_generator uuid_gen;

    std::string makeUUID() {
      return boost::lexical_cast<std::string>(uuid_gen());
    }

  }
}
