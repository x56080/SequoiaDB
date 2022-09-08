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

   Source File Name = bytesReader.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/bytesReader.h"

namespace engine
{
namespace vessel
{
   bytesReader::bytesReader(const slice &bytes, BOOLEAN reverse):
   _reverse(reverse),
   _bytes(bytes),
   _offset(0)
   {
      if (reverse)
      {
         _offset = static_cast<INT64>(_bytes.getSize()) - 1;
      }
   }

   void bytesReader::init(const slice &bytes, BOOLEAN reverse)
   {
      _reverse = reverse;
      _bytes = bytes;
      _offset = !reverse ? 0 : static_cast<INT64>(_bytes.getSize()) - 1;
   }

   void bytesReader::reset()
   {
      _reverse = FALSE;
      _bytes.reset();
      _offset = 0;
   }

   BOOLEAN bytesReader::slide(UINT32 size)
   {
      if (_reverse)
      {
         _offset -= size;
      }
      else
      {
         _offset += size;
      }

      return !_isOutOfBound(1);
   }
} // namespace vessel

} // namespace engine
