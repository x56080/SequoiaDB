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

   Source File Name = lsmKeyStringEntry.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef VESSEL_LSM_KEY_STRING_ENTRY_H_
#define VESSEL_LSM_KEY_STRING_ENTRY_H_

#include "vessel/keyString.h"
#include "vessel/globalIndexID.h"

namespace engine
{
namespace vessel
{
   class lsmKeyStringEntry : public keyString
   {
      public:
         lsmKeyStringEntry() = default;
         ~lsmKeyStringEntry() = default;
         lsmKeyStringEntry(const slice &s);
         lsmKeyStringEntry(UINT32 size, const CHAR *data);

      public:
         INT32 init(const slice &s);
         recordID getRid() const;
         globalIndexID getIndexId() const;
   };//class lsmKeyStringEntry
} // namespace vessel

} // namespace engine


#endif//VESSEL_LSM_KEY_STRING_ENTRY_H_