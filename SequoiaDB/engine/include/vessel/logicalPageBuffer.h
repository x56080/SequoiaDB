/*******************************************************************************

   Copyright (C) 2011-Present SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   Source File Name = logicalPageBuffer.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef VESSEL_LOGICAL_PAGE_BUFFER_H_
#define VESSEL_LOGICAL_PAGE_BUFFER_H_

#include "vessel/runtimePageBuffer.h"
#include "utilPooledObject.hpp"
#include "vessel/lpageDescriptor.h"

#include <atomic>

namespace engine
{
namespace vessel
{
   class logicalPageSpace;
   class requestContext;

   class logicalPageBuffer : public _utilPooledObject
   {
      friend class logicalPageSpace;
      public:
         logicalPageBuffer(){}
         virtual ~logicalPageBuffer();
         logicalPageBuffer(const logicalPageBuffer &) = delete;
         logicalPageBuffer &operator=(const logicalPageBuffer &) = delete;
         logicalPageBuffer(logicalPageBuffer &&);
         logicalPageBuffer &operator=(logicalPageBuffer &&);

      public:
         OSS_INLINE PAGE_ID getLogicalPid()const
         {
            return _lpid;
         }
         OSS_INLINE BOOLEAN isValid()const
         {
            return INVALID_PAGE_ID != _lpid &&
                   NULL != _context &&
                   NULL != _lps &&
                   _rpb.isValid();
         }
         OSS_INLINE PAGE_SNAPSHOT_VERION getPsv()const
         {
            return _psv;
         }
         OSS_INLINE const ossSharedLatchMode &getLockingMode()const
         {
            return _mode;
         }
         OSS_INLINE logicalPageSpace *getLogicalPageSpace()
         {
            return _lps;
         }
         OSS_INLINE requestContext* getContext()
         {
            return _context;
         }

         OSS_INLINE UINT32 getPageSize()const
         {
            return _rpb.getPageSize();
         }
         OSS_INLINE const GLOBAL_PAGE_ID &getGlobalPid()const
         {
            return _rpb.getGlobalPid();
         }

         OSS_INLINE const runtimePageBuffer &getRuntimeBuffer()const
         {
            return _rpb;
         }

      public:
         virtual void fini();
         virtual INT32 prepareToWrite();
         void commit(DPS_LSN_OFFSET lsn);
         
         INT32 autoGetWritableBodyBuffer(strictBuffer &buffer);

         BOOLEAN isWritable()const;

         INT32 validatePage(PAGE_TYPE type)const;

         strictBuffer getReadableBodyBuffer()const;
         strictBuffer getWritableBodyBuffer();

         /// release lpid and pid
//         void destroy();

      public:
         /// must hold shared lock first
         BOOLEAN tryLockExclusiveFromShared();

         /// must hold upgrade lock first
         BOOLEAN tryLockExclusiveFromUpgrade();

         /// must hold upgrade lock first
         void lockExclusiveFromUpgrade();

      protected:
         void _fini();

      protected:
         PAGE_ID _lpid = INVALID_PAGE_ID;
         ossSharedLatchMode _mode;
         requestContext *_context = NULL;
         logicalPageSpace *_lps = NULL;
         runtimePageBuffer _rpb;
         PAGE_SNAPSHOT_VERION _psv = INVALID_PAGE_SNAPSHOT_VERSION;
   };//class logicalPageBuffer

   using LPAGE_BUFFER_UPTR = std::unique_ptr<logicalPageBuffer>;
   using LPAGE_BUFFER_SPTR = std::shared_ptr<logicalPageBuffer>;
}//namespace vessel
}//namespace engine

#endif//VESSEL_LOGICAL_PAGE_BUFFER_H_