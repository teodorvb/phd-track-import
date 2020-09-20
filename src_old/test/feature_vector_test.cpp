#include "data/feature_vector.hpp"
#include "fixtures.hpp"

namespace td = track_select::data;
namespace ts = track_select;
int main(int argc, char ** argv) {
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

  td::FeatureVector seq;
  if (seq.dimSet()) return -1;
  if (seq.sourceDataSetSet()) return -1;
  if (seq.sourceTrackIdSet()) return -1;
  if (seq.sourceRangeSet()) return -1;
  if (seq.categorySet()) return -1;

  ts::Vector frame = ts::Vector::Random(5);
  seq = frame;

  if (!seq.dimSet()) return -1;
  if (seq.dim() != 5) return -1;

  seq.setSourceDataSet("test");
  if (!seq.sourceDataSetSet()) return -1;
  if (seq.sourceDataSet() != "test") return -1;

  seq.setSourceTrackId(5);
  if (!seq.sourceTrackIdSet()) return -1;
  if (seq.sourceTrackId() != 5) return -1;

  seq.setSourceRange(ts::msmm::CustomRange(4, 5));
  if (!seq.sourceRangeSet()) return -1;
  if (seq.sourceRange() != ts::msmm::CustomRange(4, 5)) return -1;

  seq.setCategory(4);
  if (!seq.categorySet()) return -1;
  if (seq.category() != 4) return -1;

  try {

    seq = ts::Vector::Random(10);
    return -1;
  } catch (ts::ErrorInconsistentDim e) {
    if (e.what() != std::string("FeatureVector: dim of assignment should be 5 "
                                "but it is 10")) return -1;
  }

  if (seq.dim() != 5) return -1;

  try {
    seq.setDim(50);
    return -1;
  } catch (ts::ErrorAttributeAlreadySet e) {
    if (e.what() != std::string("DataPoint::dim_")) return -1;
  }

  if (seq.dim() != 5) return -1;

  td::FeatureVector clone;

  if (clone == seq) return -1;

  clone.setDim(5);
  if (clone == seq) return -1;

  clone.setSourceDataSet("test");
  if (clone == seq) return -1;

  clone.setSourceTrackId(5);
  if (clone == seq) return -1;

  clone.setSourceRange(ts::msmm::CustomRange(4, 5));
  if (clone == seq) return -1;

  clone.setCategory(4);
  if (clone == seq) return -1;

  clone = frame;
  if (!(clone == seq)) return -1;

  clone.setSourceDataSet("da");
  if (clone == seq) return -1;

  clone.setSourceDataSet("test");
  clone.setSourceTrackId(231);
  if (clone == seq) return -1;

  clone.setSourceTrackId(5);
  clone.setCategory(1);
  if (clone == seq) return -1;

  clone.setCategory(4);
  clone.setSourceRange(ts::msmm::CustomRange(20, 50));
  if (clone == seq) return -1;

  clone(2) -= 2;
  if (clone == seq) return -1;

  td::FeatureVector clone1 = seq;
  if (!(clone1 == seq)) return -1;


  td::FeatureVector clone2;
  clone2 = seq;
  if (!(clone2 == seq)) return -1;

  seq.setForeignKeyId(1);
  ts::database_id id;
  {
    pqxx::work w(conn);
    id = seq.store(w);
    w.commit();
  }

  td::FeatureVector from_db;
  {
    pqxx::work w(conn);
    from_db = td::FeatureVector::read(id, w);
    w.commit();
  }

  if (!(from_db == seq)) return -1;


  return 0;
}
