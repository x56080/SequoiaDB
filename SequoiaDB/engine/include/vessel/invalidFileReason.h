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

   Source File Name = invalidFileReason.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef VESSEL_INVALID_FILE_REASON_H_
#define VESSEL_INVALID_FILE_REASON_H_

#include "core.hpp"
#include "oss.hpp"

namespace engine
{
namespace vessel
{
   enum class invalidFileReason : INT32
   {
      NONE = 0,
      INVALID_HEADER_SIZE, /// file size lower than header size
      INVLAID_HEADER_MAGIC_CHARS,
      INVALID_HEADER_CHECKSUM,
      UNEXPECTED_HEADER_VERSION,
      UNEXPECTED_HEADER_CONTENT,
      UNEXPECTED_FILE_SIZE,
   };//enum class invalidFileReason

} // namespace vessel

} // namespace engine


#endif//VESSEL_INVALID_FILE_REASON_H_
