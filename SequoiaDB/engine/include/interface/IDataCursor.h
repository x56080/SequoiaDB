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

   Source File Name = IDataCursor.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef SDB_I_DATA_CURSOR_H_
#define SDB_I_DATA_CURSOR_H_

#include "sdbInterface.hpp"
#include "utilPooledObject.hpp"
#include "vessel/slice.h"
#include "dms.hpp"
#include "../bson/bson.hpp"
#include "pdTrace.hpp"

#include <memory> // c++ 11

namespace engine
{
   class IDataCursor : public _utilPooledObject
   {
      public:
         IDataCursor(){}
         virtual ~IDataCursor(){}
         IDataCursor(const IDataCursor &) = delete;
         IDataCursor &operator=(const IDataCursor &) = delete;

      public:
         virtual const CHAR *getName()const = 0;
         
      public:
         virtual BOOLEAN isClosed()const = 0;
         virtual void close() = 0;
         virtual INT32 fetchNext(IExecutor *executor) = 0;

      public:/// fetchNext first
         virtual vessel::slice getRawData()const = 0;
         virtual dmsRecordID getRid()const {return dmsRecordID();}
         virtual DPS_TRANS_ID getTransId()const {return DPS_TRANS_ID();}
         virtual vessel::slice getDataSlice()const = 0; /// record data slice

      public:/// fetchNext first
         bson::BSONObj getBsonRecord(BOOLEAN check=TRUE)const
         {
            bson::BSONObj obj;
            vessel::slice s = getDataSlice();
            if (s.getSize() <= sizeof(UINT32))
            {
               PD_LOG(PDERROR, "invalid slice size:%d", s.getSize());
            }
            else
            {
               try
               {
                  obj = bson::BSONObj(s.data(), check);
               }
               catch(const std::exception& e)
               {
                  PD_LOG(PDERROR, "failed to parse record data:%s", e.what());
               }
            }

            return obj;
         }

         template<class T>
         const T *getDataObjPtr()const
         {
            const T *ptr = nullptr;
            vessel::slice s = getDataSlice();
            if (sizeof(T) <= s.getSize())
            {
               ptr = (const T *)(s.data());
            }
            return ptr;
         }

   };//class IDataCursor

   typedef std::shared_ptr<IDataCursor> DATA_CURSOR_PTR;

} // namespace engie


#endif//SDB_I_DATA_CURSOR_H_