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

   Source File Name = btreeKeyStringEntry.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_BTREE_KEY_STRING_ENTRY_H_
#define VESSEL_BTREE_KEY_STRING_ENTRY_H_

#include "vessel/keyString.h"

namespace engine
{
namespace vessel
{
   class btreeKeyStringEntry : public keyString
   {
      public:
         btreeKeyStringEntry() = default;
         ~btreeKeyStringEntry() = default;
         btreeKeyStringEntry(const slice &s);
         btreeKeyStringEntry(UINT32 size, const CHAR *data);
         btreeKeyStringEntry(const btreeKeyStringEntry &);
         btreeKeyStringEntry &operator=(const btreeKeyStringEntry &);
         btreeKeyStringEntry(btreeKeyStringEntry &&o)noexcept;
         btreeKeyStringEntry &operator=(btreeKeyStringEntry &&o)noexcept;

      public:
         INT32 init(const slice &s);
         INT32 moveFrom(keyString &&);
         INT32 shallowCopy(const keyString &ks);
         recordID getRid() const;
   };//class btreeKeyStringEntry
} // namespace vessel

} // namespace engine


#endif//VESSEL_BTREE_KEY_STRING_ENTRY_H_