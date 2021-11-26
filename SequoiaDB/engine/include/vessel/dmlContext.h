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

         OSS_INLINE UINT32 getUniqueKeyHashSize()const
         {
            return _uniqueKeyHash.size();
         }
         OSS_INLINE const UINT32 *getUniqueKeyHashes()const
         {
            return _uniqueKeyHash.empty() ? NULL : _uniqueKeyHash.data();
         }

         /// adding can be executed multiple times before locking.
         void addKeysToBeConstraintCheck(const dmlIndexRequestArray &arr);

         INT32 lockUniqueIndexKeys();

         void unlockUniqueKeys();

         OSS_INLINE const recordID &getRid()const
         {
            return _rid;
         }
         OSS_INLINE void setRid(const recordID &rid)
         {
            _rid = rid;
         }
         OSS_INLINE BOOLEAN isDmlPositionSet()const
         {
            return _rid.valid() && INVALID_CL_PAGE_SEQ != _seq;
         }
         OSS_INLINE void setPageSeq(UINT32 s)
         {
            _seq = s;
         }
         OSS_INLINE scanEntry getScanEntry()const
         {
            return scanEntry(_seq, _rid.getSlotID());
         }

   
         void clearDmlHistroy();
         
         void setDmlLSN(const DPS_LSN_OFFSET &lsn)
         {
            _dmlLSN = lsn;
         }
         const DPS_LSN_OFFSET &getDmlLSN()const
         {
            return _dmlLSN;
         }

      private:

         INT32 _lockUniqueIndexKeys();

         void loopTryLock();

      private:
         typedef ossPoolVector<UNIQUE_INDEX_LATCH_MAP::object> _UNIQUE_KEY_CONTEXT;
      private:
         ossPoolVector<UINT32> _uniqueKeyHash;
         _UNIQUE_KEY_CONTEXT _uniqueKeyContext;
         UINT32 _seq = INVALID_CL_PAGE_SEQ;
         DPS_LSN_OFFSET _dmlLSN = DPS_INVALID_LSN_OFFSET;
         recordID _rid;
   };//class dmlContext
}//namespace vessel
}//namespace engine

#endif//VESSEL_INSERT_CONTEXT_H_
