/************************************
*@Description: 修改replsize
*@author:      luweikang
*@createdate:  2018.4.25
*@testlinkCase:seqDB-14986
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
   var clName = CHANGEDPREFIX + "_14986";

   var options = { ReplSize: 0 };
   var cl = commCreateCL( db, csName, clName, options, true, false, "create CL in the begin" );

   //这个地方写测试步骤
   println( "---test alter ReplSize---" );
   cl.setAttributes( { ReplSize: 1 } );
   checkSnapshot( db, SDB_SNAP_CATALOG, csName, clName, "ReplSize", 1 );

   cl.setAttributes( { ReplSize: 0 } );
   checkSnapshot( db, SDB_SNAP_CATALOG, csName, clName, "ReplSize", 7 );

   commDropCL( db, csName, clName, true, false, "clean cl" );
   println( "---end the test---" );
}
