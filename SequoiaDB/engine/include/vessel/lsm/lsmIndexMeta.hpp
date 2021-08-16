/*******************************************************************************


   Copyright (C) 2011-2018 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU Affero General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

   Source File Name = index.hpp

   Descriptive Name = index metadata

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          03/03/2021  JT  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef LSMINDEXMETA_HPP_
#define LSMINDEXMETA_HPP_

#include "vessel/globalIndexID.h"
#include "rocksdb/rocksdb_namespace.h"
#include "vessel/orderingWrapper.h"

namespace engine
{
namespace vessel
{

class lsmIndexMeta : public SDBObject
{
public:
  lsmIndexMeta(){}
  explicit lsmIndexMeta
  (
     const globalIndexID & idxId,
     const orderingWrapper & ordering
  ):
  _idxId(idxId),
  _ordering(ordering)
  {
     SDB_ASSERT( (idxId.isValid()), "Invalid IndexID" ) ;
  }

  virtual ~lsmIndexMeta()
  {
  }

  lsmIndexMeta& operator= ( const lsmIndexMeta &rhs )
  {
     _idxId      = rhs._idxId ;
     _ordering   = rhs._ordering;
     return *this ;
  }

  OSS_INLINE const globalIndexID &getIdxId() const { return _idxId; }
  OSS_INLINE const bson::Ordering *getBsonOrdering() const
  { return _ordering.toBsonOrdering(); }

  OSS_INLINE const orderingWrapper &getOrdering()const
  {
     return _ordering;
  }
  //OSS_INLINE BSONObj getKeyPattern() const { return _keyPattern ; }
  OSS_INLINE BOOLEAN isValid() const
  {
     return _idxId.isValid() ;
  }
protected:
  globalIndexID      _idxId ;  // { UINT32 _csID, UINT32 _clID, UINT32 _idxLID }
  orderingWrapper    _ordering;
};


} // namespace vessel
} // namespace engine
#endif  //  LSMINDEXMETA_HPP_
