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

   Source File Name = lsmLobcComparator.h

   Descriptive Name = 

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/20/2022  LYC  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef LSM_LOBC_KEY_COMPARATOR_H_
#define LSM_LOBC_KEY_COMPARATOR_H_

#include "ossTypes.h"
#include "rocksdb/comparator.h"
namespace engine
{
namespace vessel
{
   class lsmLobcKeyComparatorImpl : public rocksdb::Comparator
   {
      public:
         virtual INT32 Compare(const rocksdb::Slice &a, const rocksdb::Slice &b)const override;

         virtual const CHAR* Name()const override{return "sdb.lsmLobcKeyComparator";}

         virtual void FindShortestSeparator(std::string*, const rocksdb::Slice&)const override{}
         virtual void FindShortSuccessor(std::string*)const override{}
   };

   const rocksdb::Comparator *lsmLobcKeyComparator();
} // namespace vessel
} // namespace engine
#endif // LSM_LOBC_KEY_COMPARATOR_H_