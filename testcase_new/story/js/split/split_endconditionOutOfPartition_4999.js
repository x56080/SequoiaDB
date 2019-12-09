/************************************
*@Description： hash范围切分设置endcondition条件值超出partition范围_ST.split.01.022
*@author：2019-5-30 wangkexin
*@testlinkCase: seqDB-4999
**************************************/
main();
function main ()
{
   var csName = COMMCSNAME;
   var clName = CHANGEDPREFIX + "_cl_4999";

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
   commDropCL( db, csName, clName, true, true, "drop CL in the beginning." );

   var groupsInfo = getGroupName2( db, true );
   var srcGrName = groupsInfo[0][0];
   var tarGrName = groupsInfo[1][0];

   var options = { ShardingKey: { a: 1 }, ShardingType: "hash", Partition: 1024, ReplSize: 0, Group: srcGrName };
   var cl = commCreateCLByOption( db, csName, clName, options, false );
   insertData( db, csName, clName, 100 );

   println( "--start split, srcGrName :" + srcGrName + " , tarGrName : " + tarGrName );
   checkEndconditionOutOfPartition( cl, srcGrName, tarGrName, 1025 );

   commDropCL( db, csName, clName, true, true, "drop CL in the end." );
}

function checkEndconditionOutOfPartition ( cl, srcGrName, tarGrName, endcondition )
{
   try
   {
      cl.split( srcGrName, tarGrName, 0, { Partition: 10 }, { Partition: endcondition } )
      throw "expect fail but succeed, condition = 10, endcondition = " + endcondition;
   }
   catch( e )
   {
      if( e !== -6 )
      {
         throw buildException( "split()", e, "", '-6', e );
      }
   }
}