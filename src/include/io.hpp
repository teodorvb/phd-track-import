#ifndef __IO_H_
#define __IO_H_

#include<string>
#include<sstream>

#include<fstream>
#include<vector>
#include<tuple>

/* Read CSV files */

template<class... AllColumns>
class CSVReader {
public:
  static const unsigned short int tuple_size = sizeof...(AllColumns);
private:

  std::tuple<AllColumns...>& csv_line_;

public:
  CSVReader(std::tuple<AllColumns...>& csv_line)
    : csv_line_(csv_line) {}

  template<typename C>
  void readCSVLine_(std::tuple<C>& t, const std::vector<std::string>& d) {
    C val;
    std::stringstream ss;
    if (!(d.size() > tuple_size - 1))
      throw std::runtime_error("Reading csv file");
    ss << d[tuple_size - 1];
    ss >> val;
    std::get<tuple_size - 1>(csv_line_) = val;
  }


  template<class C, class... Columns>
  void readCSVLine_(std::tuple<C, Columns...>&t, const std::vector<std::string>& d) {
    std::tuple<Columns...> tuple;
    readCSVLine_<Columns...>(tuple, d);

    const unsigned short int Index = sizeof...(Columns);
    C val;
    std::stringstream ss;
    if (!(d.size() > tuple_size - Index - 1))
      throw std::runtime_error("Reading csv file");
    ss << d[tuple_size - Index - 1];
    ss >> val;

    std::get<tuple_size - Index - 1>(csv_line_) = val;
  }


  void readCSVLine(const std::vector<std::string>& d) {
    readCSVLine_<AllColumns...>(csv_line_, d);
  }

};

template<typename... Columns>
void readCSV(std::string fname, std::vector<std::tuple<Columns...>>& data) {

  std::ifstream csv(fname);
  std::string line;

  while(std::getline(csv, line)) {
    std::stringstream linestream(line);
    std::vector<std::string> result;

    while( linestream.good() ) {
      std::string token;
      std::getline( linestream, token, ',' );
      result.push_back(token);
    }

    std::tuple<Columns...> record;
    CSVReader<Columns...> reader(record);
    reader.readCSVLine(result);

	
    data.push_back(record);
  }
      
}


/* Read and write Base64 */

template <class data_type>                                                      
std::string write_binary(data_type* dt, unsigned int len) {                     
  unsigned int dsize = sizeof(data_type);                                       
  unsigned char* p = (unsigned char*)dt;                                        
                                                                                
  std::stringstream ss;                                                         
  for (unsigned int i = 0; i < dsize*len; i++)                                  
    ss << p[i];                                                                 

  return ss.str();                                                              
}


static const char b64_table[65] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static const char reverse_table[128] = {                                        
  64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,               
  64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,               
  64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 62, 64, 64, 64, 63,               
  52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 64, 64, 64, 64, 64, 64,               
  64,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,               
  15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 64, 64, 64, 64, 64,               
  64, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,               
  41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 64, 64, 64, 64, 64                
}; 


std::string base64_encode(const std::string &bindata);
std::string base64_decode(const ::std::string &ascdata);

#endif
