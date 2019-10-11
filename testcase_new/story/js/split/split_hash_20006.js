/******************************************************************************
@Description : seqDB-20006:hash切分表百分百切分多次，多个组多次切分覆盖边界值
@Author :
   2019-10-11   XiaoNi Huang  init
*******************************************************************************/
main();

function main()
{
   if( true == commIsStandalone( db ) )
   {
      println( "---Is standalone." );
      return;
   } 
   
   if ( commGetGroupsNum( db ) < 3 )
   {
      println("---Least three groups.");
      return ;
   }
   
   var seqDB = "20006";
   var dmName = "dm" + seqDB;
   var csName = "cs" + seqDB;
   var clName = "cl";
   var dm;
   var cl;
   var recordsNum = 1000;
   
   var groups = commGetGroups(db, false, "", false, true, true );
   var groupNames = [ groups[1][0].GroupName, groups[2][0].GroupName, groups[3][0].GroupName ];
   
   commDropCS( db, csName, true, "drop cs in the begin");
	commDropDomain( db, dmName, true, "drop domain in the end.");
   
   // create domain / cs / cl
   println("\n---Begin to create domain & cs & cl.");
   dm = db.createDomain( dmName, [ groups[1][0].GroupName, groups[2][0].GroupName ] );
   var cs = db.createCS( csName, {"Domain": dmName} );
   cl = cs.createCL( clName,  {"ShardingKey":{"id":1}, "ShardingType":"hash", "AutoSplit":true} );
     
   // insert records
   println("\n---Begin to insert records.");
   var recs = [];
   for( var i = 0; i < 1000; ++i ) 
   {
      recs.push({"id": i, "name":"a" + i});
   }
   cl.insert( recs ); 
   
   // alter domain, add group
   println("\n---Begin to alter domain, add group.");
   dm.alter({"Groups": groupNames });
   
   // split
   println("\n---Begin to split, rgA -> rgC, 33%.");
   cl.split( groupNames[0], groupNames[2], 33); 
   
   println("\n---Begin to split, rgB -> rgC, 33%.");
   cl.split( groupNames[1], groupNames[2], 33); 
   
   println("\n---Begin to split, rgC -> rgA, 50%.");
   cl.split( groupNames[2], groupNames[0], 33); 
   
   // check results
   println("\n---Begin to check records.");
   checkRecordsNum( cl, csName, clName, recordsNum, groupNames );
   checkShardingRange( csName, clName );
   
   println("\n---Begin to drop domain & cs.");
   commDropCS( db, csName, false, "drop cs in the end.");
	commDropDomain( db, dmName, false, "drop domain in the end.");
}

function checkRecordsNum( cl, csName, clName, recordsNum, groupNames )
{
   println("   Begin to check total count." ) ;
   var cnt = Number( cl.count() );
   if( recordsNum !== cnt )
   {
      throw buildException("checkRecs", null, "[check total number]", recordsNum, cnt) ;
   }
   
   println("   Begin to check count for each group." ) ;
   var totalNodeRecsCnt = 0;
   for ( var i = 0; i < groupNames.length; i++ ) 
   {
      var nodeDB = null;
      try 
      {         
         var nodeDB = db.getRG( groupNames[ i ] ).getMaster().connect();
         var nodeRecsCnt = nodeDB.getCS( csName ).getCL( clName ).count();
         totalNodeRecsCnt += nodeRecsCnt;
      } 
      finally
      {
         if ( nodeDB !== null) nodeDB.close();
      }
   }
   if( recordsNum !== totalNodeRecsCnt )
   {
      throw buildException("checkRecs", null, "[check node records number]", recordsNum, totalNodeRecsCnt) ;
   }
}

function checkShardingRange( csName, clName ) 
{
   println("\n---Begin to check sharding range by snapshot(8,...)." ) ;
   var cataInfo = db.snapshot(8, {"Name": csName + "." + clName }).next().toObj().CataInfo;
   if( cataInfo[0]["LowBound"][""] !== 0    || cataInfo[0]["UpBound"][""] !== 1373 
    || cataInfo[1]["LowBound"][""] !== 1373 || cataInfo[1]["UpBound"][""] !== 2048 
    || cataInfo[2]["LowBound"][""] !== 2048 || cataInfo[2]["UpBound"][""] !== 3421 
    || cataInfo[3]["LowBound"][""] !== 3421 || cataInfo[3]["UpBound"][""] !== 3651
    || cataInfo[4]["LowBound"][""] !== 3651 || cataInfo[4]["UpBound"][""] !== 4096 )
   {
      throw "CataInfo error after split, actual cataInfo = [" + JSON.stringify( cataInfo ) + "]" ;
   } 
}