/************************************
*@Description: 开启/关闭修改分区属性
*@author:      luweikang
*@createdate:  2018.4.25
*@testlinkCase:seqDB-15239
**************************************/

main();

function main ()
{
   if( commIsStandalone( db ) )
   {
      println( "Run mode is standalone" );
      return;
   }
   //less two groups no split
   var allGroupName = getGroupName( db, true );
   if( 1 === allGroupName.length )
   {
      println( "--least two groups" );
      return;
   }
   println( "---begin test---" );
   var csName = COMMCSNAME;
   var clName = CHANGEDPREFIX + "_15239";

   var cl = commCreateCL( db, csName, clName, 1, false, true, false, "create CL in the begin" );

   for( i = 0; i < 5000; i++ )
   {
      cl.insert( { a: i, b: "sequoiadh test split cl alter option" } );
   }

   cl.enableSharding( { ShardingKey: { a: 1 }, ShardingType: 'range', AutoSplit: false, EnsureShardingIndex: false } );
   checkSnapshot( db, SDB_SNAP_CATALOG, csName, clName, "ShardingKey", { a: 1 } );
   checkSnapshot( db, SDB_SNAP_CATALOG, csName, clName, "ShardingType", "range" );
   checkSnapshot( db, SDB_SNAP_CATALOG, csName, clName, "AutoSplit", false );
   checkSnapshot( db, SDB_SNAP_CATALOG, csName, clName, "EnsureShardingIndex", false );

   cl.enableSharding( { ShardingKey: { b: 1 }, ShardingType: 'hash', Partition: 2048, AutoSplit: true, EnsureShardingIndex: true } );
   checkSnapshot( db, SDB_SNAP_CATALOG, csName, clName, "ShardingKey", { b: 1 } );
   checkSnapshot( db, SDB_SNAP_CATALOG, csName, clName, "ShardingType", "hash" );
   checkSnapshot( db, SDB_SNAP_CATALOG, csName, clName, "Partition", 2048 );
   checkSnapshot( db, SDB_SNAP_CATALOG, csName, clName, "AutoSplit", true );
   checkSnapshot( db, SDB_SNAP_CATALOG, csName, clName, "EnsureShardingIndex", true );

   try
   {
      cl.disableSharding();
      throw "DISABLE_ERR";
   }
   catch( e )
   {
      if( e !== -32 )
      {
         throw buildException( "disableSharding", e, "disable sharding", -32, e );
      }
   }

   commDropCL( db, csName, clName, true, false, "clean cl" );
   println( "---end the test---" );
}
