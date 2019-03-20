/******************************************************************************
*@Description : seqDB-447:新建分区组进行数据切分 
*@Author      : 2019-3-19  XiaoNi Huang
******************************************************************************/
main();

function main()
{  
   if( commIsStandalone(db) )
   {
      println(" Deploy mode is standalone!");
      return;
   }
   if( commGetGroupsNum(db) < 2 )
   {  
      println("This testcase needs at least 2 groups to split sub cl!");
      return;
   }   
   
   var tmpGroupName = "rg_447";
   var groupNum = 1;
   var nodeNum = 2;
   var newGroupNames = [];
   var clName = "cl_447";
   var recordsNum = 10000;
   
   // clean env
   commDropCL( db, COMMCSNAME, clName, true, true, 
            "Failed to drop CL in the pre-condition." ); 
   removeDataGroups( [tmpGroupName + "_0"], true );  //createDataGroups, tmpGroupName+"_"+i
   
   println("\n---Begin to createCL and insert records");
   var options = {ShardingKey:{a:1}, ShardingType:"range", ReplSize:0};
   var cl = commCreateCLByOption(db, COMMCSNAME, clName, options, true, true);
   var srcRg = commGetCLGroups( db, COMMCSNAME + "." + clName )[0];

   var docs = readyDocs( recordsNum );
   cl.insert( docs );
   checkRecords( cl, docs );
   
   println("\n---Begin to createRG");
   newGroupNames = createDataGroups( COORDHOSTNAME , groupNum, tmpGroupName, nodeNum );
   var targetRg = newGroupNames[0];
   
   println("\n---Begin to split and check results");
   cl.split( srcRg, targetRg, { a: recordsNum/2 }, { a: recordsNum } );
   checkRecords( cl, docs );
   checkRgRecords( srcRg, targetRg, clName, docs );
   
   // clean env
   cleanCL( clName ); 
   removeDataGroups( newGroupNames, false );
}

function readyDocs( recordsNum )
{
   var doc = [];
   for( var i = 0; i < recordsNum; i++ )
   {
      doc.push( {a:i, b:"test" + i, c:"hello" } );
   }   
   return doc;
}

function checkRecords( cl, expData ) 
{
   var rc = cl.find( {}, {_id:{$include:0}} ).sort({a:1} );
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

function checkRgRecords( srcRg, targetRg, clName, docs )
{  
   var docsNum = docs.length;
   // srcRg
   println("   Begin to checkRgRecords[" + srcRg +"]");
   var nodeDB = db.getRG( srcRg ).getMaster().connect();
   var cl = nodeDB.getCS( COMMCSNAME ).getCL( clName );
   checkRecords( cl, docs.slice( 0, (docsNum/2) ) );
   nodeDB.close();
   
   // targetRg
   println("   Begin to checkRgRecords[" + targetRg +"]");
   var nodeDB = db.getRG( targetRg ).getMaster().connect();
   var cl = nodeDB.getCS( COMMCSNAME ).getCL( clName );
   checkRecords( cl, docs.slice( docsNum/2, docsNum ) );
   nodeDB.close();
}