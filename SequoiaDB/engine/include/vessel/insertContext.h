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

   Source File Name = insertContext.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef SDB_VESSEL_INSERT_CONTEXT_H_
#define SDB_VESSEL_INSERT_CONTEXT_H_

#include "vessel/dmlContext.h"
#include "vessel/insertOptions.h"
#include "vessel/freeSpaceMapDef.h"
#include "vessel/slice.h"
#include "vessel/memoryBlock.h"

namespace engine
{
namespace vessel
{
   class insertContext : public dmlContext
   {
      public:
         OSS_INLINE insertContext()
         {}

         virtual ~insertContext(){}
      public:
         virtual void close();

         OSS_INLINE const insertOptions &getOptions()const
         {
            return _options;
         }
         OSS_INLINE void setOptions(const insertOptions &o)
         {
            _options = o;
         }
         OSS_INLINE fsmCandidate &getCandidate()
         {
            return _candidate;
         }
         OSS_INLINE void setLastFreeSize(UINT32 size)
         {
            _lastFreeSize = size;
         }
         OSS_INLINE UINT32 getLastFreeSize()const
         {
            return _lastFreeSize;
         }

         OSS_INLINE UTIL_COMPRESSOR_TYPE getCompressionType()const
         {
            return _compressionType;
         }
         OSS_INLINE BOOLEAN isCompressed()const
         {
            return UTIL_COMPRESSOR_INVALID != _compressionType;
         }

         OSS_INLINE void setOriginalRecord(const slice &r)
         {
            _originalRecord = r;
         }
         /// Return compressed record or original record.
         slice getRecordToInsert()const;

         OSS_INLINE void setStriping(STRIPING_ID s)
         {
            _striping = s;
         }
         OSS_INLINE STRIPING_ID getStriping()const
         {
            return _striping;
         }
         OSS_INLINE const recordID &getRid()const
         {
            return _rid;
         }
         OSS_INLINE void setRid(const recordID &rid)
         {
            _rid = rid;
         }

         OSS_INLINE const slice &getOriginalRecord()const
         {
            return _originalRecord;
         }
         OSS_INLINE memoryBlock &getCompressionMB()
         {
            return _compressionMB;
         }

      private:
         void fini();

      private:
         /// init from request
         insertOptions _options;
         STRIPING_ID _striping = INVALID_STRIPING_ID;
         slice _originalRecord;

         /// runtime
         UTIL_COMPRESSOR_TYPE _compressionType = UTIL_COMPRESSOR_INVALID;
         memoryBlock _compressionMB;

         fsmCandidate _candidate;

         recordID _rid;
         UINT32 _lastFreeSize = 0;
   };//class insertContext
}//namespace vessel
}//namespace engine

#endif//SDB_VESSEL_INSERT_CONTEXT_H_