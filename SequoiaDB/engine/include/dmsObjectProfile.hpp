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

   Source File Name = dmsObjectProfile.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef SDB_DMS_OBJECT_PROFILE_HPP_
#define SDB_DMS_OBJECT_PROFILE_HPP_

#include "oss.hpp"
#include "core.hpp"

namespace engine
{
   class _dmsMBProfile : public SDBObject
   {
      public:
         _dmsMBProfile(){}
         ~_dmsMBProfile(){}
         _dmsMBProfile(const _dmsMBProfile &o):
         data(o.data),
         index(o.index)
         {}

         _dmsMBProfile &operator=(const _dmsMBProfile &o)
         {
            data = o.data;
            index = o.index;
            return *this;
         }

      public:
         struct dataSummary
         {
            dataSummary(){}
            dataSummary(const dataSummary &o):
            totalRecordCount(o.totalRecordCount),
            totalDataPages(o.totalDataPages),
            originalRecordSize(o.originalRecordSize)
            {}
            dataSummary &operator=(const dataSummary &o)
            {
               totalRecordCount = o.totalRecordCount;
               totalDataPages = o.totalDataPages;
               originalRecordSize = o.originalRecordSize;
               return *this;
            }

            UINT64 totalRecordCount = 0;
            UINT32 totalDataPages = 0;
            UINT64 originalRecordSize = 0;
         };

         struct indexSummary
         {
            indexSummary(){}
            indexSummary(const indexSummary &o):
            btreeIndexCount(o.btreeIndexCount),
            lsmIndexCount(o.lsmIndexCount),
            totalIndexPages(o.totalIndexPages),
            totalIndexFreeSpace(o.totalIndexFreeSpace)
            {}
            indexSummary &operator=(const indexSummary &o)
            {
               btreeIndexCount = o.btreeIndexCount;
               lsmIndexCount = o.lsmIndexCount;
               totalIndexPages = o.totalIndexPages;
               totalIndexFreeSpace = o.totalIndexFreeSpace;
               return *this;
            }

            UINT32 btreeIndexCount =0 ;
            UINT32 lsmIndexCount = 0;
            UINT32 totalIndexPages = 0;
            UINT64 totalIndexFreeSpace = 0;
         };

      public:
         dataSummary data;
         indexSummary index;
         
   };//class _dmsMBProfile
   typedef class _dmsMBProfile dmsMBProfile;
} // namespace engine


#endif//SDB_DMS_OBJECT_PROFILE_HPP_
