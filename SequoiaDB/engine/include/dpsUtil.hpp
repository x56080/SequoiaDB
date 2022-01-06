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

   // format transaction ID to string format
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

}

#endif // DPSUTIL_HPP_


