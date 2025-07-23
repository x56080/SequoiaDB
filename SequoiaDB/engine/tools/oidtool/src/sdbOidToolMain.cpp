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

   Source File Name = sdbOidToolMain.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who      Description
   ====== =========== ======== ==============================================
          2022/11/04  TZB      Initial Draft

   Last Changed =

*******************************************************************************/
#include "utilCommon.hpp"
#include "sdbOidToolUtil.hpp"
#include "sdbDataChecker.hpp"
#include "sdbDataReloader.hpp"

/**
 * SdbOidTool
 */
class SdbOidTool
{
public:
   SdbOidTool() ;
   ~SdbOidTool() ;

   INT32 init() ;
   INT32 process( BSONObj &result ) ;

private:
   INT32 _processOidCheck( BSONObj &result ) ;
   INT32 _processDataReload( BSONObj &result ) ;

private:
   InfoOptions*   _pOptions ;
} ;

/**
 * Implement of SdbOidTool
 */
SdbOidTool::SdbOidTool()
{
   _pOptions = getInfoOptions() ;
}

SdbOidTool::~SdbOidTool()
{
}

INT32 SdbOidTool::init()
{
   // enable dialog
   sdbEnablePD( _pOptions->logpath.c_str() ) ;

   // set debug level
   if ( _pOptions->isDebug )
   {
      setPDLevel( PDDEBUG ) ;
   }
   else
   {
      setPDLevel( PDINFO ) ;
   }

   return SDB_OK ;
}

INT32 SdbOidTool::process( BSONObj &result )
{
   INT32 rc = SDB_OK ;
   string action = _pOptions->action ;
   if ( "check" == action )
   {
      rc = _processOidCheck( result ) ;
   }
   else if ( "repair" == action )
   {
      rc = _processDataReload( result ) ;
   }
   else
   {
      cerr << "Error: Invalid --action has been specified: "
           << action.c_str() << endl ;
      rc = SDB_INVALIDARG ;
   }
   return rc ;
}

INT32 SdbOidTool::_processOidCheck( BSONObj &result )
{
   INT32 rc = SDB_OK ;
   SdbOidChecker oidChecker ;

   rc = oidChecker.init() ;
   if ( rc )
   {
      PD_LOG( PDERROR, "Failed to init oid checker for cl[%s], rc=%d",
              _pOptions->srcCLFullName.c_str(), rc ) ;
      goto error ;
   }

   rc = oidChecker.run( result ) ;
   if ( rc )
   {
      PD_LOG( PDERROR, "Failed to run oid checker for cl[%s], rc=%d",
              _pOptions->srcCLFullName.c_str(), rc ) ;
      goto error ;
   }
done:
   return rc ;
error:
   goto done ;
}

INT32 SdbOidTool::_processDataReload( BSONObj &result )
{
   INT32 rc = SDB_OK ;
   SdbDataReloader dataReloader ;

   rc = dataReloader.run( result ) ;
   if ( rc )
   {
      PD_LOG( PDERROR, "Failed to run data reloader for cl[%s], rc=%d",
              _pOptions->srcCLFullName.c_str(), rc ) ;
      goto error ;
   }

done:
   return rc ;
error:
   goto done ;
}

INT32 _checkAndPrintResult( const BSONObj &result )
{
   BSONObjIterator it( result ) ;
   BOOLEAN hasErrNo = FALSE ;

   while ( it.more() )
   {
      BSONElement ele = it.next() ;
      if ( 0 == ossStrcmp( ele.fieldName(), "ErrNo" ) )
      {
         hasErrNo = TRUE ;
         break ;
      }
   }

   // check and print
   if ( !hasErrNo ) {
      return SDB_SYS ;
   }
   else
   {
      // display info for js to catch
      cout << result.toString( FALSE, TRUE, FALSE ) << endl ;
      return SDB_OK ;
   }
}

INT32 main( INT32 argc, CHAR **argv )
{
   INT32 rc = SDB_OK ;
   InfoOptions *pOptions = getInfoOptions() ;
   SdbOidTool tool ;
   BSONObj result ;

   rc = pOptions->parse( argc, argv ) ;
   if ( rc )
   {
      goto error ;
   }

   rc = tool.init() ;
   if ( rc )
   {
      goto error ;
   }

   rc = tool.process( result ) ;
   if ( rc )
   {
      goto error ;
   }

   rc = _checkAndPrintResult( result ) ;
   if ( rc )
   {
      goto error ;
   }
done:
   return engine::utilRC2ShellRC( rc ) ;
error:
   goto done ;
}