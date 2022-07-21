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

namespace engine
{
namespace vessel
{
   ///////////////////////////// lsmIndexIdKey
   void lsmIndexIdKey::init(const globalIndexID &id)
   {
#ifdef SDB_BIG_ENDIAN
      globalIndexID *dataPtr = reinterpret_cast<globalIndexID *>(_data);
      *dataPtr = id;
#else
      UINT32 csLid = id.getLogicalCSID();
      UINT32 clLid = id.getLogicalCLID();
      UINT32 indexLid = id.getLogicalIndexID();
      UINT32 *val = reinterpret_cast<UINT32 *>(_data);
      ossEndianConvert4(csLid, (*val));
      val = reinterpret_cast<UINT32 *>(_data + sizeof(csLid));
      ossEndianConvert4(clLid, (*val));
      val = reinterpret_cast<UINT32 *>(_data + sizeof(csLid) + sizeof(clLid));
      ossEndianConvert4(indexLid, (*val));
#endif
   }

   void lsmIndexIdKey::initAsUpKey(const globalIndexID &id)
   {
      SDB_ASSERT(id.isValid(), "can not be invalid");
      globalIndexID upID(id.getLogicalCSID(),
                         id.getLogicalCLID(),
                         id.getLogicalIndexID() + 1);
#ifdef SDB_BIG_ENDIAN
      globalIndexID *dataPtr = reinterpret_cast<globalIndexID *>(_data);
      *dataPtr = id;
#else
      UINT32 csLid = upID.getLogicalCSID();
      UINT32 clLid = upID.getLogicalCLID();
      UINT32 indexLid = upID.getLogicalIndexID();
      UINT32 *val = reinterpret_cast<UINT32 *>(_data);
      ossEndianConvert4(csLid, (*val));
      val = reinterpret_cast<UINT32 *>(_data + sizeof(csLid));
      ossEndianConvert4(clLid, (*val));
      val = reinterpret_cast<UINT32 *>(_data + sizeof(csLid) + sizeof(clLid));
      ossEndianConvert4(indexLid, (*val));
#endif
   }

   void lsmIndexIdKey::reset()
   {
      ossMemset(_data, 0, sizeof(_data));
   }

   //////////////////////////////// lsmCLIndexKey
   void lsmIndexManifestKey::init(UINT32 csLid, UINT32 clLid)
   {
#ifdef SDB_BIG_ENDIAN
      UINT32 *dataPtr = reinterpret_cast<UINT32 *>(_data);
      *dataPtr = csLid;
      dataPtr = reinterpret_cast<UINT32 *>(_data + sizeof(csLid));
      *dataPtr = clLid;
#else
      UINT32 *val = reinterpret_cast<UINT32 *>(_data);
      ossEndianConvert4(csLid, (*val));
      val = reinterpret_cast<UINT32 *>(_data + sizeof(csLid));
      ossEndianConvert4(clLid, (*val));
#endif
   }

   void lsmIndexManifestKey::initAsUpKey(UINT32 csLid, UINT32 clLid)
   {
      SDB_ASSERT(DMS_INVALID_LOGICCSID != csLid, "can not be invalid");
      SDB_ASSERT(DMS_INVALID_LOGICCLID != clLid, "can not be invalid");
      UINT32 upCLLid = clLid + 1;
#ifdef SDB_BIG_ENDIAN
      UINT32 *dataPtr = reinterpret_cast<UINT32 *>(_data);
      *dataPtr = csLid;
      dataPtr = reinterpret_cast<UINT32 *>(_data + sizeof(csLid));
      *dataPtr = upCLLid;
#else
      UINT32 *val = reinterpret_cast<UINT32 *>(_data);
      ossEndianConvert4(csLid, (*val));
      val = reinterpret_cast<UINT32 *>(_data + sizeof(csLid));
      ossEndianConvert4(upCLLid, (*val));
#endif
   }

   void lsmIndexManifestKey::reset()
   {
      ossMemset(_data, 0, sizeof(_data));
   }

} // namespace vessel
} // namespace engine
