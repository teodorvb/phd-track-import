#include "data/data_set.hpp"
#include "data/feature_vector.hpp"
#include "fixtures.hpp"

namespace ts = track_select;
int main(int argc, char** argv) {
  ts::data::DataSet<ts::data::FeatureVector> data_set;
  if (data_set.dimSet()) return -1;
  if (data_set.infoSet()) return -1;
  if (data_set.frameInfoSet()) return -1;
  ts::data::FeatureVector data_1(ts::Vector::Random(5));
  ts::data::FeatureVector data_2(ts::Vector::Random(5));
  data_1.setId(10);
  data_set << data_1;
  if (data_set[0].owner() != &data_set) return -1;
  if (data_set[0].hasId()) return -1;

  if (!data_set.dimSet()) return -1;
  if (data_set.dim() != 5) return -1;
  if (data_set.size() != 1) return -1;
  try {
    data_set << ts::data::FeatureVector(ts::Vector::Random(2));
    return -1;
  } catch (ts::ErrorInconsistentDim e) {
    if (e.what() != std::string("DataSet: data dim should be 5 however it "
                                "is 2")) return -1;
  }
  if (data_set.size() != 1) return -1;
  data_set << data_2;
  if (data_set.size() != 2) return -1;
  if (data_set[0] != data_1) return -1;
  if (data_set[1] != data_2) return -1;

  if (*data_set.begin() != data_1) return -1;
  if (*(data_set.begin()+1) != data_2) return -1;

  data_set.setInfo("Dummy Set");
  if (!data_set.infoSet()) return -1;
  if (data_set.info() != std::string("Dummy Set")) return -1;

  try {
    data_set.setFrameInfo({"Hello", "world"});
    return -1;
  } catch (ts::ErrorInconsistentDim e) {
    if (e.what() != std::string("DataSet: info dim should be 5 however it "
                                "is 2")) return -1;
  }
  std::vector<std::string> frame_info = {"Dummy 1",
                                         "Dummy 2",
                                         "Dummy 3",
                                         "Dummy 4",
                                         "Dummy 5"};
  data_set.setFrameInfo(frame_info);
  if (!data_set.frameInfoSet()) return -1;
  if (data_set.frameInfo() != frame_info) return -1;

  ts::data::DataSet<ts::data::FeatureVector> clone1(data_set);
  if (clone1.dim() != data_set.dim()) return -1;
  if (clone1.info() != data_set.info()) return -1;
  if (clone1.frameInfo() != data_set.frameInfo()) return -1;
  if (clone1.size() != data_set.size()) return -1;

  for (unsigned int i = 0; i < clone1.size(); i++)
    if (clone1[i] != data_set[i]) return -1;


  ts::data::DataSet<ts::data::FeatureVector> clone2;
  clone2 = data_set;

  if (clone2.dim() != data_set.dim()) return -1;
  if (clone2.info() != data_set.info()) return -1;
  if (clone2.frameInfo() != data_set.frameInfo()) return -1;
  if (clone2.size() != data_set.size()) return -1;

  for (unsigned int i = 0; i < clone2.size(); i++)
    if (clone2[i] != data_set[i]) return -1;

  pqxx::connection conn("dbname=test");
  {
    pqxx::work w(conn);
    ts::test::destroyFixtures(argc, argv, w);
    w.commit();
  }
  {
    pqxx::work w(conn);
    ts::test::buildFixtures(argc, argv, w);
    w.commit();
  }

  ts::database_id id;
  {
    pqxx::work w(conn);
    id = data_set.store(w);
    w.commit();
  }
  ts::data::DataSet<ts::data::FeatureVector> from_db;
  {
    pqxx::work w(conn);
    from_db = ts::data::DataSet<ts::data::FeatureVector>::read(id, w);
  }


  if (from_db.dim() != data_set.dim()) return -1;
  if (from_db.info() != data_set.info()) return -1;
  if (from_db.frameInfo() != data_set.frameInfo()) return -1;
  if (from_db.size() != data_set.size()) return -1;

  for (unsigned int i = 0; i < from_db.size(); i++)
    if (from_db[i] != data_set[i]) return -1;

  return 0;
}
