#ifndef __TOOLS_COMMAND_LINE_H_
#define __TOOLS_COMMAND_LINE_H_

#include <boost/program_options.hpp>

namespace track_select {
  namespace tools {
    bool checkArgument(boost::program_options::variables_map& vm,
                       boost::program_options::options_description& desc,
                       std::string arg);
  }
}
#endif
