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

   Source File Name = utilArguments.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          01/03/2018  HGM Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef UTIL_ARGUMENTS_HPP_
#define UTIL_ARGUMENTS_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "ossUtil.hpp"

namespace engine
{

   #define UTIL_ARG_FIELD_EMPTY              ( 0x00000000 )
   #define UTIL_ARG_FIELD_ALL                ( 0xFFFFFFFF )

   #define UTIL_CL_NAME_FIELD                ( 0x00000001 )
   #define UTIL_CL_SHDKEY_FIELD              ( 0x00000002 )
   #define UTIL_CL_REPLSIZE_FIELD            ( 0x00000004 )
   #define UTIL_CL_ENSURESHDIDX_FIELD        ( 0x00000008 )
   #define UTIL_CL_SHDTYPE_FIELD             ( 0x00000010 )
   #define UTIL_CL_PARTITION_FIELD           ( 0x00000020 )
   #define UTIL_CL_COMPRESSED_FIELD          ( 0x00000040 )
   #define UTIL_CL_ISMAINCL_FIELD            ( 0x00000080 )
   #define UTIL_CL_AUTOSPLIT_FIELD           ( 0x00000100 )
   #define UTIL_CL_AUTOREBALANCE_FIELD       ( 0x00000200 )
   #define UTIL_CL_AUTOIDXID_FIELD           ( 0x00000400 )
   #define UTIL_CL_COMPRESSTYPE_FIELD        ( 0x00000800 )
   #define UTIL_CL_CAPPED_FIELD              ( 0x00001000 )
   #define UTIL_CL_MAXREC_FIELD              ( 0x00002000 )
   #define UTIL_CL_MAXSIZE_FIELD             ( 0x00004000 )
   #define UTIL_CL_OVERWRITE_FIELD           ( 0x00008000 )
   #define UTIL_CL_STRICTDATAMODE_FIELD      ( 0x00010000 )
   #define UTIL_CL_AUTOINC_FIELD             ( 0x00020000 )
   #define UTIL_CL_LOBKEYFORMAT_FIELD        ( 0x00040000 )
   #define UTIL_CL_REPAIRCHECK_FIELD         ( 0x00080000 )
// #define UTIL_CL_AUTOINCREMENT_FIELD       ( 0x00100000 )  The same with UTIL_CL_AUTOINC_FIELD, deleted
   #define UTIL_CL_NOTRANS_FIELD             ( 0x00200000 )
   #define UTIL_CL_CONSISTENCYSTRATEGY_FIELD ( 0x00400000 )
   #define UTIL_CL_DATASOURCE_FIELD          ( 0x00800000 )

   // mask for one field of autoincrement filed attr.
   #define UTIL_CL_AUTOINC_INCREMENT_FIELD   ( 0x00000001 )
   #define UTIL_CL_AUTOINC_STARTVALUE_FIELD  ( 0x00000002 )
   #define UTIL_CL_AUTOINC_MINVALUE_FIELD    ( 0x00000004 )
   #define UTIL_CL_AUTOINC_MAXVALUE_FIELD    ( 0x00000008 )
   #define UTIL_CL_AUTOINC_CACHESIZE_FIELD   ( 0x00000010 )
   #define UTIL_CL_AUTOINC_ACQUIRESIZE_FIELD ( 0x00000020 )
   #define UTIL_CL_AUTOINC_CYCLED_FIELD      ( 0x00000040 )
   #define UTIL_CL_AUTOINC_GENERATED_FIELD   ( 0x00000080 )
   #define UTIL_CL_AUTOINC_CURVALUE_FIELD    ( 0x00000100 )
   #define UTIL_CL_AUTOINC_INITIAL_FIELD     ( 0x00000200 )
   #define UTIL_CL_AUTOINC_CYCLEDCOUNT_FIELD ( 0x00000400 )

   #define UTIL_CS_NAME_FIELD                ( 0x00000001 )
   #define UTIL_CS_DOMAIN_FIELD              ( 0x00000002 )
   #define UTIL_CS_PAGESIZE_FIELD            ( 0x00000004 )
   #define UTIL_CS_LOBPAGESIZE_FIELD         ( 0x00000008 )
   #define UTIL_CS_CAPPED_FIELD              ( 0x00000010 )

   #define UTIL_DOMAIN_NAME_FIELD            ( 0x00000001 )
   #define UTIL_DOMAIN_GROUPS_FIELD          ( 0x00000002 )
   #define UTIL_DOMAIN_AUTOSPLIT_FIELD       ( 0x00000004 )
   #define UTIL_DOMAIN_AUTOREBALANCE_FIELD   ( 0x00000008 )
   #define UTIL_DOMAIN_ACTIVE_LOCATION_FIELD ( 0x00000010 )
   #define UTIL_DOMAIN_LOCATION_FIELD        ( 0x00000020 )

}

#endif // UTIL_ARGUMENTS_HPP_
