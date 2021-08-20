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

   Source File Name = dmlContext.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_DML_CONTEXT_H_
#define VESSEL_DML_CONTEXT_H_

#include "vessel/strSlice.h"
#include "utilUniqueID.hpp"
#include "utilCompression.hpp"
#include "dpsDef.hpp"
#include "vessel/recordID.h"
#include "ossMemPool.hpp"
#include "ossLikely.hpp"
#include "vessel/vesselIdDef.h"
#include "vessel/requestContext.h"
#include "dms.hpp"
#include "vessel/dmlIndexRequest.h"
#include "vessel/scanEntry.h"

namespace engine
{
namespace vessel
{
   class collectionSpace;
   class collection;

   class dmlContext : public requestContext
   {
      public:
         dmlContext(){}
         virtual ~dmlContext();

      public:
         virtual void close();

         OSS_INLINE void setTransID(const DPS_TRANS_ID &transID)
         {
            _transID = transID;
         }
         OSS_INLINE const DPS_TRANS_ID &getTransID()const
         {
            return _transID;
         }
         OSS_INLINE UINT32 getUniqueKeyHashSize()const
         {
            return _uniqueKeyHash.size();
         }
         OSS_INLINE const UINT32 *getUniqueKeyHashes()const
         {
            return _uniqueKeyHash.empty() ? NULL : _uniqueKeyHash.data();
         }

         INT32 lockUniqueIndexKeys(const dmlIndexRequestArray &requests);

         void unlockUniqueKeys();

         OSS_INLINE void setMinFreeSize(UINT32 size)
         {
            _minFreeSize = size;
         }
         OSS_INLINE UINT32 getMinFreeSize()const
         {
            return _minFreeSize;
         }
         OSS_INLINE void setCompressionType(UTIL_COMPRESSOR_TYPE type)
         {
            _compressionType = type;
         }
         OSS_INLINE UTIL_COMPRESSOR_TYPE getCompressionType()const
         {
            return _compressionType;
         }

         OSS_INLINE const recordID &getDmlRid()const
         {
            return _rid;
         }
         OSS_INLINE void setDmlRid(const recordID &rid)
         {
            _rid = rid;
         }
         OSS_INLINE BOOLEAN isDmlPositionSet()const
         {
            return _rid.valid() && INVALID_CL_PAGE_SEQ != _pageSequence;
         }
         OSS_INLINE void setDmlLSN(DPS_LSN_OFFSET lsn)
         {
            _lsn = lsn;
         }
         OSS_INLINE DPS_LSN_OFFSET getDmlLSN()const
         {
            return _lsn;
         }
         OSS_INLINE void setPageSequence(UINT32 s)
         {
            _pageSequence = s;
         }
         OSS_INLINE scanEntry getScanEntry()const
         {
            return scanEntry(_pageSequence, _rid.getSlotID());
         }
         OSS_INLINE void setHasIndexReq(BOOLEAN hasIndexReq)
         {
            _hasIndexReq = hasIndexReq;
         }
         OSS_INLINE BOOLEAN hasIndexReq()const
         {
            return _hasIndexReq;
         }

         INT32 tryToLockRid(const recordID &rid,
                            OSS_SHARED_LATCH_MODE mode,
                            BOOLEAN &locked);

         /// WARNING: Always try lock rid under page latch!
         INT32 lockRid(const recordID &rid,
                       OSS_SHARED_LATCH_MODE mode);

         void unlockRid(const recordID &rid);

         void unlockRids();

         void unlockRidsAndUniqueKeys();

      private:
         void fini();

         void buildUniqueKeyHash(const dmlIndexRequestArray &requests,
                                 ossPoolVector<UINT32> &hashArray)const;

         INT32 _lockUniqueIndexKeys();

         void loopTryLock();

      private:
         typedef ossPoolVector<UNIQUE_INDEX_LATCH_MAP::object> _UNIQUE_KEY_CONTEXT;

      private:
         UINT32 _minFreeSize = 0;
         UTIL_COMPRESSOR_TYPE _compressionType = UTIL_COMPRESSOR_INVALID;
         DPS_TRANS_ID _transID;
         BOOLEAN _hasIndexReq = FALSE;

         ossPoolVector<UINT32> _uniqueKeyHash;
         _UNIQUE_KEY_CONTEXT _uniqueKeyContext;
         objectSharedLatchContext<recordIdLatchKey> _ridLatchContext;

         DPS_LSN_OFFSET _lsn = DPS_INVALID_LSN_OFFSET;
         recordID _rid;
         UINT32 _pageSequence = INVALID_CL_PAGE_SEQ;
   };//class dmlContext
}//namespace vessel
}//namespace engine

#endif//VESSEL_INSERT_CONTEXT_H_
