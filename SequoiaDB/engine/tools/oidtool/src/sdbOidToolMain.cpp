/*******************************************************************************

   Copyright (C) 2011-2022 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU Affero General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

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