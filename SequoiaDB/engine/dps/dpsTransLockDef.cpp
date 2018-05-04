/*******************************************************************************

   Copyright (C) 2011-2018 SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   Source File Name = dpsTransLockDef.cpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of runtime component. This file contains code logic for
   common functions for coordinator node.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================

   Last Changed =

*******************************************************************************/

#include "dpsTransLockDef.hpp"
#include "dms.hpp"
#include "dmsStorageUnit.hpp"

using namespace bson ;
namespace engine
{
   dpsTransLockId::dpsTransLockId( UINT32 logicCSID,
                                   UINT16 collectionID,
                                   const _dmsRecordID *recordID )
   {
      _logicCSID = logicCSID ;
      _collectionID = collectionID ;
      if ( recordID )
      {
         _recordExtentID = recordID->_extent ;
         _recordOffset = recordID->_offset ;
      }
      else
      {
         _recordExtentID = DMS_INVALID_EXTENT ;
         _recordOffset = DMS_INVALID_OFFSET ;
      }
   }

   dpsTransLockId::dpsTransLockId()
   {
      reset() ;
   }

   dpsTransLockId::~dpsTransLockId()
   {
   }

   BOOLEAN dpsTransLockId::operator<( const dpsTransLockId &rhs ) const
   {
      if ( _logicCSID < rhs._logicCSID )
      {
         return TRUE;
      }
      else if ( _logicCSID > rhs._logicCSID )
      {
         return FALSE;
      }
      if ( _collectionID < rhs._collectionID )
      {
         return TRUE;
      }
      else if ( _collectionID > rhs._collectionID )
      {
         return FALSE;
      }
      if ( _recordExtentID < rhs._recordExtentID )
      {
         return TRUE;
      }
      else if ( _recordExtentID > rhs._recordExtentID )
      {
         return FALSE;
      }
      if ( _recordOffset < rhs._recordOffset )
      {
         return TRUE;
      }
      return FALSE;
   }

   BOOLEAN dpsTransLockId::operator==( const dpsTransLockId &rhs ) const
   {
      if ( _logicCSID == rhs._logicCSID &&
           _collectionID == rhs._collectionID &&
           _recordExtentID == rhs._recordExtentID &&
           _recordOffset == rhs._recordOffset )
      {
         return TRUE ;
      }
      return FALSE ;
   }

   void dpsTransLockId::reset()
   {
      _logicCSID = ~0 ;
      _collectionID = DMS_INVALID_MBID ;
      _recordExtentID = DMS_INVALID_EXTENT ;
      _recordOffset = DMS_INVALID_OFFSET ;
   }

   BOOLEAN dpsTransLockId::isValid() const
   {
      if ( (UINT32)~0 == _logicCSID )
      {
         return FALSE ;
      }
      return TRUE ;
   }

   dpsTransLockId & dpsTransLockId::operator=( const dpsTransLockId & rhs )
   {
      _logicCSID        = rhs._logicCSID ;
      _collectionID     = rhs._collectionID ;
      _recordExtentID   = rhs._recordExtentID ;
      _recordOffset     = rhs._recordOffset ;

      return *this ;
   }

   std::string dpsTransLockId::toString() const
   {
      CHAR szBuffer[100] = {0};
      ossSnprintf( szBuffer, sizeof(szBuffer)-1,
                  "CSID:%u, CLID:%u, recordID:%d, recordOffset:%d",
                  _logicCSID, _collectionID, _recordExtentID, _recordOffset );
      std::string strInfo( szBuffer );
      return strInfo;
   }

   BSONObj dpsTransLockId::toBson() const
   {
      return BSON( "CSID" << (INT32)_logicCSID <<
                   "CLID" << (INT32)_collectionID <<
                   "recordID" << (INT32)_recordExtentID <<
                   "recordOffset" << (INT32)_recordOffset ) ;
   }

   dpsTransCBLockInfo::dpsTransCBLockInfo( DPS_TRANSLOCK_TYPE lockType )
   : _lockType( lockType )
   {
      _pNextWaitCB = NULL ;
      _pRef = SDB_OSS_NEW ossAtomicSigned64(0);
   }
   dpsTransCBLockInfo::~dpsTransCBLockInfo()
   {
      if ( _pRef )
      {
         SDB_OSS_DEL _pRef ;
      }
   }
   INT64 dpsTransCBLockInfo::incRef()
   {
      return _pRef->inc();
   }

   INT64 dpsTransCBLockInfo::decRef()
   {
      return _pRef->dec();
   }

   BOOLEAN dpsTransCBLockInfo::isLockMatch( DPS_TRANSLOCK_TYPE type )
   {
      if ( _lockType == type )
      {
         return TRUE ;
      }
      if ( DPS_TRANSLOCK_IS == type )
      {
         return TRUE ;
      }
      if ( DPS_TRANSLOCK_X == _lockType )
      {
         return TRUE ;
      }
      return FALSE ;
   }

   DPS_TRANSLOCK_TYPE dpsTransCBLockInfo::getType()
   {
      return _lockType ;
   }

   void dpsTransCBLockInfo::setType( DPS_TRANSLOCK_TYPE lockType )
   {
      _lockType = lockType ;
   }

   _pmdEDUCB *dpsTransCBLockInfo::getNextWaitCB()
   {
      return _pNextWaitCB ;
   }
   void dpsTransCBLockInfo::setNextWaitCB( _pmdEDUCB *pWaitCB )
   {
      _pNextWaitCB = pWaitCB ;
   }
}
