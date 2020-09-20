#include "data/storable.hpp"

namespace track_select {
  namespace data {
    /* Standard Functions */
    Storable::Storable()
      : has_id_(false) {}

    Storable::Storable(database_id id)
      : has_id_(true),
        id_(id) {}

    Storable::Storable(const Storable& obj)
      : has_id_(obj.has_id_),
        id_(obj.id_) {}
    Storable::~Storable() {}

    Storable& Storable::operator=(const Storable& obj) {
      has_id_ = obj.has_id_;
      id_ = obj.id_;

      return *this;
    }

    bool Storable::operator==(const Storable& obj) const {
      if (has_id_ != obj.has_id_)
        return false;

      if (has_id_)
        return id_ == obj.id_;

      return true;
    }


    /* Protected Functions */
    Storable& Storable::setId(database_id id) {
      if (hasId())
        throw ErrorAttributeAlreadySet("Storable::id_");

      id_ = id;
      has_id_ = true;
      return *this;
    }

    /* Public Functions */

    bool Storable::hasId() const {
      return has_id_;
    }

    database_id Storable::id() const {
      if (!hasId())
        throw ErrorAttributeNotSet("Storable::id_");

      return id_;
    }


    void Storable::clearId() {
      has_id_ = false;
    }

  }
}
