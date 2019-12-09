/******************************************************************************
@Description: seqDB-7500:对多个子表做切分后创建索引，删除跟索引字段匹配的记录
@modify list:
   2014-7-30   pusheng Ding  Init
   2019-4-15   xiaoni huang  modify
*******************************************************************************/

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

   var mclName = "mcl_7500";
   var sclName = "scl_7500";
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

   var sOpt = { ShardingKey: { a: 1 }, ShardingType: "hash", Group: srcRG };
   var subCL = commCreateCLByOption( db, COMMCSNAME, sclName, sOpt, true, true );

   mainCL.attachCL( COMMCSNAME + "." + sclName, { LowBound: { a: 0 }, UpBound: { a: 100 } } );

   // insert
   println( "\n---Begin to insert." );
   var recordsNum = 100;
   var docs = [];
   for( var i = 0; i < recordsNum; ++i )
   {
      docs.push( { a: i } );
   }
   mainCL.insert( docs );

   // split and create index
   println( "\n---Begin to create split and index." );
   subCL.split( srcRG, trgRG, 50 );
   mainCL.createIndex( "idx", { b: 1 } );

   // CRUD
   println( "\n---Begin to exec CRUD." );
   // insert
   var docs = [];
   for( var i = 0; i < recordsNum; ++i )
   {
      docs.push( { a: i, b: i } );
   }
   mainCL.insert( docs );
   var cnt = mainCL.count( { b: { $exists: 1 } } );
   if( Number( cnt ) !== recordsNum ) 
   {
      throw buildException( "main", null, "check insert", recordsNum, cnt );
   }

   // remove
   mainCL.remove( { $and: [{ b: { $lt: 50 } }, { b: { $exists: 1 } }] } );
   subCL.remove( { $and: [{ b: { $gte: 50 } }, { b: { $exists: 1 } }] } );
   var cnt = mainCL.count( { b: { $exists: 1 } } );
   if( Number( cnt ) !== 0 ) 
   {
      throw buildException( "main", null, "check remove", 0, cnt );
   }

   // count
   var cnt = mainCL.count();
   if( Number( cnt ) !== recordsNum ) 
   {
      throw buildException( "main", null, "check total count", recordsNum, cnt );
   }

   // clear env
   commDropCL( db, COMMCSNAME, mclName, true, false, "drop mcl in the end" );
}
