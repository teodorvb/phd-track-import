#include "data/foreign_key.hpp"

namespace track_select {
  namespace data {

    ForeignKey::ForeignKey()
      : m_has_foreign_key_id(false),
        m_owner(NULL),
        m_owner_set(false) {}

    ForeignKey::ForeignKey(const ForeignKey& obj)
      : m_foreign_key_id(obj.m_foreign_key_id),
        m_has_foreign_key_id(obj.m_has_foreign_key_id),
        m_owner(obj.m_owner),
        m_owner_set(obj.m_owner_set) {}


    ForeignKey::~ForeignKey() {}


    ForeignKey& ForeignKey::operator=(const ForeignKey& obj) {
      m_foreign_key_id = obj.m_foreign_key_id;
      m_has_foreign_key_id = obj.m_has_foreign_key_id;

      m_owner = obj.m_owner;
      m_owner_set = obj.m_owner_set;

      return *this;
    }

    bool ForeignKey::operator==(const ForeignKey& obj) const {
      if (m_has_foreign_key_id != obj.m_has_foreign_key_id)
        return false;

      if (m_has_foreign_key_id && (m_foreign_key_id != obj.m_foreign_key_id))
        return false;

      if (m_owner_set != obj.m_owner_set)
        return false;

      if (m_owner_set && (m_owner != obj.m_owner))
        return false;

      return true;
    }


    ForeignKey& ForeignKey::setForeignKeyId(database_id id) {
      if (hasForeignKeyId())
        throw ErrorAttributeAlreadySet("ForeignKey::foreign_key_id_");

      m_foreign_key_id = id;
      m_has_foreign_key_id = true;

      return *this;
    }

    database_id ForeignKey::foreignKeyId() const {
      if (!hasForeignKeyId())
        throw ErrorAttributeNotSet("ForeignKey::foreign_key_id_");

      return m_foreign_key_id;
    }

    bool ForeignKey::hasForeignKeyId() const {
      return m_has_foreign_key_id;
    }

    ForeignKey& ForeignKey::setOwner ( const Storable* owner ) {

      m_owner = owner;
      m_owner_set = true;
      return *this;
    }


    const Storable* ForeignKey::owner() const {
      if (!isOwnerSet())
        throw ErrorAttributeNotSet("ForeignKey::owner_");
      return m_owner;
    }

    bool ForeignKey::isOwnerSet() const {
      return m_owner_set;
    }

  }
}
