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

   Source File Name = lsmIdxID.hpp

   Descriptive Name = LSM IndexID  Header

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          02/25/2021  JT  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef LSMIDXID_HPP_
#define LSMIDXID_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "dms.hpp"
namespace engine
{
namespace vessel
{
#pragma pack(4)

// unique index ID
class sdbIndexID : public SDBObject
{
public:
   UINT32  _csID;   // collectionspace unique id
   UINT32  _clID;   // collection inner id
   UINT32  _idxLID; // index logic id

   sdbIndexID()
   {
      _csID   = DMS_INVALID_SUID ;
      _clID   = DMS_INVALID_CLID ;
      _idxLID = ((UINT32)(-1)) ;
   }

   sdbIndexID( UINT32 csID, UINT16 clID, SINT32 idxLID )
   {
      _csID   = csID ;
      _clID   = clID ;
      _idxLID = idxLID ;
   }

   sdbIndexID& operator= ( const sdbIndexID &rhs )
   {
      _csID   = rhs._csID ;
      _clID   = rhs._clID ;
      _idxLID = rhs._idxLID ;
      return *this ;
   }

   BOOLEAN operator< ( const sdbIndexID &rhs ) const
   {
      BOOLEAN rv = FALSE ;
      if ( _csID < rhs._csID )
      {
         rv = TRUE ;
      }
      else if ( _csID == rhs._csID )
      {
         if ( _clID < rhs._clID )
         {
            rv = TRUE ;
         }
         else if ( _clID == rhs._clID )
         {
            if ( _idxLID < rhs._idxLID )
            {
               rv = TRUE ;
            }
         }
      }
      return rv ;
   }

   BOOLEAN operator== ( const sdbIndexID &rhs ) const
   {
      if ( ( _csID == rhs._csID ) &&
           ( _clID == rhs._clID ) &&
           ( _idxLID == rhs._idxLID ) )
      {
         return TRUE ;
      }
      return FALSE ;
   }

   OSS_INLINE BOOLEAN isValid() const
   {
      return ( ( ((UINT32)(-1)) != _idxLID ) &&
               ( ((UINT32)DMS_INVALID_CLID) != _clID ) &&
               ( ((UINT32)DMS_INVALID_SUID) != _csID ) ) ;
   }
} ;

const UINT32 lsmIdxIDSz        = sizeof( sdbIndexID ) ;

#pragma pack()

} //namespace vessel
} // namespace engine
#endif  // LSMIDXID_HPP_
