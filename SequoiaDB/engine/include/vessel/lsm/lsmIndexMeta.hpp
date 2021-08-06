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

#include "vessel/lsm/lsmIdxID.hpp"  // sdbIndexID
#include "rocksdb/rocksdb_namespace.h"

using namespace bson ;
using namespace rocksdb ;

namespace engine
{
namespace vessel
{

enum SDB_INDEX_TYPE
{
   SDB_INDEX_INVALID = 0,
   SDB_INDEX_BTREE,  // btree index
   SDB_INDEX_LSM,    // lsm( rocksdb ) index
   SDB_INDEX_MAX = SDB_INDEX_LSM
};


class indexKeyOrdering : public SDBObject
{
public:
  indexKeyOrdering( const Ordering &order ) : _ordering( order ) {}
  ~indexKeyOrdering() {}
  const Ordering* getOrdering() const { return & _ordering ; }
private:
  Ordering _ordering ;
} ;


class indexMeta : public SDBObject
{
public:
  indexMeta(){ _type = SDB_INDEX_INVALID ; _pOrder = NULL; }
  indexMeta
  (
     SDB_INDEX_TYPE     type,
     const sdbIndexID & idxId,
     const BSONObj    & keyPattern
  )
  {
     SDB_ASSERT( (idxId.isValid()), "Invalid IndexID" ) ;
     _type  = type;
     _idxId = idxId ;
     _keyPattern = keyPattern.getOwned() ;
     _pOrder = SDB_OSS_NEW indexKeyOrdering( Ordering::make( _keyPattern ) ) ;
     SDB_ASSERT( (_pOrder), "Failed to construct key ordering info" ) ;
  }

  virtual ~indexMeta()
  {
     if ( _pOrder )
     {
        SDB_OSS_DEL _pOrder ;
        _pOrder = NULL ;
     }
  }

  indexMeta& operator= ( const indexMeta &rhs )
  {
     _type       = rhs._type ;
     _idxId      = rhs._idxId ;
     _keyPattern = rhs._keyPattern.getOwned();
     _pOrder     = SDB_OSS_NEW indexKeyOrdering(Ordering::make( _keyPattern ));
     return *this ;
  }

  OSS_INLINE SDB_INDEX_TYPE getIdxType() const { return _type; }
  OSS_INLINE sdbIndexID getIdxId() const { return _idxId; }
  OSS_INLINE const Ordering * getOrdering() const { return _pOrder->getOrdering(); }
  OSS_INLINE BSONObj getKeyPattern() const { return _keyPattern ; }
  OSS_INLINE BOOLEAN isValid() const
  {
     return ( ( SDB_INDEX_INVALID != _type ) && _pOrder && _idxId.isValid() ) ;
  }
protected:
  SDB_INDEX_TYPE     _type ;
  sdbIndexID         _idxId ;  // { UINT32 _csID, UINT32 _clID, UINT32 _idxLID }
  BSONObj            _keyPattern ;
  indexKeyOrdering * _pOrder ;
};


} // namespace vessel
} // namespace engine
#endif  //  LSMINDEXMETA_HPP_
