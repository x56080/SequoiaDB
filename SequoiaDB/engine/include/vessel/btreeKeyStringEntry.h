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

   Source File Name = btreeKeyStringEntry.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
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