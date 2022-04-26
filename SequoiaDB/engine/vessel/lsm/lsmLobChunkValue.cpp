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

   Source File Name = lsmLobChunkValue.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/20/2022  LYC  Initial Draft

   Last Changed =

******************************************************************************/
#include "vessel/lsm/lsmLobChunkValue.h"
#include "dpsDef.hpp"

namespace engine
{
namespace vessel
{
   constexpr CHAR *LSM_LOBC_VALUE_MDID = "mbid";
   constexpr CHAR *LSM_LOBC_VALUE_LSN = "lsn";
   constexpr CHAR *LSM_LOBC_VALUE_EXTENTS = "extents";
   constexpr CHAR *LSM_LOBC_VALUE_EXT_PCNT = "pcnt";
   constexpr CHAR *LSM_LOBC_VALUE_EXT_PID = "pid";
   constexpr CHAR *LSM_LOBC_VALUE_EXT_PSV = "psv";
   constexpr CHAR *LSM_LOBC_VALUE_EXT_SZ = "size";

   bson::BSONObj buildLsmLobChunkValue(UINT16 mbid,
                                       UINT64 lsn,
                                       const ossPoolVector<lextentDescriptor> &exts)
   {
      SDB_ASSERT(INVALID_CL_MB_ID != mbid, "can not be invalid");
      SDB_ASSERT(DPS_INVALID_LSN_OFFSET != lsn, "can not be invalid");
      SDB_ASSERT(!exts.empty(), "can not be empty");

      bson::BSONObjBuilder builder;
      builder.append(LSM_LOBC_VALUE_MDID, (INT32)mbid);
      builder.append(LSM_LOBC_VALUE_LSN, (INT64)lsn);

      bson::BSONArrayBuilder extentsBuilder(builder.subarrayStart(LSM_LOBC_VALUE_EXTENTS));
      for (UINT32 i = 0; i < exts.size(); ++i)
      {
         bson::BSONObjBuilder extBuilder(extentsBuilder.subobjStart());
         const lextentDescriptor &desc = exts[i];
         SDB_ASSERT(desc.isValid(), "can not be invalid");
         extBuilder.append(LSM_LOBC_VALUE_EXT_PCNT, (INT32)desc.pcnt);
         extBuilder.append(LSM_LOBC_VALUE_EXT_PID, (INT32)desc.pid);
         extBuilder.append(LSM_LOBC_VALUE_EXT_PSV, (INT32)desc.psv);
         extBuilder.append(LSM_LOBC_VALUE_EXT_SZ, (INT32)desc.size);
         extBuilder.doneFast();
      }
      extentsBuilder.doneFast();
      return builder.obj();
   }

   INT32 extractLsmLobChunkValue(const bson::BSONObj &value,
                                 UINT16 &mbid,
                                 UINT64 &lsn,
                                 ossPoolVector<lextentDescriptor> &exts)
   {
      INT32 rc = SDB_OK;
      bson::BSONElement valueEle;
      bson::BSONObj arrayObj;
      bson::BSONObjIterator it;

      exts.clear();
      mbid = INVALID_CL_MB_ID;
      lsn = DPS_INVALID_LSN_OFFSET;
      if (!value.isValid())
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      valueEle = value.getField(LSM_LOBC_VALUE_MDID);
      if (bson::NumberInt != valueEle.type())
      {
         rc = SDB_INVALIDARG;
         PD_LOG(PDERROR, "invalid mbid, value:%s",
                value.toString().c_str());
         goto error;
      }
      mbid = (UINT16)valueEle.numberInt();

      valueEle = value.getField(LSM_LOBC_VALUE_LSN);
      if (bson::NumberLong != valueEle.type())
      {
         rc = SDB_INVALIDARG;
         PD_LOG(PDERROR, "invalid lsn, value:%s",
                value.toString().c_str());
         goto error;
      }
      lsn = (UINT64)valueEle.numberLong();

      valueEle = value.getField(LSM_LOBC_VALUE_EXTENTS);
      if (bson::Array != valueEle.type())
      {
         rc = SDB_INVALIDARG;
         PD_LOG(PDERROR, "invalid extents array, value:%s",
                value.toString().c_str());
         goto error;
      }

      arrayObj = valueEle.embeddedObject();
      it = bson::BSONObjIterator(arrayObj);
      while(it.more())
      {
         bson::BSONElement extEle = it.next();
         bson::BSONObj extObj;
         bson::BSONElement e;
         lextentDescriptor desc;

         if (bson::Object != extEle.type())
         {
            rc = SDB_INVALIDARG;
            PD_LOG(PDERROR, "invalid lob extent, value:%s",
                   value.toString().c_str());
            goto error;
         }

         extObj = extEle.embeddedObject();
         e = extObj.getField(LSM_LOBC_VALUE_EXT_PCNT);
         if (bson::NumberInt != e.type())
         {
            rc = SDB_INVALIDARG;
            PD_LOG(PDERROR, "invalid extent page count, value:%s", 
                   extObj.toString().c_str());
            goto error;
         }
         desc.pcnt = (UINT32)e.numberInt();

         e = extObj.getField(LSM_LOBC_VALUE_EXT_PID);
         if (bson::NumberInt != e.type())
         {
            rc = SDB_INVALIDARG;
            PD_LOG(PDERROR, "invalid extent page id, value:%s",
                   extObj.toString().c_str());
            goto error;
         }
         desc.pid = (UINT32)e.numberInt();

         e = extObj.getField(LSM_LOBC_VALUE_EXT_PSV);
         if (bson::NumberInt != e.type())
         {
            rc = SDB_INVALIDARG;
            PD_LOG(PDERROR, "invalid extent page snapshot version, value:%s",
                   extObj.toString().c_str());
            goto error;
         }
         desc.psv = (UINT32)e.numberInt();

         e = extObj.getField(LSM_LOBC_VALUE_EXT_SZ);
         if (bson::NumberInt != e.type())
         {
            rc = SDB_INVALIDARG;
            PD_LOG(PDERROR, "invalid extent size, value:%s",
                   extObj.toString().c_str());
            goto error;
         }
         desc.size = (UINT32)e.numberInt();
         exts.push_back(desc);
      }
      
      if (exts.empty())
      {
         rc = SDB_INVALIDARG;
         PD_LOG(PDERROR, "invalid lob extents, value:%s",
                value.toString().c_str());
         goto error;
      }

   done:
      return rc;
   error:
      mbid = INVALID_CL_MB_ID;
      lsn = DPS_INVALID_LSN_OFFSET;
      exts.clear();
      goto done;
   }
} // namespace vessel
} // namespace engine