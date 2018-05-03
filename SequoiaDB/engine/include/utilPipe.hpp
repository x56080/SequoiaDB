/*******************************************************************************

   Copyright (C) 2011-2018 SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   Source File Name = utilPipe.hpp

   Descriptive Name =

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/28/2012  YW  Initial Draft

   Last Changed =

*******************************************************************************/

// this file is only intened to be included by sdb.cpp and sdbbp.cpp
#ifndef UTILPIPE_HPP__
#define UTILPIPE_HPP__

#include "core.h"


#define SH_VERIFY_RC                                             \
   if ( rc != SDB_OK ) {                                          \
      PD_LOG ( PDERROR , "%s, rc = %d" , getErrDesp(rc) , rc ) ;  \
      goto error ;                                                \
   }

#define SH_VERIFY_COND(cond, ret)  \
   if (!(cond)) {                   \
      rc=ret;                       \
      SH_VERIFY_RC                 \
   }

#define SDB_SHELL_WAIT_PIPE_PREFIX        "sdb-shell-wait-"
#define SDB_SHELL_F2B_PIPE_PREFIX         "sdb-shell-f2b-"
#define SDB_SHELL_B2F_PIPE_PREFIX         "sdb-shell-b2f-"

INT32 getWaitPipeName ( const OSSPID & ppid , CHAR * buf , UINT32 bufSize ) ;
INT32 getPipeNames( const OSSPID & ppid , CHAR * f2bName , UINT32 f2bSize ,
                    CHAR * b2fName , UINT32 b2fSize ) ;
INT32 getPipeNames2( const OSSPID & ppid , const OSSPID & pid ,
                     CHAR * f2bName , UINT32 f2bSize ,
                     CHAR * b2fName , UINT32 b2fSize ) ;

INT32 getPipeNames1( CHAR * bpf2bName , UINT32 bpf2bSize ,
                     CHAR * bpb2fName , UINT32 bpb2fSize ,
                     CHAR * f2bName , CHAR * b2fName ) ;

void  clearDirtyShellPipe( const CHAR *prefix ) ;


#endif //UTILPIPE_HPP__

