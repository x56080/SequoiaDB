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

   Source File Name = logicalPageBuffer.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_LOGICAL_PAGE_BUFFER_H_
#define VESSEL_LOGICAL_PAGE_BUFFER_H_

#include "vessel/runtimePageBuffer.h"
#include "utilPooledObject.hpp"
#include "vessel/lpageDescriptor.h"

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
         ~logicalPageBuffer();
         logicalPageBuffer(const logicalPageBuffer &) = delete;
         logicalPageBuffer &operator=(const logicalPageBuffer &) = delete;

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
         void fini();
         INT32 prepareToWrite();
         void commit(DPS_LSN_OFFSET lsn);
         
         INT32 autoGetWritableBodyBuffer(strictBuffer &buffer);

         BOOLEAN isWritable()const;

         INT32 validatePage(PAGE_TYPE type)const;

         strictBuffer getReadableBodyBuffer()const;
         strictBuffer getWritableBodyBuffer();

         /// release lpid and pid
         void destroy();

      public:
         /// must hold shared lock first
         BOOLEAN tryLockExclusiveFromShared();

         /// must hold upgrade lock first
         BOOLEAN tryLockExclusiveFromUpgrade();

         /// must hold upgrade lock first
         void lockExclusiveFromUpgrade();
      
      private:
         PAGE_ID _lpid = INVALID_PAGE_ID;
         ossSharedLatchMode _mode;
         requestContext *_context = NULL;
         logicalPageSpace *_lps = NULL;
         runtimePageBuffer _rpb;
         PAGE_SNAPSHOT_VERION _psv = INVALID_PAGE_SNAPSHOT_VERSION;
   };//class logicalPageBuffer
}//namespace vessel
}//namespace engine

#endif//VESSEL_LOGICAL_PAGE_BUFFER_H_