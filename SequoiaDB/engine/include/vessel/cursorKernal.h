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

   Source File Name = cursorKernal.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_CURSOR_KERNAL_H_
#define VESSEL_CURSOR_KERNAL_H_

#include "vessel/slice.h"
#include <initializer_list>
#include "vessel/cursorDef.h"
#include "sdbInterface.hpp"
#include "interface/IDataCursor.h"
#include "vessel/cursorOptions.h"
#include "vessel/elasticBlockRowBatch.h"

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
         elasticBlockRowBatch _batch;
         INT32 _pos = -1;
   };//class cursorKernal
}//namespace vessel
}//namespace engine

#endif//VESSEL_CURSOR_KERNAL_H_