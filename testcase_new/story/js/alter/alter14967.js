/************************************
*@Description: 重复开启关闭压缩
*@author:      luweikang
*@createdate:  2018.4.25
*@testlinkCase:seqDB-14967
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
   var clName = "cl14967";

   var options = { Compressed: false };
   var cl = commCreateCL( db, csName, clName, options, true, false, "create CL in the begin" );

   println( "---cl setAttributes CompressionType---" );
   cl.setAttributes( { CompressionType: 'snappy' } );
   checkSnapshot( db, SDB_SNAP_CATALOG, csName, clName, "CompressionType", 0 );
   checkSnapshot( db, SDB_SNAP_CATALOG, csName, clName, "CompressionTypeDesc", "snappy" );

   cl.setAttributes( { Compressed: false } );
   checkSnapshot( db, SDB_SNAP_CATALOG, csName, clName, "Attribute", 0 );
   checkSnapshot( db, SDB_SNAP_CATALOG, csName, clName, "AttributeDesc", "" );

   cl.setAttributes( { Compressed: true } );
   checkSnapshot( db, SDB_SNAP_CATALOG, csName, clName, "CompressionType", 1 );
   checkSnapshot( db, SDB_SNAP_CATALOG, csName, clName, "CompressionTypeDesc", "lzw" );

   commDropCL( db, csName, clName, true, false, "clean cl" );
   println( "---end the test---" );
}

