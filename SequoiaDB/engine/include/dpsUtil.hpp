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

   Source File Name = dpsUtil.hpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of OSS component. This file contains declare for data types used in
   SequoiaDB.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          05/08/2019  Linyoub  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef DPSUTIL_HPP_
#define DPSUTIL_HPP_

#include "ossTypes.h"
#include "dpsDef.hpp"
#include "ossMemPool.hpp"
#include "stpLogicalTime.hpp"
#include "../bson/bson.hpp"

namespace engine
{

   #define DPS_TRANS_STR_LEN           ( 64 )

   dpsLogConfig &dpsGetGlobalLogConfig() ;

   const CHAR* dpsTransStatusToString( INT32 status ) ;

   INT32 dpsGetTransIDFromString( const CHAR *pStr, DPS_TRANS_ID &transID ) ;

<<<<<<< HEAD
=======
   // format transaction ID to string format
>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2
   const CHAR* dpsTransIDToString( const DPS_TRANS_ID &transID,
                                   CHAR *pBuff,
                                   UINT32 bufSize ) ;

   // format transaction ID to string format
   ossPoolString dpsTransIDToString( const DPS_TRANS_ID &transID ) ;

   // format transaction SN to string format
   const CHAR *dpsTransSNToString( const DPS_TRANSID_SN &transSN,
                                   CHAR *buffer,
                                   UINT32 bufferSize ) ;

   // format transaction SN to string format
   ossPoolString dpsTransSNToString( const DPS_TRANSID_SN &transSN ) ;

   // format transaction SN to HEX string format
   const CHAR *dpsTransSNToHEXString( const DPS_TRANSID_SN &transSN,
                                      CHAR *buffer,
                                      UINT32 bufferSize ) ;

   // format transaction SN to HEX string format
   ossPoolString dpsTransSNToHEXString( const DPS_TRANSID_SN &transSN ) ;

   // format transaction time to string format
   const CHAR* dpsTransTimeToString( const stpLogicalTimeUS &time,
                                     CHAR *buffer,
                                     UINT32 bufferSize ) ;

   // format transaction time to string format
   ossPoolString dpsTransTimeToString( const stpLogicalTimeUS &time ) ;

   const CHAR* dpsTransIDAttrToString( const DPS_TRANS_ID &transID,
                                       CHAR *pBuff,
                                       UINT32 bufSize ) ;

   ossPoolString dpsTransIDAttrToString( const DPS_TRANS_ID &transID ) ;

<<<<<<< HEAD
=======
   // format transaction ID into BSON object
   INT32 dpsTransIDToBSON( const DPS_TRANS_ID &transID,
                           bson::BSONObjBuilder &builder ) ;

   // format transaction ID into BSON object with field name
   INT32 dpsTransIDToBSON( const DPS_TRANS_ID &transID,
                           bson::BSONObjBuilder &builder,
                           const CHAR *fieldName ) ;

   // parse BSON object into transaction ID
   INT32 dpsTransIDFromBSON( const bson::BSONObj &object,
                             DPS_TRANS_ID &transID ) ;

   // calculate hash value of transaction ID
   UINT64 dpsTransIDHashMod( const DPS_TRANS_ID &transID ) ;
   UINT32 dpsTransIDHashMod( const DPS_TRANS_ID &transID, UINT32 modSize ) ;

   typedef struct _dpsTransIDHash
   {
      std::size_t operator()( const DPS_TRANS_ID &transID ) const
      {
         return (std::size_t)( transID.getGlobSN() ) ;
      }
   } dpsTransIDHash ;

>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2
   void dpsFlags2String( UINT16 flags, CHAR * pBuffer, INT32 bufSize ) ;

   void dpsAppendFlagString( CHAR * pBuffer, INT32 bufSize,
                             const CHAR *flagStr ) ;

   typedef ossPoolSet< DPS_TRANS_ID > DPS_TRANS_ID_SET ;

<<<<<<< HEAD
   // downgrade transaction ID from v1 to v0
   // WARNING: will lose high 16 bits of timestamp
   DPS_TRANS_ID dpsTransIDDowngrade( const dpsTransID_v1 &transID ) ;

=======
>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2
}

#endif // DPSUTIL_HPP_


