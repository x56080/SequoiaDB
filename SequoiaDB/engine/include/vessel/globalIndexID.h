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

   Source File Name = globalIndexID.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_GLOBAL_INDEX_ID_H_
#define VESSEL_GLOBAL_INDEX_ID_H_

#include "dms.hpp"
#include "vessel/indexDef.h"

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

         BOOLEAN operator<(const globalIndexID &o)const
         {
            if (_csLid < o._csLid)
            {
               return TRUE;
            }
            else if (_csLid > o._csLid)
            {
               return FALSE;
            }
            else if (_clLid < o._clLid)
            {
               return TRUE;
            }
            else if (_clLid > o._clLid)
            {
               return FALSE;
            }
            else
            {
               return _indexLid < o._indexLid;
            }
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