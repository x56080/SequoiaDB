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
