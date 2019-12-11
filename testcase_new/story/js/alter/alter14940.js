/************************************
*@Description: 切分表修改ShardingKey字段, ShardingType, Partition, EnsureShardingIndex
*@author:      luweikang
*@createdate:  2018.4.25
*@testlinkCase:seqDB-14938, seqDB-14950, seqDB-14957, seqDB-14978
**************************************/

try
{
   main();
}
catch( e )
{
   if( e.constructor === Error )
   {
      println( e.stack );
   }
   throw e;
}


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
   var clName = CHANGEDPREFIX + "_14940";

   var options = { ShardingType: 'hash', ShardingKey: { a: 1 } };
   var cl = commCreateCLByOption( db, csName, clName, options, true, false, "create CL in the begin" );

   for( i = 0; i < 5000; i++ )
   {
      cl.insert( { a: i, b: "sequoiadh test split cl alter option" } );
   }

   println( "---split cl---" );
   var splitGroup = getSplitGroup( db, csName, clName );
   cl.split( splitGroup.srcGroup, splitGroup.tarGroup, 50 );

   //test split cl alter ShardingKey
   println( "---test alter ShardingKey---" );
   clSetAttributes( cl, { ShardingKey: { 'b': 1 } } );
   checkSnapshot( db, SDB_SNAP_CATALOG, csName, clName, "ShardingKey", { a: 1 } );

   println( "---test alter ShardingType---" );
   clSetAttributes( cl, { ShardingType: 'range' } );
   checkSnapshot( db, SDB_SNAP_CATALOG, csName, clName, "ShardingType", 'hash' );

   println( "---test alter Partition---" );
   clSetAttributes( cl, { Partition: 8192 } );
   checkSnapshot( db, SDB_SNAP_CATALOG, csName, clName, "Partition", 4096 );

   println( "---test alter EnsureShardingIndex---" );
   clSetAttributes( cl, { EnsureShardingIndex: false } );
   checkSnapshot( db, SDB_SNAP_CATALOG, csName, clName, "EnsureShardingIndex", true );

   commDropCL( db, csName, clName, true, false, "clean cl" );
   println( "---end the test---" );
}

