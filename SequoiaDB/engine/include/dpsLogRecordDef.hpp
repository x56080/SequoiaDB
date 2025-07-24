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

   Source File Name = dpsLogRecordDef.hpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of DPS component. This file contains declare for dpsLogWrapper.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/27/2012  YW  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef DPSLOGRECORDDEF_HPP_
#define DPSLOGRECORDDEF_HPP_

#include "dpsDef.hpp"

namespace engine
{
   enum DPS_LOG_PUBLIC
   {
      DPS_LOG_PUBLIC_INVALID = 0,
      DPS_LOG_PUBLIC_BEGIN = 200,
      DPS_LOG_PUBLIC_FULLNAME = 201,
      DPS_LOG_PUBLIC_TRANSID = 202,
      DPS_LOG_PUBLIC_PRETRANS = 203,
      DPS_LOG_PUBLIC_RELATED_TRANS = 204,    // only for rollback trans,
                                             // mapping to really trans lsn
      DPS_LOG_PUBLIC_FIRSTTRANS = 205
   } ;

/// number in public can not be used in definition !

   enum DPS_LOG_INSERT
   {
      DPS_LOG_INSERT_OBJ = 1,
   } ;

   enum DPS_LOG_UPDATE
   {
      DPS_LOG_UPDATE_OLDMATCH =1,
      DPS_LOG_UPDATE_OLDOBJ = 2,
      DPS_LOG_UPDATE_NEWMATCH = 3,
      DPS_LOG_UPDATE_NEWOBJ = 4,
   } ;

   enum DPS_LOG_DELETE
   {
      DPS_LOG_DELETE_OLDOBJ = 1,
   } ;

   enum DPS_LOG_CSCRT
   {
      DPS_LOG_CSCRT_CSNAME = 1,
      DPS_LOG_CSCRT_PAGESIZE = 2,
      DPS_LOG_CSCRT_LOBPAGESZ = 3,
   } ;

   enum DPS_LOG_CSDEL
   {
      DPS_LOG_CSDEL_CSNAME = 1,
   } ;

   enum DPS_LOG_CSRENAME
   {
      DPS_LOG_CSRENAME_CSNAME       = 1,
      DPS_LOG_CSRENAME_NEWNAME      = 2
   } ;

   enum DPS_LOG_CLCRT
   {
      DPS_LOG_CLCRT_ATTRIBUTE = 1,
      DPS_LOG_CLCRT_COMPRESS_TYPE = 2
   } ;

   enum DPS_LOG_CLDEL
   {
   } ;

   enum DPS_LOG_IXCRT
   {
      DPS_LOG_IXCRT_IX = 1,
      DPS_LOG_IXCRT_IX_MODE = 2,
   } ;

   enum DPS_LOG_IXDEL
   {
      DPS_LOG_IXDEL_IX = 1,
   } ;

   enum DPS_LOG_CLRENAME
   {
      DPS_LOG_CLRENAME_CSNAME =1,
      DPS_LOG_CLRENAME_CLOLDNAME =2,
      DPS_LOG_CLRENAME_CLNEWNAME =3,
   } ;

   enum DPS_LOG_CLTRUNC
   {
   } ;

   enum DPS_LOG_TS_COMMIT
   {
   } ;

   enum DPS_LOG_TS_ROLLBACK
   {
   } ;

   enum DPS_LOG_INVALIDCATA
   {
   } ;

   enum DPS_LOG_ROW
   {
      DPS_LOG_ROW_ROWDATA = 1,
   };

   enum DPS_LOG_LOB
   {
      DPS_LOG_LOB_OID = 1,
      DPS_LOG_LOB_SEQUENCE,
      DPS_LOG_LOB_OFFSET,
      DPS_LOG_LOB_HASH,
      DPS_LOG_LOB_LEN,
      DPS_LOG_LOB_DATA,
      DPS_LOG_LOB_PAGE,
      DPS_LOG_LOB_OLD_LEN,
      DPS_LOG_LOB_OLD_DATA,
   } ;
}


#endif

