#include "tools/command_line.hpp"
#include<iostream>

namespace track_select {
  namespace tools {
    
    bool checkArgument(boost::program_options::variables_map& vm,
                       boost::program_options::options_description& desc,
                       std::string arg) {
      if (!vm.count(arg.c_str())) {
        std::cerr << "Error: missing argument --" << arg << "." << std::endl
                  << std::endl
                  << "Usage" << desc << std::endl;
        return false;
      }
      return true;
    }

  }
}
