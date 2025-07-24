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

   Source File Name = cursorKernal.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef VESSEL_CURSOR_KERNAL_H_
#define VESSEL_CURSOR_KERNAL_H_

#include "vessel/slice.h"
#include <initializer_list>
#include "vessel/cursorDef.h"
#include "sdbInterface.hpp"
#include "interface/IDataCursor.h"
#include "vessel/cursorOptions.h"
#include "vessel/elasticBlockRowBatch.hpp"

namespace engine
{
namespace vessel
{
   class vesselImpl;

   class cursorKernal : public IDataCursor
   {
      public:
         cursorKernal(){}
         virtual ~cursorKernal();

      public:
         virtual BOOLEAN isClosed()const override;
         virtual void close() override;
         virtual INT32 fetchNext(IExecutor *executor) override;
         virtual slice getRawData()const override;

      public:
         virtual CURSOR_TYPE getType()const = 0;

      public:
         BOOLEAN isOpen()const;
         INT32 open(vesselImpl *db,
                    const cursorOptions *o=NULL);
         
         /// push completed record
         INT32 pushData(UINT32 len, const CHAR *data);

         /// push "ONE RECORD" with multi memory fragments
         INT32 pushDataFragments(std::initializer_list<slice> il);

         void setEOC();

         BOOLEAN noMorePushThisLoop()const;

      public:
         OSS_INLINE const cursorOptions &getBaseOptions()const
         {
            return _options;
         }
      
      private:
         void _close();
         
         BOOLEAN hitTheEnd()const;
         OSS_INLINE BOOLEAN hasMoreInBatch()const
         {
            return (_pos + 1) < (INT32)_batch.getRowCount();
         }
      
      private:
         cursorOptions _options;
         UINT32 _flags = 0;
         vesselImpl *_db = NULL;

         INT64 _fetched = 0;
         ELASTIC_BLOCK_ROW_BATCH _batch;
         INT32 _pos = -1;
   };//class cursorKernal
}//namespace vessel
}//namespace engine

#endif//VESSEL_CURSOR_KERNAL_H_