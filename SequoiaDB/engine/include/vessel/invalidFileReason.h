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

   Source File Name = invalidFileReason.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

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
