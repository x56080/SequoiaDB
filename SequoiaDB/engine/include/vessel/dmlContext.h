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

#include "vessel/recordData.h"
#include "vessel/strSlice.h"
#include "utilUniqueID.hpp"
#include "utilCompression.hpp"
#include "dpsDef.hpp"
#include "vessel/recordID.h"
#include "ossMemPool.hpp"
#include "ossLikely.hpp"
#include "vessel/vesselDef.h"
#include "vessel/requestContext.h"

namespace engine
{
namespace vessel
{
   class collectionSpace;
   class collection;

   class dmlContext : public requestContext
   {
      public:
         OSS_INLINE dmlContext():
         _clLogicalID(DMS_INVALID_LOGICCLID),
         _uniqueID(utilBuildCLUniqueID(UTIL_INVALID_CS_UNIQUE_ID,
                                       UTIL_INVALID_CL_INNER_ID)),
         _compressionType(UTIL_COMPRESSOR_INVALID),
         _striping(INVALID_STRIPING_ID),
         _compressedRecordSize(0),
         _compressionBuffer(NULL),
         _compressionBufferSize(0),
         _lsn(DPS_INVALID_LSN_OFFSET)
         {
            
         }

         virtual ~dmlContext();
      public:
         virtual void close();

         INT32 initCL(collectionSpace *cs,
                      collection *cl);

         OSS_INLINE const strSlice &getCSName()const
         {
            return _csName;
         }
         OSS_INLINE const strSlice &getCLName()const
         {
            return _clName;
         }
         OSS_INLINE UINT32 getLogicalID()const
         {
            return _clLogicalID;
         }
         OSS_INLINE const utilCLUniqueID &getUniqueID()const
         {
            return _uniqueID;
         }
         OSS_INLINE BOOLEAN clInfoIsValid()const
         {
            return !_csName.empty() &&
                   !_clName.empty() &&
                   DMS_INVALID_LOGICCLID != _clLogicalID;
         }
         OSS_INLINE UTIL_COMPRESSOR_TYPE getCompressionType()const
         {
            return _compressionType;
         }
         OSS_INLINE void setTransID(const DPS_TRANS_ID &transID)
         {
            _transID = transID;
         }
         OSS_INLINE const DPS_TRANS_ID &getTransID()const
         {
            return _transID;
         }
         OSS_INLINE void setRecordData(const recordData &r)
         {
            _record = r;
         }
         OSS_INLINE const recordData &getRecord()const
         {
            return _record;
         }
         OSS_INLINE void setStriping(STRIPING_ID s)
         {
            _striping = s;
         }
         OSS_INLINE STRIPING_ID getStriping()const
         {
            return _striping;
         }

         OSS_INLINE void setCompressedRecordSize(UINT32 size)
         {
            _compressedRecordSize = size;
         }
         OSS_INLINE UINT32 getCompressedRecordSize()const
         {
            return _compressedRecordSize;
         }

         INT32 allocateCompressionBuffer(UINT32 size);

         OSS_INLINE UINT32 getCompressionBufferSize()const
         {
            return _compressionBufferSize;
         }

         OSS_INLINE CHAR *getCompressionBuffer()
         {
            return _compressionBuffer;
         }
         OSS_INLINE const CHAR *getCompressionBuffer()const
         {
            return _compressionBuffer;
         }

         OSS_INLINE void setLsn(const DPS_LSN_OFFSET &lsn)
         {
            _lsn = lsn;
            return;
         }
         OSS_INLINE const DPS_LSN_OFFSET &getLsn()const
         {
            return _lsn;
         }
         OSS_INLINE void setRid(const recordID &rid)
         {
            _rid = rid;
            return;
         }
         OSS_INLINE const recordID &getRid()const
         {
            return _rid;
         }

         OSS_INLINE void pushUniqueIdxHash(UINT16 h)
         {
            _uniqueIndexHash.push_back(h);
         }

         OSS_INLINE void sortIndexHash()
         {
            if (1 < _uniqueIndexHash.size())
            {
               std::sort(_uniqueIndexHash.begin(), _uniqueIndexHash.end());
            }
         }
         OSS_INLINE UINT32 getUniqueIndexCount()const
         {
            return _uniqueIndexHash.size();
         }
         OSS_INLINE const UINT16 *getUniqueIdexData()const
         {
            return _uniqueIndexHash.data();
         }
      private:
         void reset();

      private:
         strSlice _csName;
         strSlice _clName;
         UINT32 _clLogicalID;
         utilCLUniqueID _uniqueID;
         UTIL_COMPRESSOR_TYPE _compressionType;
         DPS_TRANS_ID _transID;
         STRIPING_ID _striping;
         recordData _record;
         UINT32 _compressedRecordSize;
         CHAR *_compressionBuffer;
         UINT32 _compressionBufferSize;
         DPS_LSN_OFFSET _lsn;
         recordID _rid;
         ossPoolVector<UINT16> _uniqueIndexHash;
   };//class dmlContext
}//namespace vessel
}//namespace engine

#endif//VESSEL_INSERT_CONTEXT_H_
