/* *****************************************************************************
@discretion: Clean after test-case
@modify list:
   2014-2-28 Jianhui Xu  Init
***************************************************************************** */

// RUNRESULT is input parameter
if( typeof ( RUNRESULT ) == "undefined" )
{
   RUNRESULT = 0;
}
var db = new Sdb( COORDHOSTNAME, COORDSVCNAME );

function main ( db )
{
   // 1. check nodes
   var groups = commGetGroups( db, "", "", false );
   var checkLSN = false;
   if( 0 != RUNRESULT )
   {
      checkLSN = true;
   }
   var errNodes = commCheckBusiness( groups, checkLSN );
   if( errNodes.length == 0 )
   {
   }
   else
   {
      println( "Groups or nodes has error after test-case: " );
      commPrint( errNodes );
   }

   if( 0 != RUNRESULT )
   {
      // not clean
      return;
   }

}

try
{
   main( db );
}
catch( e )
{
   println( "After test-case environment clear failed: " + e );
}
