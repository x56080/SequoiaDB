/*******************************************************************
* @Description : test resetSnapshot() option function
*                seqDB-14445:Type字段测试
*                seqDB-14446:SessionID字段测试
*                seqDB-14447:命令位置参数测试
*                seqDB-15383:ResetTimestamp字段测试
* @author      : linsuqiang
* 
*******************************************************************/
// 本用例实现是文本用例的简化，因为功能测试是手工覆盖的，
// 本自动化只是驱动接口测试

main( db );

function main ( db )
{
   if( commIsStandalone( db ) )
   {
      println( "Run mode is standalone" );
      return;
   }

   var csName = "resetSnapshot14445";
   var clName = "resetSnapshot14445";
   var cl = commCreateCL( db, csName, clName, /*option*/{},
         /*autoCreateCS*/true, /*ignoreExisted*/true );
   insertData( cl );

   var clFullName = csName + "." + clName;
   var rgName = commGetCLGroups( db, clFullName )[0];
   var dataDB = db.getRG( rgName ).getMaster().connect();
   var dbTime1 = getResetSnapTime( dataDB, SDB_SNAP_DATABASE );
   var ssTime1 = getResetSnapTime( dataDB, SDB_SNAP_SESSIONS );

   var notExistID = 999999; // is nearly impossible to be such many sessions
   createStatisInfo( dataDB, csName, clName );
   db.resetSnapshot( { Type: "sessions", SessionID: notExistID } );
   assert( !isSessionSnapClean( dataDB ), "session snap shouldn't be reset(notExistID)" );
   assert( !isDatabaseSnapClean( dataDB ), "database snap shouldn't be reset(notExistID)" );

   var currID = getCurrentID( dataDB );
   createStatisInfo( dataDB, csName, clName );
   db.resetSnapshot( { Type: "sessions", SessionID: currID } );
   assert( isSessionSnapClean( dataDB ), "session snap should be reset(validID)" );
   assert( !isDatabaseSnapClean( dataDB ), "database snap shouldn't be reset(validID)" );
   var dbTime2 = getResetSnapTime( dataDB, SDB_SNAP_DATABASE );
   var ssTime2 = getResetSnapTime( dataDB, SDB_SNAP_SESSIONS_CURRENT );
   assert( dbTime1 === dbTime2 );
   assert( ssTime1 < ssTime2 );

   createStatisInfo( dataDB, csName, clName );
   db.resetSnapshot( { Type: "database", NodeSelect: "secondary" } );
   assert( !isSessionSnapClean( dataDB ), "session snap shouldn't be reset(notSelected)" );
   assert( !isDatabaseSnapClean( dataDB ), "database snap shouldn't be reset(notSelected)" );

   createStatisInfo( dataDB, csName, clName );
   db.resetSnapshot( { Type: "database", NodeSelect: "primary" } );
   assert( !isSessionSnapClean( dataDB ), "session snap shouldn't be reset(selected)" );
   assert( isDatabaseSnapClean( dataDB ), "database snap should be reset(selected)" );
   var dbTime3 = getResetSnapTime( dataDB, SDB_SNAP_DATABASE );
   var ssTime3 = getResetSnapTime( dataDB, SDB_SNAP_SESSIONS_CURRENT );
   assert( dbTime2 < dbTime3 );
   assert( ssTime2 === ssTime3 );

   dataDB.close();
   commDropCS( db, csName );
}

function insertData ( cl )
{
   var docNum = 100;
   var docs = [];
   for( var i = 0; i < docNum; ++i )
   {
      docs.push( { a: 1 } );
   }
   cl.insert( docs );
}

function createStatisInfo ( dataDB, csName, clName )
{
   var cl = dataDB.getCS( csName ).getCL( clName );
   var cur = cl.find();
   while( cur.next() ) { }
   cur.close();
}

// if another one also directly connects data node, 
// this judge may be not excat. 
function isSessionSnapClean ( dataDB )
{
   var cur = dataDB.snapshot( SDB_SNAP_SESSIONS, { "Type": "Agent" } );
   var totalRead = cur.next().toObj().TotalRead;
   cur.close();
   return ( totalRead == 0 );
}

function isDatabaseSnapClean ( dataDB )
{
   var cur = dataDB.snapshot( SDB_SNAP_DATABASE );
   var totalRead = cur.next().toObj().TotalRead;
   cur.close();
   return ( totalRead == 0 );
}

function getCurrentID ( dataDB )
{
   var cur = dataDB.snapshot( SDB_SNAP_SESSIONS_CURRENT );
   var sessionID = cur.next().toObj().SessionID;
   cur.close();
   return sessionID;
}

function getResetSnapTime ( dataDB, snapType )
{
   var cur = dataDB.snapshot( snapType );
   var time = cur.next().toObj().ResetTimestamp;
   cur.close();
   return time;
}
