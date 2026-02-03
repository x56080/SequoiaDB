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

   Source File Name = sqlCB.cpp

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
#include "sqlCB.hpp"
#include "pmd.hpp"
#include "pmdCB.hpp"
#include "qgmPlanContainer.hpp"
#include "qgmBuilder.hpp"
#include "utilStr.hpp"
#include "optQgmOptimizer.hpp"
#include "rtnSQLFunc.hpp"
#include "rtnSQLFuncFactory.hpp"
#include "rtnContextQGM.hpp"

namespace engine
{
   _sqlCB::_sqlCB()
   {

   }

   _sqlCB::~_sqlCB()
   {

   }

   INT32 _sqlCB::init ()
   {
      return SDB_OK ;
   }

   INT32 _sqlCB::active ()
   {
      return SDB_OK ;
   }

   INT32 _sqlCB::deactive ()
   {
      return SDB_OK ;
   }

   INT32 _sqlCB::fini ()
   {
      return SDB_OK ;
   }

   INT32 _sqlCB::exec( const CHAR *sql, _pmdEDUCB *cb,
                       SINT64 &contextID,
                       BOOLEAN &needRollback,
                       BSONObjBuilder *pBuilder )
   {
      SDB_ASSERT( NULL != sql, "impossible" ) ;
      INT32 rc = SDB_OK ;
      qgmPlanContainer *container = NULL ;
      BOOLEAN containerOwnned = TRUE ;
      qgmOptiTreeNode *opti = NULL ;
      qgmOptiTreeNode *extend = NULL ;
      const CHAR *trimedSql = NULL ;

#ifdef _DEBUG
      PD_LOG( PDDEBUG, "sql[%s]", sql ) ;
#endif

      rc = utilStrTrim( (CHAR *)sql, trimedSql ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to trim sql, rc: %d", rc ) ;
         goto error ;
      }

      container = SDB_OSS_NEW qgmPlanContainer() ;
      if ( NULL == container )
      {
         PD_LOG( PDERROR, "failed to allocate mem." ) ;
         rc = SDB_OOM ;
         goto error ;
      }

      /// step 1: ast parse
      container->ast() = SQL_PARSE( trimedSql, _grammar ) ;
      if ( !container->ast().match
           || !container->ast().full )
      {
         PD_LOG( PDERROR, "syntax error [%s]", container->ast().stop ) ;
         rc = SDB_SQL_SYNTAX_ERROR ;
         goto error ;
      }

      {
      /// step 2: build opti tree
      qgmBuilder builder( container->ptrTable(),
                          container->paramTable()) ;
      rc = builder.build( container->ast().trees, opti ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to build qgm tree:%d", rc ) ;
         goto error ;
      }

      /// step 3: extend
      rc = opti->extend( extend ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to extend qgm tree:%d", rc ) ;
         goto error ;
      }

      /// step 4: optimize
      {
      _qgmOptTree tree( extend ) ;
      _optQgmOptimizer optimizer ;
      rc = optimizer.adjust( tree ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to rewrite sql:%d", rc ) ;
         goto error ;
      }

      extend = tree.getRoot() ;
      }

      rc = extend->checkPrivileges( cb->getSession() );
      if ( SDB_NO_PRIVILEGES == rc )
      {
         PD_LOG( PDERROR, "No privileges to execute this query" ) ;
         goto error ;
      }
      else if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to check privileges, rc: %d", rc );
      }

      /// step 5:build physical plan.
      rc = builder.build( extend, container->plan() ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to build phy tree:%d", rc ) ;
         goto error ;
      }

      SDB_ASSERT( QGM_PLAN_TYPE_MAX != container->type(),
                  "impossible" ) ;

      /// step 6: if it is a query. create context.
      if ( QGM_PLAN_TYPE_RETURN == container->type() )
      {
         rc = _createContext( container, cb, contextID ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to create context:%d", rc ) ;
            goto error ;
         }
         containerOwnned = FALSE ;
      }

      /// set names
      if ( cb->getMonQueryCB() )
      {
         ossPoolSet< ossPoolString > setObjs ;
         container->getObjects( setObjs ) ;
         MONQUERY_SET_NAMES( cb, setObjs ) ;         
      }

      /// step 7: execute.
      rc = container->execute( cb ) ;
      needRollback = container->needRollback() ;
      if ( pBuilder )
      {
         container->buildRetInfo( *pBuilder ) ;
      }
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to execute qgm tree, rc: %d", rc ) ;
         goto error ;
      }

      /// step 8: set cur trans context id
      if ( -1 != contextID && cb->isAutoCommitTrans() )
      {
         cb->setCurAutoTransCtxID( contextID ) ;
      }

      }
   done:
      /// if extended, we noly need release extended root.
      if ( NULL != extend )
      {
         SDB_OSS_DEL extend ;
      }
      else
      {
         SAFE_OSS_DELETE( opti ) ;
      }
      if ( container && containerOwnned )
      {
         SDB_OSS_DEL container ;
         container = NULL ;
      }
      return rc ;
   error:
      if ( -1 != contextID )
      {
         pmdGetKRCB()->getRTNCB()->contextDelete( contextID, cb ) ;
         contextID = -1 ;
      }
      goto done ;
   }

   INT32 _sqlCB::getFunc( const CHAR *name,
                          UINT32 paramNum,
                          _rtnSQLFunc *&func )
   {
      _rtnSQLFuncFactory factory ;
      return factory.create( name, paramNum, func ) ;
   }

   INT32 _sqlCB::_createContext( _qgmPlanContainer *container,
                                 _pmdEDUCB *cb, SINT64 &contextID )
   {
      INT32 rc = SDB_OK ;

      rtnContextQGM::sharePtr context ;
      SDB_RTNCB *rtnCB = pmdGetKRCB()->getRTNCB() ;
      rc = rtnCB->contextNew ( RTN_CONTEXT_QGM, context,
                               contextID, cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to create new context, rc: %d", rc ) ;
      // open
      rc = context->open( container ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to open context[%lld], rc: %d",
                   context->contextID(), rc ) ;

   done:
      return rc ;
   error:
      rtnCB->contextDelete ( contextID, cb ) ;
      contextID = -1 ;
      goto done ;
   }

   /*
      get global sql cb
   */
   SQL_CB* sdbGetSQLCB ()
   {
      static SQL_CB s_sqlCB ;
      return &s_sqlCB ;
   }

}

