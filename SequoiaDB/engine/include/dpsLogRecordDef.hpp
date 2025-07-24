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
   enum DPS_LOG_PUBLIC : UINT8
   {
      DPS_LOG_PUBLIC_INVALID = 0,
      DPS_LOG_PUBLIC_BEGIN = 200,
      DPS_LOG_PUBLIC_FULLNAME = 201,         // cl full name

      // transaction ID
      // V0: whole transaction ID
      // V1: serial number with global transaction tag
      DPS_LOG_PUBLIC_TRANSID = 202,
      DPS_LOG_PUBLIC_PRETRANS = 203,
      DPS_LOG_PUBLIC_RELATED_TRANS = 204,    // only for rollback trans,
                                             // mapping to really trans lsn
      DPS_LOG_PUBLIC_FIRSTTRANS = 205,
<<<<<<< HEAD
      DPS_LOG_PUBLIC_TIME = 206,

      // 207 - 209 reserved for global transaction
      // node ID component for transaction ID of V1
      DPS_LOG_PUBLIC_TRANSID_NODEID = 207,


      DPS_LOG_PUBLIC_NEW_UNQIDX_HASH = 210,
      DPS_LOG_PUBLIC_OLD_UNQIDX_HASH = 211
=======
      // real time of record ( related to --logtimeon option )
      DPS_LOG_PUBLIC_TIME = 206,

      // global transaction components
      // node ID component for transaction ID of V1
      DPS_LOG_PUBLIC_TRANSID_NODEID = 207,
      // time component of logical time for global transaction
      DPS_LOG_PUBLIC_TRANS_TIME = 208,
      // time error component of logical time for global transaction
      DPS_LOG_PUBLIC_TRANS_TIME_ERROR = 209,

      DPS_LOG_PUBLIC_NEW_UNQIDX_HASH = 210,
      DPS_LOG_PUBLIC_OLD_UNQIDX_HASH = 211,
	  
	  ///vessel only
      DPS_LOG_PUBLIC_VESSEL_GPID = 220,
      DPS_LOG_PUBLIC_VESSEL_FULL_PAGE_DUMP = 221,


      DPS_LOG_PUBLIC_CL_UNIQUE_ID = 230,
      DPS_LOG_PUBLIC_PAGE_ADDR = 231,
      DPS_LOG_PUBLIC_OPL_NODE = 232,
      DPS_LOG_PUBLIC_OPL_ROLLBACK_INFO = 233,
>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2
   } ;

/// number in public can not be used in definition !

   enum DPS_LOG_INSERT
   {
      DPS_LOG_INSERT_OBJ = 1
   } ;

   enum DPS_LOG_UPDATE
   {
      DPS_LOG_UPDATE_OLDMATCH =1,
      DPS_LOG_UPDATE_OLDOBJ = 2,
      DPS_LOG_UPDATE_NEWMATCH = 3,
      DPS_LOG_UPDATE_NEWOBJ = 4,
      DPS_LOG_UPDATE_OLDSHARDINGKEY = 5,
      DPS_LOG_UPDATE_NEWSHARDINGKEY = 6,
      DPS_LOG_UPDATE_WRITEMOD = 7
   } ;

   enum DPS_LOG_DELETE
   {
      DPS_LOG_DELETE_OLDOBJ = 1,
      // only used for mark deleting
      DPS_LOG_DELETE_POSITION = 2
   } ;

   enum DPS_LOG_POP
   {
      DPS_LOG_POP_LID = 1,
      DPS_LOG_POP_DIRECTION = 2
   } ;

   enum DPS_LOG_CSCRT
   {
      DPS_LOG_CSCRT_CSNAME = 1,
      DPS_LOG_CSCRT_PAGESIZE = 2,
      DPS_LOG_CSCRT_LOBPAGESZ = 3,
      DPS_LOG_CSCRT_CSTYPE = 4,
      DPS_LOG_CSCRT_CSUNIQUEID = 5,

      ///vessel format
      DPS_LOG_CSCRT_VESSEL_SID = 100,
      DPS_LOG_CSCRT_VESSEL_META = 101,
      DPS_LOG_CSCRT_VESSEL_OPTIONS = 102,
   } ;

   enum DPS_LOG_CSDEL
   {
      DPS_LOG_CSDEL_CSNAME = 1,
      DPS_LOG_CSDEL_OPTIONS = 2,
   } ;

   enum DPS_LOG_CSRENAME
   {
      DPS_LOG_CSRENAME_CSNAME = 1,
      DPS_LOG_CSRENAME_NEWNAME      = 2
   } ;

   enum DPS_LOG_CLCRT
   {
      DPS_LOG_CLCRT_ATTRIBUTE = 1,
      DPS_LOG_CLCRT_COMPRESS_TYPE = 2,
      DPS_LOG_CLCRT_EXT_OPTIONS = 3,
      DPS_LOG_CLCRT_CLUNIQUEID = 4,
<<<<<<< HEAD
      DPS_LOG_CLCRT_IDIDX_DEF = 5
=======
      DPS_LOG_CLCRT_IDIDX_DEF = 5,

      ///vessel format
      /// DPS_LOG_PUBLIC_FULLNAME
      /// DPS_LOG_PUBLIC_VESSEL_GPID
      DPS_LOG_CLCRT_VESSEL_MBID = 100,
      DPS_LOG_CLCRT_VESSEL_INNER_ID = 101,
      DPS_LOG_CLCRT_VESSEL_LOGICAL_ID = 102,
      DPS_LOG_CLCRT_VESSEL_ADJUNCT = 103,
      
      
>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2
   } ;

   enum DPS_LOG_CLDEL
   {
      DPS_LOG_CLDEL_OPTIONS = 1
   } ;

   enum DPS_LOG_IXCRT
   {
      DPS_LOG_IXCRT_IX = 1,
      DPS_LOG_IXCRT_IX_MODE = 2,
      DPS_LOG_IXCRT_OPTION = 3,
<<<<<<< HEAD
=======

      ///vessel format
      /// DPS_LOG_PUBLIC_FULLNAME
      DPS_LOG_IXCRT_IX_SLOT = 100,
      DPS_LOG_IXCRT_IX_INDEX_ID = 101,
      DPS_LOG_IXCRT_IX_DEF_OBJ = 102,
      
>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2
   } ;

   enum DPS_LOG_IXCRT_END
   {
      ///vessel format
      /// DPS_LOG_PUBLIC_FULLNAME

      DPS_LOG_IXCRT_END_IX_SLOT = 100,
      DPS_LOG_IXCRT_END_IX_INDEX_ID = 101,
      DPS_LOG_IXCRT_END_IX_NAME = 102,
      DPS_LOG_IXCRT_END_RC = 103
   };

   enum DPS_LOG_IXDEL
   {
      DPS_LOG_IXDEL_IX = 1,
      DPS_LOG_IXDEL_OPTION = 2
   } ;

   enum DPS_LOG_CLRENAME
   {
      DPS_LOG_CLRENAME_CSNAME = 1 ,
      DPS_LOG_CLRENAME_CLOLDNAME = 2 ,
      DPS_LOG_CLRENAME_CLNEWNAME = 3
   } ;

   enum DPS_LOG_CLTRUNC
   {
      DPS_LOG_CLTRUNC_OPTIONS = 1
   } ;

   enum DPS_LOG_TS_COMMIT
   {
      DPS_LOG_TSCOMMIT_NODE_NUM  = 2,
      DPS_LOG_TSCOMMIT_NODES     = 3,
      DPS_LOG_TSCOMMIT_ATTR      = 4
   } ;

   #define DPS_TS_COMMIT_ATTR_PRE            ( 1 )
   #define DPS_TS_COMMIT_ATTR_SND            ( 2 )

   #define DPS_TS_COMMIT_ATTR_SND_STR        "Snd-Commit"
   #define DPS_TS_COMMIT_ATTR_PRE_STR        "Pre-Commit"

   enum DPS_LOG_TS_ROLLBACK
   {
   } ;

   enum DPS_LOG_INVALIDCATA
   {
      DPS_LOG_INVALIDCATA_TYPE = 1,
      DPS_LOG_INVALIDCATA_IXNAME
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
      DPS_LOG_LOB_PAGE_SIZE
   } ;

   enum DPS_LOG_ANALYZE
   {
      DPS_LOG_ANALYZE_CSNAME = 1,
      DPS_LOG_ANALYZE_CLNAME,
      DPS_LOG_ANALYZE_IXNAME,
      DPS_LOG_ANALYZE_MODE
   } ;

   enum DPS_LOG_ALTER
   {
      DPS_LOG_ALTER_OBJECT_TYPE = 1,
      DPS_LOG_ALTER_OBJECT
   } ;

   enum DPS_LOG_ADDUNIQUEID
   {
      DPS_LOG_ADDUNIQUEID_CSNAME = 1,
      DPS_LOG_ADDUNIQUEID_CSUNIQUEID,
      DPS_LOG_ADDUNIQUEID_CLINFO
   } ;

   enum DPS_LOG_RETURN
   {
      DPS_LOG_RETURN_OPTIONS = 1
   } ;

<<<<<<< HEAD
=======
   /// logical page space page management
   enum DPS_LOG_VESSEL_LPS_PM
   {
      DPS_LOG_VESSEL_LPS_PM_SID_AND_TYPE = 1,
      DPS_LOG_VESSEL_LPS_PM_DELTA_LOG = 2,
   };

   enum DPS_LOG_VESSEL_COPY_PAGE
   {
      //DPS_LOG_PUBLIC_VESSEL_GPID
      //DPS_LOG_PUBLIC_VESSEL_FULL_PAGE_DUMP
      DPS_LOG_VESSEL_COPY_PAGE_LPID = 1,
   };

   enum DPS_LOG_VESSEL_CL_RECORD_UPDATE
   {
      //DPS_LOG_PUBLIC_VESSEL_GPID
      DPS_LOG_VESSEL_CL_RECORD_UPDATE_LPID = 1,
      DPS_LOG_VESSEL_CL_RECORD_UPDATE_MASK = 2,
      DPS_LOG_VESSEL_CL_RECORD_UPDATE_OLD = 3,
      DPS_LOG_VESSEL_CL_RECORD_UPDATE_NEW = 4,
   };

   enum DPS_LOG_VESSEL_ROUTE_PAGE_INSERT
   {
      //DPS_LOG_PUBLIC_VESSEL_GPID.
      DPS_LOG_VESSEL_ROUTE_PAGE_INSERT_LPID = 1,
      DPS_LOG_VESSEL_ROUTE_PAGE_INSERT_OLD_CNT = 2,
      DPS_LOG_VESSEL_ROUTE_PAGE_INSERT_PAGES = 3,
   };

   enum DPS_LOG_VESSEL_RDP_INSERT
   {
      ///DPS_LOG_PUBLIC_FULLNAME
      ///DPS_LOG_PUBLIC_TRANSID
      ///DPS_LOG_PUBLIC_NEW_UNQIDX_HASH

      // DPS_LOG_PUBLIC_VESSEL_GPID
      DPS_LOG_VESSEL_RDP_INSERT_RID = 1,
      DPS_LOG_VESSEL_RDP_INSERT_PAGE_HEAD = 2,
      DPS_LOG_VESSEL_RDP_INSERT_OLD_PAGE_HEAD = 3,
      DPS_LOG_VESSEL_RDP_INSERT_SLOT = 4,
      DPS_LOG_VESSEL_RDP_INSERT_RECORD_AND_HEAD = 5,
      DPS_LOG_VESSEL_RDP_INSERT_UNCOMPRESSED_RECORD = 6,
      DPS_LOG_VESESL_RDP_INSERT_STRIPING = 7,
   } ;

   enum DPS_LOG_VESSEL_PAGE_INIT
   {
      // DPS_LOG_PUBLIC_VESSEL_GPID
      DPS_LOG_VESSEL_PAGE_INIT_LPID = 1,
      DPS_LOG_VESSEL_PAGE_INIT_PSV = 2,
      DPS_LOG_VESSEL_PAGE_INIT_PAGE_TYPE = 3,
      DPS_LOG_VESSEL_PAGE_INIT_ADJUNCT = 4
   };//enum DPS_LOG_VESSEL_PAGE_INIT

   enum DPS_LOG_VESSEL_CSMB_UPDATE
   {
      // DPS_LOG_PUBLIC_VESSEL_GPID
      DPS_LOG_VESSEL_CSMB_UPDATE_MASK = 1,
      DPS_LOG_VESSEL_CSMB_UPDATE_OLD = 2,
      DPS_LOG_VESSEL_CSMB_UPDATE_NEW = 3
   }; // enum DPS_LOG_VESSEL_CSMB_UPDATE

>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2
}

#endif
