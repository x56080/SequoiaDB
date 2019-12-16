/************************************
*@Description: alter修改压缩属性
*@author:      luweikang
*@createdate:  2018.4.25
*@testlinkCase:seqDB-15033
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
   println( "---begin test---" );
   var csName = COMMCSNAME;
   var clName1 = CHANGEDPREFIX + "_15033_1";
   var clName2 = CHANGEDPREFIX + "_15033_2";

   var options = { ShardingType: 'hash', ShardingKey: { a: 1 }, Compressed: false };
   var cl1 = commCreateCL( db, csName, clName1, options, true, false, "create CL in the begin" );
   var cl2 = commCreateCL( db, csName, clName2, {}, true, false, "create CL in the begin" );
   for( i = 0; i < 5000; i++ )
   {
      cl1.insert( { a: i, b: "sequoiadh test split cl alter option" } );
      cl2.insert( { a: i, b: "sequoiadh test split cl alter option" } );
   }

   //lzw类型为1, snappy为0
   println( "---test alter ShardingKey---" );
   cl1.setAttributes( { Compressed: true, CompressionType: 'lzw' } );
   checkSnapshot( db, SDB_SNAP_CATALOG, csName, clName1, "AttributeDesc", "Compressed" );
   checkSnapshot( db, SDB_SNAP_CATALOG, csName, clName1, "CompressionType", 1 );

   cl2.setAttributes( { Compressed: true } );
   checkSnapshot( db, SDB_SNAP_CATALOG, csName, clName2, "AttributeDesc", "Compressed" );

   cl2.setAttributes( { CompressionType: 'lzw' } );
   checkSnapshot( db, SDB_SNAP_CATALOG, csName, clName2, "CompressionType", 1 );

   commDropCL( db, csName, clName1, true, false, "clean cl1" );
   commDropCL( db, csName, clName2, true, false, "clean cl2" );
   println( "---end the test---" );
}
