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

   Source File Name = keyStringCoder.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
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

   void keyStringCoder::encodeGlobalIndexId(const globalIndexID &id, BOOLEAN asUpperKey, void *buf)
   {
      CHAR *data = (CHAR *)buf;
      encodeUnsignedNative<UINT32>(id.getLogicalCSID(), FALSE, data);
      encodeUnsignedNative<UINT32>(id.getLogicalCLID(), FALSE, data + sizeof(UINT32));
      encodeUnsignedNative<UINT32>(asUpperKey ? id.getLogicalIndexID() + 1 : id.getLogicalIndexID(),
                                   FALSE, data + (sizeof(UINT32) << 1));
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
