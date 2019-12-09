/************************************
*@Description： range分区组进行范围切分，其中endcondition字段值不在源分区组内_ST.split.01.024
*@author：2019-5-30 wangkexin
*@testlinkCase: seqDB-5001
**************************************/
main();
function main ()
{
   var csName = COMMCSNAME;
   var clName = CHANGEDPREFIX + "_cl_5001";

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

   var options = { ShardingKey: { a: 1 }, ShardingType: "range", ReplSize: 0, Group: srcGrName };
   var cl = commCreateCLByOption( db, csName, clName, options, false );
   insertData( db, csName, clName, 100 );

   println( "--start split, srcGrName :" + srcGrName + " , tarGrName : " + tarGrName );
   //test a : endcondition 为null
   var endcondition = null;
   try
   {
      cl.split( srcGrName, tarGrName, 0, endcondition )
      throw "expect fail but succeed, endcondition = " + endcondition;
   }
   catch( e )
   {
      if( e !== -6 )
      {
         throw buildException( "split()", e, "", '-6', e );
      }
   }

   //test b : endcondition 为空串
   endcondition = '';
   try
   {
      cl.split( srcGrName, tarGrName, endcondition )
      throw "expect fail but succeed, endcondition = " + endcondition;
   }
   catch( e )
   {
      //-259 : SDB_OUT_OF_BOUND
      if( e !== -259 )
      {
         throw buildException( "split()", e, "", '-259', e );
      }
   }

   commDropCL( db, csName, clName, true, true, "drop CL in the end." );
}