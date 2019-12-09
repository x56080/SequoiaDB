/******************************************************************************
@Description: seqDB-10439:数据落在不同组上的相同子表中，批量插入数据
@modify list:
   2016.11.23  zengxianquan  Init
   2019-4-15   xiaoni huang  modify
******************************************************************************/

main();
function main ()
{
   if( true == commIsStandalone( db ) )
   {
      println( "---Is standalone." );
      return;
   }
   if( commGetGroupsNum( db ) < 2 )
   {
      println( "---Least two groups" );
      return;
   }
   db.setSessionAttr( { PreferedInstance: "M" } );

   var mclName = "mcl_10439";
   var sclName = "scl_10439";
   var groups = commGetGroups( db, false, "", false, true, true );
   var srcRG = groups[1][0].GroupName;
   var trgRG = groups[2][0].GroupName;

   // clear env
   commDropCL( db, COMMCSNAME, mclName, true, true, "drop mcl in the begin" );
   commDropCL( db, COMMCSNAME, sclName, true, true, "drop scl in the begin" );

   // create cs and cl, attach cl
   println( "\n---Begin to create cl, and attach cl." );
   var mclOpt = { "ShardingKey": { a: 1 }, "IsMainCL": true };
   var mainCL = commCreateCLByOption( db, COMMCSNAME, mclName, mclOpt, true, false );

   var sOpt = { ShardingKey: { a: 1 }, ShardingType: "range", Group: srcRG };
   var subCL = commCreateCLByOption( db, COMMCSNAME, sclName, sOpt, true, true );

   mainCL.attachCL( COMMCSNAME + "." + sclName, { LowBound: { a: 0 }, UpBound: { a: 100 } } );

   // split and create index
   println( "\n---Begin to create split." );
   subCL.split( srcRG, trgRG, { a: 50 }, { a: 100 } );

   // insert
   println( "\n---Begin to insert." );
   var recordsNum = 100;
   var docs = [];
   for( var i = 0; i < recordsNum; ++i )
   {
      docs.push( { a: i } );
   }
   mainCL.insert( docs );

   // check total count
   println( "\n---Begin to check total count." );
   var totalCnt = mainCL.count();
   if( Number( totalCnt ) !== recordsNum ) 
   {
      throw buildException( "main", null, "check total count", recordsNum, totalCnt );
   }

   // check srcRG count
   println( "\n---Begin to check results in srcRG." );
   var rg1 = db.getRG( srcRG ).getMaster().connect();
   var expRG1Cnt = 50;
   var actRG1Cnt = rg1.getCS( COMMCSNAME ).getCL( sclName ).count();
   if( Number( actRG1Cnt ) !== expRG1Cnt ) 
   {
      throw buildException( "main()", null, "check srgRG results", expRG1Cnt, actRG1Cnt );
   }

   // check trgRG count
   println( "\n---Begin to check results in trgRG." );
   var rg2 = db.getRG( trgRG ).getMaster().connect();
   var expRG2Cnt = 50;
   var actRG2Cnt = rg2.getCS( COMMCSNAME ).getCL( sclName ).count();
   if( Number( actRG2Cnt ) !== expRG2Cnt ) 
   {
      throw buildException( "main", null, "check trgRG results", expRG2Cnt, actRG2Cnt );
   }

   // clear env
   commDropCL( db, COMMCSNAME, mclName, true, false, "drop mcl in the end" );
}












