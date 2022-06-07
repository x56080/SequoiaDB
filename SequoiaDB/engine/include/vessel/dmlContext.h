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
#include "vessel/modifyRecordContext.h"
#include "dmsStripingId.hpp"

namespace engine
{
namespace vessel
{
   class dmlContext : public requestContext
   {
      public:
         dmlContext(){}
         virtual ~dmlContext();

      public:
         OSS_INLINE UINT32 getUniqueKeyHashSize()const
         {
            return _uniqueKeyHash.size();
         }
         OSS_INLINE const UINT32 *getUniqueKeyHashes()const
         {
            return _uniqueKeyHash.empty() ? NULL : _uniqueKeyHash.data();
         }
         OSS_INLINE const recordID &getRid()const
         {
            return _rid;
         }
         OSS_INLINE BOOLEAN isDmlPositionSet()const
         {
            return _rid.isValid();
         }
         OSS_INLINE scanEntry getScanEntry()const
         {
            return scanEntry(_seq, _rid.getPos());
         }
         
         OSS_INLINE const DPS_LSN_OFFSET &getDmlLSN()const
         {
            return _lsn;
         }
         void setDmlRecordInfo(UINT32 seq,
                               const recordID &rid);

         void setDmlLSN(const DPS_LSN_OFFSET &lsn);

         OSS_INLINE void setIndexReqCount(UINT32 n)
         {
            _indexReqCount = n;
         }

         OSS_INLINE UINT32 getIndexReqCount()const
         {
            return _indexReqCount;
         }

         INT32 saveReocordDataToMrc(const slice &record);
         
         void setStripingId(const dmsStripingId &striping);

         const dmsStripingId &getStripingId()const{return _striping;}

         modifyRecordContext &getMrc() {return _mrc;}

         const modifyRecordContext &getMrc()const {return _mrc;}
      public:
         INT32 lockUniqueIndexKeys(const dmlIndexRequestArray &ra);
         void unlockUniqueKeys();
         void reset();

      private:
         virtual void _onClose()override;

         void _reset();

      private:
         INT32 _lockUniqueIndexKeys();

         void loopTryLock();

      private:
         typedef ossPoolVector<UNIQUE_INDEX_LATCH_MAP::object> _UNIQUE_KEY_CONTEXT;
      private:
         ossPoolVector<UINT32> _uniqueKeyHash;
         _UNIQUE_KEY_CONTEXT _uniqueKeyContext;

         UINT32 _indexReqCount = 0;
         UINT32 _seq = 0;
         DPS_LSN_OFFSET _lsn = DPS_INVALID_LSN_OFFSET;
         recordID _rid;
         modifyRecordContext _mrc;
         dmsStripingId _striping;
   };//class dmlContext
}//namespace vessel
}//namespace engine

#endif//VESSEL_INSERT_CONTEXT_H_
