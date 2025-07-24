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

   Source File Name = bytesReader.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
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
