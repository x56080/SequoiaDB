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

   Source File Name = rtnAlterFuncs.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          08/05/2015  YW  Initial Draft

   Last Changed =

*******************************************************************************/
#include "rtn.hpp"
#include "pd.hpp"

using namespace bson ;

namespace engine
{
   INT32 rtnCreateIDIndex( const CHAR *name,
                           const bson::BSONObj &pubArgs,
                           const bson::BSONObj &args,
                           _pmdEDUCB *cb,
                           _dpsLogWrapper *dpsCB )
   {
      INT32 rc = SDB_OK ;
      INT32 sortBufferSize = SDB_INDEX_SORT_BUFFER_DEFAULT_SIZE ;

      if ( args.hasField( IXM_FIELD_NAME_SORT_BUFFER_SIZE ) )
      {
         sortBufferSize = args.getIntField( IXM_FIELD_NAME_SORT_BUFFER_SIZE ) ;
      }

      SDB_ASSERT( NULL != name, "can not be null" ) ;
      SDB_ASSERT( NULL != cb, "can not be null" ) ;
      BSONObj idxDef = BSON( IXM_FIELD_NAME_KEY <<
                             BSON( DMS_ID_KEY_NAME << 1 ) <<
                             IXM_FIELD_NAME_NAME << IXM_ID_KEY_NAME
                             << IXM_FIELD_NAME_UNIQUE <<
                             true << IXM_FIELD_NAME_V << 0 <<
                             IXM_FIELD_NAME_ENFORCED << true ) ;
      rc = rtnCreateIndexCommand( name,
                                  idxDef, cb,
                                  sdbGetDMSCB(),
                                  dpsCB,
                                  TRUE,
                                  sortBufferSize ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to create id index on cl:%s, rc:%d",
                 name, rc ) ;
         goto error ; 
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 rtnCreateIDIndexVerify( const bson::BSONObj &args )
   {
      INT32 rc = SDB_OK ;

      if ( args.hasField( IXM_FIELD_NAME_SORT_BUFFER_SIZE ) )
      {
         BSONElement e = args.getField( IXM_FIELD_NAME_SORT_BUFFER_SIZE ) ;
         if ( NumberInt != e.type() || e.Int() < 0 )
         {
            PD_LOG( PDERROR, "invalid arguments:%s",
                    args.toString( FALSE, TRUE ).c_str() ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
      }

   done:
      return rc ;
   error:
      goto done ; 
   }

   INT32 rtnDropIDIndex( const CHAR *name,
                         const bson::BSONObj &pubArgs,
                         const bson::BSONObj &args,
                         _pmdEDUCB *cb,
                         _dpsLogWrapper *dpsCB )
   {
      INT32 rc = SDB_OK ;
      BSONObj obj = BSON( IXM_FIELD_NAME_NAME << IXM_ID_KEY_NAME ) ;
      BSONElement e ;
      rc = rtnDropIndexCommand( name, obj.firstElement(),
                                cb, sdbGetDMSCB(),
                                dpsCB, TRUE ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to drop id index on cl:%s, rc:%d",
                 name, rc ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }
}

