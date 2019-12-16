/************************************
*@Description: 普通表修改固定集合属性
*@author:      luweikang
*@createdate:  2018.4.25
*@testlinkCase:seqDB-14985
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
   var clName = CHANGEDPREFIX + "_14985";

   var cl = commCreateCL( db, csName, clName, {}, true, false, "create CL in the begin" );

   //alter cl attribute
   println( "---test alter Size---" );
   clSetAttributes( cl, { Size: 32 } );
   checkSnapshot( db, SDB_SNAP_CATALOG, csName, clName, "Size", undefined );

   println( "---test alter Max---" );
   clSetAttributes( cl, { Max: 1000 } );
   checkSnapshot( db, SDB_SNAP_CATALOG, csName, clName, "Max", undefined );

   println( "---test alter OverWrite---" );
   clSetAttributes( cl, { OverWrite: true } );
   checkSnapshot( db, SDB_SNAP_CATALOG, csName, clName, "OverWrite", undefined );

   commDropCL( db, csName, clName, true, false, "clean cl" );
   println( "---end the test---" );
}
