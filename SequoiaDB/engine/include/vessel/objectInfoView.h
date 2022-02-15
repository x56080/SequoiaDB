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

   Source File Name = clInfoView.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_CL_INFO_VIEW_H_
#define VESSEL_CL_INFO_VIEW_H_

#include "vessel/objectIdentifier.h"
#include "vessel/collectionRecordPage.h"
#include "dmsStripingId.hpp"
#include "utilCompression.hpp"

namespace engine
{
namespace vessel
{
   class csInfoView : public SDBObject
   {
      public:
         csInfoView(){}
         ~csInfoView(){}

      public:
         collectionSpaceId csid;
         std::string name;

   };//class csInfoView
   typedef 

   class clInfoView : public SDBObject
   {
      public:
         clInfoView(){}
         ~clInfoView(){}

      public:
         collectionId clid;
         std::string name;
         COLLECTION_TYPE type = COLLECTION_TYPE_INVALID;
         UINT8 compressionType = UTIL_COMPRESSOR_INVALID;
         FLOAT32 minFreePercent = 0.0f;
         dmsStripingRange stripingRange;

   };//class clInfoView

   class indexInfoView : public SDBObject
   {
      public:
         indexInfoView(){}
         ~indexInfoView(){}

      public:
         indexIdentifier indexId;
         UINT32 flags = 0;
         std::string name;
   };//class indexInfoView
}
}

#endif//VESSEL_CL_INFO_VIEW_H_
