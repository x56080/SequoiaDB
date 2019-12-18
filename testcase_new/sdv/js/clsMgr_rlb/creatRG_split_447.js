/******************************************************************************
*@Description : seqDB-447:新建分区组进行数据切分 
*@Author      : 2019-3-19  XiaoNi Huang
******************************************************************************/
main();

function main ()
{
   if( commIsStandalone( db ) )
   {
      println( " Deploy mode is standalone!" );
      return;
   }
   if( commGetGroupsNum( db ) < 2 )
   {
      println( "This testcase needs at least 2 groups to split sub cl!" );
      return;
   }

   var tmpGroupName = "rg_447";
   var groupNum = 1;
   var nodeNum = 2;
   var logSourcePaths = [];
   var clName = "cl_447";
   var recordsNum = 10000;

   // clean env
   commDropCL( db, COMMCSNAME, clName, true, true, "Failed to drop CL in the pre-condition." );
   removeDataGroup( tmpGroupName, true );
   var coordGroup = commGetGroups( db, true, "", true, false, true );
   var hostname = coordGroup[0][1].HostName;

   println( "\n---Begin to createCL and insert records" );
   var options = { ShardingKey: { a: 1 }, ShardingType: "range", ReplSize: 0 };
   var cl = commCreateCL( db, COMMCSNAME, clName, options, true, true );
   var srcRg = commGetCLGroups( db, COMMCSNAME + "." + clName )[0];

   var docs = readyDocs( recordsNum );
   cl.insert( docs );
   checkRecords( cl, docs );

   try
   {
      println( "\n---Begin to createRG" );
      logSourcePaths = createDataGroups( hostname, tmpGroupName, nodeNum );
      var targetRg = tmpGroupName;

      println( "\n---Begin to split and check results" );
      cl.split( srcRg, targetRg, { a: recordsNum / 2 }, { a: recordsNum } );
      checkRecords( cl, docs );
      checkRgRecords( srcRg, targetRg, clName, docs );
   }
   catch( e )
   {
      println( "catch e : " + e );
      //将新建组日志备份到/tmp/ci/rsrvnodelog目录下
      var backupDir = "/tmp/ci/rsrvnodelog/447";
      File.mkdir( backupDir );
      for( var i = 0; i < logSourcePaths.length; i++ )
      {
         File.scp( logSourcePaths[i], backupDir + "/sdbdiag" + i + ".log" );
      }
      throw e;
   }
   finally
   {
      // clean env
      commDropCL( db, COMMCSNAME, clName, false, false, "Failed to drop CL in the end-condition" );
      removeDataGroup( tmpGroupName, false );
   }
}

function readyDocs ( recordsNum )
{
   var doc = [];
   for( var i = 0; i < recordsNum; i++ )
   {
      doc.push( { a: i, b: "test" + i, c: "hello" } );
   }
   return doc;
}

function checkRecords ( cl, expData ) 
{
   var rc = cl.find( {}, { _id: { $include: 0 } } ).sort( { a: 1 } );
   var rcRecs = new Array();
   while( tmpRecs = rc.next() )
   {
      rcRecs.push( tmpRecs.toObj() );
   }

   var expRecs = JSON.stringify( expData );
   var actRecs = JSON.stringify( rcRecs );
   if( expRecs !== actRecs )
   {
      throw buildException( "checkResult", null, "", expRecs, "  " + actRecs );
   }
}

function checkRgRecords ( srcRg, targetRg, clName, docs )
{
   var docsNum = docs.length;
   // srcRg
   println( "   Begin to checkRgRecords[" + srcRg + "]" );
   var nodeDB = db.getRG( srcRg ).getMaster().connect();
   var cl = nodeDB.getCS( COMMCSNAME ).getCL( clName );
   checkRecords( cl, docs.slice( 0, ( docsNum / 2 ) ) );
   nodeDB.close();

   // targetRg
   println( "   Begin to checkRgRecords[" + targetRg + "]" );
   var nodeDB = db.getRG( targetRg ).getMaster().connect();
   var cl = nodeDB.getCS( COMMCSNAME ).getCL( clName );
   checkRecords( cl, docs.slice( docsNum / 2, docsNum ) );
   nodeDB.close();
}

function createDataGroups ( hostName, groupName, nodeNum )
{
   var dataGroupNames = [];
   var logSourcePaths = [];
   var rg = db.createRG( groupName );
   for( var j = 0; j < nodeNum; j++ )
   {
      var port = parseInt( RSRVPORTBEGIN ) + ( j * 10 );
      var dataPath = RSRVNODEDIR + "data/" + port;
      var checkSucc = false;
      var times = 0;
      var maxRetryTimes = 10;
      do
      {
         try
         {
            rg.createNode( hostName, port, dataPath, { diaglevel: 5 } );
            checkSucc = true;
            logSourcePaths.push( hostName + ":" + CMSVCNAME + "@" + dataPath + "/diaglog/sdbdiag.log" );
         }
         catch( e )
         {
            //-145 :SDBCM_NODE_EXISTED  -290:SDB_DIR_NOT_EMPTY
            if( e == -145 || e == -290 )
            {
               port = port + 10;
               dataPath = RSRVNODEDIR + "data/" + port;
            }
            else
            {
               throw "create node failed!  port = " + port + " dataPath = " + dataPath + " errorCode: " + e;
            }
            times++;
         }
      }
      while( !checkSucc && times < maxRetryTimes );
      rg.start();
   }
   return logSourcePaths;
}

function removeDataGroup ( dataGroupName, ignoreRGNotExist )
{
   try
   {
      db.removeRG( dataGroupName );
   }
   catch( e )
   {
      if( -154 == e && !ignoreRGNotExist )
      {
         throw e;
      }
   }
}