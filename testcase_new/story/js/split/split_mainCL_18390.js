/************************************
*@Description：主表执行切分 
*@author：2019-6-6 wangkexin
*@testlinkCase: seqDB-18390
**************************************/
main();
function main ()
{
   var csName = COMMCSNAME;
   var mainClName = CHANGEDPREFIX + "_maincl_18390";
   var subClName = CHANGEDPREFIX + "_subcl_18390";

   if( true == commIsStandalone( db ) )
   {
      println( "run mode is standalone" );
      return;
   }

   //less two groups to split
   var allGroupName = getGroupName2( db, true );
   if( 2 > allGroupName.length )
   {
      println( "--least two groups" );
      return;
   }
   //clean environment before test
   commDropCL( db, csName, mainClName, true, true, "drop maincl in the beginning." );
   commDropCL( db, csName, subClName, true, true, "drop subcl in the beginning." );

   var groupsInfo = getGroupName2( db, true );
   var srcGrName = groupsInfo[0][0];
   var tarGrName = groupsInfo[1][0];

   var options = { ShardingKey: { No: 1 }, ShardingType: "range", Partition: 1024, ReplSize: 0, Group: srcGrName, IsMainCL: true };
   var maincl = commCreateCL( db, csName, mainClName, options, false );

   var options2 = { ShardingKey: { b: 1 }, ShardingType: "hash" };
   var subcl = commCreateCL( db, csName, subClName, options2, false );

   maincl.attachCL( csName + "." + subClName, { LowBound: { No: 0 }, UpBound: { No: 200 } } );
   insertData( db, csName, mainClName, 100 );

   println( "--start split, srcGrName :" + srcGrName + " , tarGrName : " + tarGrName );
   try
   {
      maincl.split( srcGrName, tarGrName, 50 );
      throw "expect failure but succeed.";
   }
   catch( e )
   {
      if( e !== -246 )
      {
         throw buildException( "main()", e, "split main cl", -246, e );
      }
   }

   commDropCL( db, csName, mainClName, true, true, "drop CL in the end." );
}