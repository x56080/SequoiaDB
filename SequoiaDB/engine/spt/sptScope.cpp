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

   Source File Name = sptScope.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          31/03/2014  YW  Initial Draft

   Last Changed =

*******************************************************************************/
#include "sptScope.hpp"
#include "pd.hpp"
#include "sptObjDesc.hpp"
#include "ossUtil.hpp"
#include "sptCommon.hpp"

namespace engine
{

   /*
      _sptResultVal implement
   */
   _sptResultVal::_sptResultVal()
   {
   }

   _sptResultVal::~_sptResultVal()
   {
   }

   BOOLEAN _sptResultVal::hasError() const
   {
      return _errStr.empty() ? FALSE : TRUE ;
   }

   const CHAR* _sptResultVal::getErrrInfo() const
   {
      return _errStr.c_str() ;
   }

   void _sptResultVal::setError( const  string &err )
   {
      _errStr = err ;
   }

   /*
      _sptScope implement
   */
   _sptScope::_sptScope()
   {
      _loadMask = 0 ;
   }

   _sptScope::~_sptScope()
   {

   }

   INT32 _sptScope::getLastError() const
   {
      return sdbGetErrno() ;
   }

   const CHAR* _sptScope::getLastErrMsg() const
   {
      return sdbGetErrMsg() ;
   }

   bson::BSONObj _sptScope::getLastErrObj() const
   {
      const CHAR *pObjData = sdbGetErrorObj() ;

      if ( pObjData )
      {
         try
         {
            bson::BSONObj obj( pObjData ) ;
            return obj ;
         }
         catch( std::exception &e )
         {
            PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         }
      }

      return bson::BSONObj() ;
   }

   INT32 _sptScope::loadUsrDefObj( _sptObjDesc *desc )
   {
      INT32 rc = SDB_OK ;
      SDB_ASSERT( NULL != desc, "desc can not be NULL" ) ;
      SDB_ASSERT( NULL != desc->getJSClassName(),
                  "obj name can not be empty" ) ;
      rc = _loadUsrDefObj( desc ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to load object defined by user:%d", rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }
}
