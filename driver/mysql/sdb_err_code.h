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

   
*******************************************************************************/
#ifndef SDB_ERR_CODE__H
#define SDB_ERR_CODE__H

#define IS_SDB_NET_ERR(rc) ( SDB_NETWORK_CLOSE == rc || SDB_NETWORK == rc || SDB_NOT_CONNECTED == rc || SDB_SYS == rc )

enum SDB_ERR_CODE
{
   SDB_ERR_OK = 0,
   SDB_ERR_COND_UNKOWN_ITEM            = 30000,
   SDB_ERR_COND_PART_UNSUPPORTED,
   SDB_ERR_COND_UNSUPPORTED,
   SDB_ERR_COND_INCOMPLETED,
   SDB_ERR_COND_UNEXPECTED_ITEM,
   SDB_ERR_OOM,
   SDB_ERR_OVF,
   SDB_ERR_SIZE_OVF,
   SDB_ERR_TYPE_UNSUPPORTED,
   SDB_ERR_INVALID_ARG,
   SDB_ERR_EOF,

   SDB_ERR_INNER_CODE_BEGIN = 40000,
   SDB_ERR_INNER_CODE_END = 50000
};

void convert_sdb_code( int &rc ) ;

int get_sdb_code( int rc ) ;

#endif
