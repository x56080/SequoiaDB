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

   Source File Name = collectionObject.h

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains functions for agent processing.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_COLLECTION_OBJECT_H_
#define VESSEL_COLLECTION_OBJECT_H_

#include "vessel/vesselDef.h"

namespace engine
{
namespace vessel
{
   class vessel;

   class collectionObject : public SDBObject
   {
      public:
         OSS_INLINE collectionObject():
         _db(NULL),
         _cslid(DMS_INVALID_LOGICCSID),
         _sid(INVALID_SPACE_ID),
         _cllid(DMS_INVALID_LOGICCLID),
         _mbid(INVALID_CL_MB_ID)
         {}

         OSS_INLINE ~collectionObject(){}

      public:
         OSS_INLINE BOOLEAN isOpen()const
         {
            return NULL != _db;
         }

         INT32 open(vessel *db,
                    UINT32 cslid,
                    UINT32 cllid,
                    SPACE_ID sid,
                    CL_MB_ID mbid);

         INT32 close();
      private:
         vessel *_db;
         UINT32 _cslid;
         SPACE_ID _sid;
         UINT32 _cllid;
         CL_MB_ID _mbid;
         
   };//class collectionObject
}//namespace vessel
}//namespace engine

#endif//VESSEL_COLLECTION_OBJECT_H_