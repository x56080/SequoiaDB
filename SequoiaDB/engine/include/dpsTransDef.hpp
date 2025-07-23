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

   Source File Name = dpsTransDef.hpp 

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of OSS component. This file contains declare for data types used in
   SequoiaDB.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          01/28/2019  XJH Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef DPS_TRANS_DEF_HPP__
#define DPS_TRANS_DEF_HPP__

#include "core.hpp"
#include "oss.hpp"

namespace engine
{

   /*
      TRANS_ISOLATION_LEVEL define
   */
   enum TRANS_ISOLATION_LEVEL
   {
      TRANS_ISOLATION_RU = 0, // READ UNCOMMITTED
      TRANS_ISOLATION_RC = 1, // READ COMMITTED
      TRANS_ISOLATION_RS = 2, // READ STABILITY
    //TRANS_ISOLATION_RR = 3, // REPEATABLE READ

      TRANS_ISOLATION_MAX
   } ;

   /*
      DPS_TRANSLOCK_OP_MODE_TYPE define
   */
   enum DPS_TRANSLOCK_OP_MODE_TYPE
   {
      DPS_TRANSLOCK_OP_MODE_TRY = 0,
      DPS_TRANSLOCK_OP_MODE_ACQUIRE,
      DPS_TRANSLOCK_OP_MODE_TEST
   } ;

   #define DPS_TRANS_ISOLATION_DFT        TRANS_ISOLATION_RU
   #define DPS_TRANS_DFT_TIMEOUT          (60)  /* 1 minute */
   #define DPS_TRANS_LOCKWAIT_DFT         FALSE
   #define DPS_TRANS_AUTOCOMMIT_DFT       FALSE
   #define DPS_TRANS_AUTOROLLBACK_DFT     TRUE
   #define DPS_TRANS_USE_RBS_DFT          TRUE

   /*
      TRANS CONFIG MASK
   */
   #define TRANS_CONF_MASK_ISOLATION         0x00000001
   #define TRANS_CONF_MASK_TIMEOUT           0x00000002
   #define TRANS_CONF_MASK_WAITLOCK          0x00000004
   #define TRANS_CONF_MASK_USERBS            0x00000008
   #define TRANS_CONF_MASK_AUTOCOMMIT        0x00000010
   #define TRANS_CONF_MASK_AUTOROLLBACK      0x00000020

   /*
      TRANS LRB AND LRB HEADER
   */
   #define DPS_TRANS_LRB_INIT_DFT         ( 524288 )
   #define DPS_TRANS_LRB_TOTAL_DFT        ( 268435456 )
   #define DPS_TRANS_LRB_MIN              ( 65536 )
   #define DPS_TRANS_LRB_MAX              ( 4294967295 )

}

#endif // DPS_TRANS_DEF_HPP__

