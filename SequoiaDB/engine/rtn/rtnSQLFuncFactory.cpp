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

   Source File Name = rtnSQLFuncFactory.cpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains functions for agent processing.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#include "rtnSQLFuncFactory.hpp"
#include "pd.hpp"
#include "rtnSQLFunc.hpp"
#include "ossUtil.hpp"
#include "rtnSQLCount.hpp"
#include "rtnSQLSum.hpp"
#include "rtnSQLMin.hpp"
#include "rtnSQLMax.hpp"
#include "rtnSQLAvg.hpp"
#include "rtnSQLFirst.hpp"
#include "utilStr.hpp"
#include "rtnSQLLast.hpp"
#include "rtnSQLPush.hpp"
#include "rtnSQLAddToSet.hpp"
#include "rtnSQLBuildObj.hpp"
#include "rtnSQLMergeArraySet.hpp"

namespace engine
{
   /// TODO:simply completed. to be optimized.
   INT32  _rtnSQLFuncFactory::create( const CHAR *funcName,
                                      UINT32 paramNum,
                                       _rtnSQLFunc *&func )
   {
      INT32 rc = SDB_OK ;

      if ( 0 == ossStrcasecmp( RTN_SQL_FUNC_COUNT, funcName) &&
           1 == paramNum )
      {
         func = SDB_OSS_NEW _rtnSQLCount( RTN_SQL_FUNC_COUNT ) ;
      }
      else if ( 0 == ossStrcasecmp( RTN_SQL_FUNC_SUM, funcName ) &&
                1 == paramNum )
      {
         func = SDB_OSS_NEW _rtnSQLSum( RTN_SQL_FUNC_SUM ) ;
      }
      else if ( 0 == ossStrcasecmp( RTN_SQL_FUNC_MIN, funcName ) &&
                1 == paramNum )
      {
         func = SDB_OSS_NEW _rtnSQLMin( RTN_SQL_FUNC_MIN ) ;
      }
      else if ( 0 == ossStrcasecmp( RTN_SQL_FUNC_MAX, funcName ) &&
                1 == paramNum )
      {
         func = SDB_OSS_NEW _rtnSQLMax( RTN_SQL_FUNC_MAX ) ;
      }
      else if ( 0 == ossStrcasecmp( RTN_SQL_FUNC_AVG, funcName ) &&
                1 == paramNum )
      {
         func = SDB_OSS_NEW _rtnSQLAvg( RTN_SQL_FUNC_AVG ) ;
      }
      else if ( 0 == ossStrcasecmp( RTN_SQL_FUNC_FIRST, funcName ) &&
                1 == paramNum )
      {
         func = SDB_OSS_NEW _rtnSQLFirst( RTN_SQL_FUNC_FIRST ) ;
      }
      else if ( 0 == ossStrcasecmp( RTN_SQL_FUNC_LAST, funcName ) &&
                1 == paramNum )
      {
         func = SDB_OSS_NEW rtnSQLLast( RTN_SQL_FUNC_LAST ) ;
      }
      else if ( 0 == ossStrcasecmp( RTN_SQL_FUNC_PUSH, funcName ) &&
                1 == paramNum )
      {
         func = SDB_OSS_NEW rtnSQLPush( RTN_SQL_FUNC_PUSH ) ;
      }
      else if ( 0 == ossStrcasecmp( RTN_SQL_FUNC_ADDTOSET, funcName ) &&
                1 == paramNum )
      {
         func = SDB_OSS_NEW rtnSQLAddToSet( RTN_SQL_FUNC_ADDTOSET ) ;
      }
      else if ( 0 == ossStrcasecmp( RTN_SQL_FUNC_BUILDOBJ, funcName ) &&
                paramNum > 0 )
      {
         func = SDB_OSS_NEW rtnSQLBuildObj( RTN_SQL_FUNC_BUILDOBJ ) ;
      }
      else if ( 0 == ossStrcasecmp( RTN_SQL_FUNC_MERGEARRAYSET, funcName ) &&
                1 == paramNum )
      {
         func = SDB_OSS_NEW rtnSQLMergeArraySet( RTN_SQL_FUNC_MERGEARRAYSET ) ;
      }
      else
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      if ( NULL == func )
      {
         PD_LOG( PDERROR, "failed to allocated mem." ) ;
         rc = SDB_OOM ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }
}
