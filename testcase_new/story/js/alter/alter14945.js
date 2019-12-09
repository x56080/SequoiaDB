/************************************
*@Description: 分区表关闭切分功能后执行切分
*@author:      luweikang
*@createdate:  2018.4.25
*@testlinkCase:seqDB-14945
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
   var clName = CHANGEDPREFIX + "_14945";

   var options = { ShardingType: 'hash', ShardingKey: { a: 1 } };
   var cl = commCreateCLByOption( db, csName, clName, options, true, false, "create CL in the begin" );


   //关闭分区功能
   println( "---close sharding---" );
   cl.disableSharding();

   println( "---split cl---" );
   var splitGroup = getSplitGroup( db, csName, clName );
   try
   {
      println( splitGroup.srcGroup + " split to " + splitGroup.tarGroup );
      cl.split( splitGroup.srcGroup, splitGroup.tarGroup, 50 );
   }
   catch( e )
   {
      if( e !== -169 )
      {
         throw e;
      }
   }
   checkSnapshot( db, SDB_SNAP_CATALOG, csName, clName, "ShardingKey", undefined );
   commDropCL( db, csName, clName, true, false, "clean cl" );
   println( "---end the test---" );
}
