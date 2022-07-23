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

   Source File Name = keyStringCoder.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/keyStringCoder.h"

namespace engine
{
namespace vessel
{
   void keyStringCoder::encodeRid(const recordID &rid, void *buf)
   {
      CHAR *ptr = (CHAR *)buf;
      encodeUnsignedNative(rid.getPid(), FALSE, ptr);
      ptr += sizeof(UINT32);
      encodeSignedNative(rid.getPos(), FALSE, ptr);
      return;
   }

   recordID keyStringCoder::decodeToRid(const void *buf) const
   {
      UINT32 pid = decodeToUnsignedNative<UINT32>(buf, FALSE);
      INT16 pos = decodeToSignedNative<INT16>((const CHAR *)buf + sizeof(UINT32), FALSE);
      return recordID(pid, pos);
   }

   void keyStringCoder::encodeLSN(UINT64 lsn, void *buf)
   {
      encodeUnsignedNative(lsn, TRUE, buf);
   }

   UINT64 keyStringCoder::decodeToLSN(const void *buf)const
   {
      return decodeToUnsignedNative<UINT64>(buf, TRUE);
   }

   globalIndexID keyStringCoder::decodeToIndexId(const void *buf)const
   {
      UINT32 cs = decodeToUnsignedNative<UINT32>(buf, FALSE);
      UINT32 cl = decodeToUnsignedNative<UINT32>((const CHAR *)buf + sizeof(UINT32), FALSE);
      UINT32 index = decodeToUnsignedNative<UINT32>((const CHAR *)buf + (sizeof(UINT32) << 1), FALSE);
      return globalIndexID(cs, cl, index);
   }
} // namespace vessel

} // namespace engine
