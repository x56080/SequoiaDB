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