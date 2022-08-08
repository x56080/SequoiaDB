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

   Source File Name = lsmIndexMetaKey.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/20/2022  LYC  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/lsm/lsmIndexMetaKey.h"
#include "ossTypes.h"
#include "dpsDef.hpp"
#include "vessel/keyStringCoder.h"

namespace engine
{
namespace vessel
{
   ///////////////////////////// lsmIndexIdKey
   void lsmIndexMetaKey::init(const globalIndexID &id)
   {
      keyStringCoder coder;
      coder.encodeGlobalIndexId(id, FALSE, _data);
   }

   void lsmIndexMetaKey::initAsUpKey(const globalIndexID &id)
   {
      SDB_ASSERT(id.getLogicalIndexID() != OSS_UINT32_MAX, "out of bound");
      keyStringCoder coder;
      coder.encodeGlobalIndexId(id, TRUE, _data);
   }

   void lsmIndexMetaKey::reset()
   {
      ossMemset(_data, 0, sizeof(_data));
   }

   globalIndexID lsmIndexMetaKey::toGlobalIndexId()const
   {
      keyStringCoder coder;
      return coder.decodeToIndexId(_data);
   }

   //////////////////////////////// lsmIndexManifestKey
   void lsmIndexManifestKey::init(UINT32 csLid)
   {
      keyStringCoder coder;
      coder.encodeUnsignedNative<UINT32>(csLid, FALSE, _data);
      return;
   }

   void lsmIndexManifestKey::initAsUpKey(UINT32 csLid)
   {
      SDB_ASSERT(csLid != OSS_UINT32_MAX, "out of bound");
      keyStringCoder coder;
      coder.encodeUnsignedNative<UINT32>(csLid + 1, FALSE, _data);
   }

   void lsmIndexManifestKey::reset()
   {
      ossMemset(_data, 0, sizeof(_data));
   }

   //////////////////////////////// lsmCLIdKey
   void lsmCLIdKey::init(UINT32 csLid, UINT32 clLid)
   {
      keyStringCoder coder;
      coder.encodeUnsignedNative<UINT32>(csLid, FALSE, _data);
      coder.encodeUnsignedNative<UINT32>(clLid, FALSE, _data + sizeof(UINT32));
   }

   void lsmCLIdKey::initAsUpKey(UINT32 csLid, UINT32 clLid)
   {
      SDB_ASSERT(clLid != OSS_UINT32_MAX, "out of bound");
      keyStringCoder coder;
      coder.encodeUnsignedNative<UINT32>(csLid, FALSE, _data);
      coder.encodeUnsignedNative<UINT32>(clLid + 1, FALSE, _data + sizeof(UINT32));
   }

   void lsmCLIdKey::reset()
   {
      ossMemset(_data, 0, sizeof(_data));
   }

} // namespace vessel
} // namespace engine
