#include <sstream>
#include <iostream>
#include "data/sequence_set.hpp"
#include <fstream>

namespace ts = track_select;


void gnuplot(std::string str) {
  std::ofstream tmp("tmp");
  tmp << str;

  tmp.close();
  std::system("gnuplot tmp - && rm tmp");

}

int main(int argc, char** argv) {
  if (argc < 2) return -1;

  unsigned int id;
  unsigned int dim;
  std::stringstream ss;
  ss << argv[1];
  ss >> id;

  ts::data::SequenceSet data;
  {
    pqxx::connection c;
    pqxx::work w(c);
    data = ts::data::SequenceSet::read(id, w);
  }

  std::stringstream script;
  script << "$intensity <<EOF" << std::endl;
  for (auto& sequence : data)
    for (auto& frame : sequence)
      script << frame(0) << std::endl;

  script << "EOF" << std::endl;

  script << "$bg <<EOF" << std::endl;
  for (auto& sequence : data)
    for (auto& frame : sequence)
      script << frame(3) << std::endl;

  script << "EOF" << std::endl;


  script << "$pos_x <<EOF" << std::endl;
  for (auto& sequence : data)
    for (auto& frame : sequence)
      script << frame(1) << std::endl;

  script << "EOF" << std::endl;


  script << "$pos_y <<EOF" << std::endl;
  for (auto& sequence : data)
    for (auto& frame : sequence)
      script << frame(2) << std::endl;

  script << "EOF" << std::endl;


  script
    << "bin(x,width)=width*floor(x/width)" << std::endl

    << "set terminal qt size 1200, 900" << std::endl

    << "set multiplot layout 2,2 rowsfirst" << std::endl

    << "set xlabel 'Intensity'" << std::endl
    << "plot $intensity using (bin($1,0.1)):(1.0) smooth freq with boxes notitle" << std::endl
    << "set xlabel 'Background'" << std::endl

    << "plot $bg using (bin($1,0.01)):(1.0) smooth freq with boxes notitle" << std::endl

    << "set xlabel 'Position X'" << std::endl
    << "plot $pos_x using (bin($1,0.05)):(1.0) smooth freq with boxes notitle" << std::endl

    << "set xlabel 'Position Y'" << std::endl
    << "plot $pos_y using (bin($1,0.05)):(1.0) smooth freq with boxes notitle" << std::endl

    << "unset multiplot" << std::endl;




  gnuplot(script.str());

  return 0;
}
