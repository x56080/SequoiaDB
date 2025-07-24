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

   Source File Name = globalIndexID.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef VESSEL_GLOBAL_INDEX_ID_H_
#define VESSEL_GLOBAL_INDEX_ID_H_

#include "dms.hpp"
#include "vessel/indexDef.h"
#include "vessel/objectIdentifier.h"

namespace engine
{
namespace vessel
{
#pragma pack(4)
   class globalIndexID : public SDBObject
   {
      public:
         globalIndexID(){}
         ~globalIndexID(){}
         explicit globalIndexID(UINT32 cs, UINT32 cl, UINT32 index):
         _csLid(cs),
         _clLid(cl),
         _indexLid(index){}

         explicit globalIndexID(const globalLogicalClId &clid, UINT32 index):
         _csLid(clid.getLogicalCSID()),
         _clLid(clid.getLogicalCLID()),
         _indexLid(index){}

         globalIndexID(const globalIndexID &o):
         _csLid(o._csLid),
         _clLid(o._clLid),
         _indexLid(o._indexLid){}

         globalIndexID &operator=(const globalIndexID &o)
         {
            _csLid = o._csLid;
            _clLid = o._clLid;
            _indexLid = o._indexLid;
            return *this;
         }

         BOOLEAN operator==(const globalIndexID &o)const
         {
            return _csLid == o._csLid &&
                   _clLid == o._clLid &&
                   _indexLid == o._indexLid;
         }

         BOOLEAN operator!=(const globalIndexID &o)const
         {
            return !(*this == o);
         }

         INT32 compare(const globalIndexID &o)const
         {
            if (_csLid < o._csLid)
            {
               return -1;
            }
            else if (_csLid > o._csLid)
            {
               return 1;
            }
            else if (_clLid < o._clLid)
            {
               return -1;
            }
            else if (_clLid > o._clLid)
            {
               return 1;
            }
            else if (_indexLid < o._indexLid)
            {
               return -1;
            }
            else if (_indexLid > o._indexLid)
            {
               return 1;
            }
            else
            {
               return 0;
            }
         }

         BOOLEAN operator<(const globalIndexID &o)const
         {
            return compare(o) < 0;
         }

      public:
         OSS_INLINE BOOLEAN isValid()const
         {
            return DMS_INVALID_LOGICCSID != _csLid &&
                   DMS_INVALID_LOGICCLID != _clLid &&
                   INVALID_LOGICAL_INDEX_ID != _indexLid;
         }
         OSS_INLINE UINT32 getLogicalCSID()const
         {
            return _csLid;
         }
         OSS_INLINE UINT32 getLogicalCLID()const
         {
            return _clLid;
         }
         OSS_INLINE UINT32 getLogicalIndexID()const
         {
            return _indexLid;
         }
         OSS_INLINE void reset(UINT32 cs = DMS_INVALID_LOGICCSID,
                               UINT32 cl = DMS_INVALID_LOGICCLID,
                               UINT32 index = INVALID_LOGICAL_INDEX_ID)
         {
            _csLid = cs;
            _clLid = cl;
            _indexLid = index;
            return;
         }

         ossPoolString toString() const
         {
            bson::StringBuilder builder(64);
            builder << '{' << _csLid << ','
                    << _clLid << "," << _indexLid << '}';
            return std::move(builder.poolStr());
         }

         static globalIndexID getMinGlobalIndexID()
         {
            return globalIndexID(0, 0, 0);
         }

         static globalIndexID getMaxGlobalIndexID()
         {
            return globalIndexID(DMS_INVALID_LOGICCSID - 1,
                                 DMS_INVALID_LOGICCLID - 1,
                                 INVALID_LOGICAL_INDEX_ID - 1);
         }

      private:
         UINT32 _csLid = DMS_INVALID_LOGICCSID;
         UINT32 _clLid = DMS_INVALID_LOGICCLID;
         UINT32 _indexLid = INVALID_LOGICAL_INDEX_ID;
   };//class globalIndexID 
#pragma pack()

   static const UINT32 GLOBAL_INDEX_ID_SIZE = sizeof(globalIndexID);
}//namespace vessel
}//namespace engine


#endif//VESSEL_GLOBAL_INDEX_ID_H_