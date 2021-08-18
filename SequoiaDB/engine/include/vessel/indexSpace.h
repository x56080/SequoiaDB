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

   Source File Name = indexSpace.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_INDEX_SPACE_H_
#define VESSEL_INDEX_SPACE_H_

#include "vessel/copyOnWriteLPS.h"
#include "vessel/dataStorageFileCluster.h"

namespace engine
{
namespace vessel
{  
   class indexSpace : public copyOnWriteLPS
   {
      public:
         indexSpace(){}
         virtual ~indexSpace(){}

      public:
         virtual SPACE_TYPE getSpaceType()const
         {
            return SPACE_TYPE_IDX;
         }

      private:
         virtual UINT32 getReservedImpCount()const;
         virtual UINT32 getFreeBoundOfLpidAllocator()const 
         {
            return 0;
         }
         virtual UINT32 getFreeBoundOfPageStorage()const
         {
            return 0;
         }
         virtual dataPageCluster *getDataStorageObj()
         {
            return &_storage;
         }

      public:
         ///lpid may be invalid 
         INT32 getIndexDefPage(requestContext *context,
                               CL_MB_ID mbID,
                               INT32 slot,
                               PAGE_ID &lpid);
         PAGE_ID getDirectMappedIndexLpid(CL_MB_ID mbID, INT32 slot)const;
         PAGE_ID getMappingPageLpid(CL_MB_ID mbID,
                                    INT32 slot,
                                    UINT32 &pos)const;

      private:
         dataStorageFileCluster _storage;
   };//class indexSpace
}//namespace vessel
}//namespace engine

#endif//VESSEL_INDEX_SPACE_H_