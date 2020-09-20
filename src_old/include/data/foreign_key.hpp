#ifndef __DATA_FOREIGN_KEY_H_
#define __DATA_FOREIGN_KEY_H_


#include "data/storable.hpp"


namespace track_select {
  namespace data {

    class ForeignKey {
      database_id m_foreign_key_id;
      bool m_has_foreign_key_id;
      const Storable * m_owner;
      bool m_owner_set;
    public:
      ForeignKey();
      ForeignKey(const ForeignKey&);
      virtual ~ForeignKey();

      virtual ForeignKey& operator=(const ForeignKey& obj);

      virtual bool operator==(const ForeignKey& obj) const;

      virtual ForeignKey& setForeignKeyId(database_id id);
      virtual bool hasForeignKeyId() const;
      virtual database_id foreignKeyId() const;

      virtual bool isOwnerSet() const;

      virtual ForeignKey& setOwner(const Storable* owner);
      virtual const Storable* owner() const;
    };

  }
}

#endif
