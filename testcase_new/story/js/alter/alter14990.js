/************************************
*@Description: enableSharding批量修改属性
*@author:      luweikang
*@createdate:  2018.4.25
*@testlinkCase:seqDB-14990
**************************************/

main();

function main ()
{
   if( commIsStandalone( db ) )
   {
      println( "Run mode is standalone" );
      return;
   }

   println( "---begin test---" );
   var csName = COMMCSNAME;
   var clName = CHANGEDPREFIX + "_14990";

   var options = { ShardingType: 'hash', ShardingKey: { a: 1 }, Compressed: true };
   var cl = commCreateCLByOption( db, csName, clName, options, true, false, "create CL in the begin" );


   //这个地方写测试步骤
   println( "---test enableSharding 1---" );
   cl.enableSharding( { ShardingKey: { b: 1 }, ShardingType: "range", EnsureShardingIndex: true } );
   checkSnapshot( db, SDB_SNAP_CATALOG, csName, clName, "ShardingKey", { b: 1 } );
   checkSnapshot( db, SDB_SNAP_CATALOG, csName, clName, "ShardingType", "range" );
   checkSnapshot( db, SDB_SNAP_CATALOG, csName, clName, "EnsureShardingIndex", true );

   println( "---test enableSharding 2---" );
   cl.enableSharding( { Partition: 2048, ShardingType: "hash", ShardingKey: { a: 1 }, EnsureShardingIndex: false } );
   checkSnapshot( db, SDB_SNAP_CATALOG, csName, clName, "Partition", 2048 );
   checkSnapshot( db, SDB_SNAP_CATALOG, csName, clName, "ShardingType", "hash" );
   checkSnapshot( db, SDB_SNAP_CATALOG, csName, clName, "ShardingKey", { a: 1 } );
   checkSnapshot( db, SDB_SNAP_CATALOG, csName, clName, "EnsureShardingIndex", false );

   println( "---test enableSharding 3---" );
   try
   {
      cl.enableSharding( { ShardingKey: { b: 1 }, ShardingType: "range", Compressed: false } );
   }
   catch( e )
   {
      if( e !== -6 )
      {
         throw buildException( "test enableSharding", e, "value is wrong", -6, e );
      }
   }
   checkSnapshot( db, SDB_SNAP_CATALOG, csName, clName, "ShardingType", "hash" );
   checkSnapshot( db, SDB_SNAP_CATALOG, csName, clName, "ShardingKey", { a: 1 } );
   checkSnapshot( db, SDB_SNAP_CATALOG, csName, clName, "AttributeDesc", "Compressed" );


   commDropCL( db, csName, clName, true, false, "clean cl" );
   println( "---end the test---" );
}
