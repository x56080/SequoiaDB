/************************************
*@Description: 修改固定集合属性
*@author:      luweikang
*@createdate:  2018.4.25
*@testlinkCase:seqDB-14981, seqDB-14983, seqDB-14984
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
   var csName = COMMCSNAME + "_14981";
   var clName = CHANGEDPREFIX + "_14981";

   var csOption = { Capped: true };
   commCreateCS( db, csName, false, "", csOption );

   var clOption = { Capped: true, Size: 1024, Max: 100000, AutoIndexId: false, OverWrite: false };
   var cl = commCreateCLByOption( db, csName, clName, clOption, true, true );


   //alter capped cl attribute
   println( "---test alter Capped---" );
   clSetAttributes( cl, { Capped: false } );
   checkSnapshot( db, SDB_SNAP_CATALOG, csName, clName, "AttributeDesc", "NoIDIndex | Capped" );

   println( "---test alter Size---" );
   cl.setAttributes( { Size: 32 } );
   checkSnapshot( db, SDB_SNAP_CATALOG, csName, clName, "Size", 33554432 );

   println( "---test alter Max---" );
   cl.setAttributes( { Max: 1000 } );
   checkSnapshot( db, SDB_SNAP_CATALOG, csName, clName, "Max", 1000 );

   println( "---test alter OverWrite---" );
   cl.setAttributes( { OverWrite: true } );
   checkSnapshot( db, SDB_SNAP_CATALOG, csName, clName, "OverWrite", true );

   commDropCS( db, csName, true, "drop cl in the end" );
   println( "---end the test---" );
}
