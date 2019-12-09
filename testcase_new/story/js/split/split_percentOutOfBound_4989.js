/************************************
*@Description：percent超过边界值_ST.split.01.012
*@author：2019-5-30 wangkexin
*@testlinkCase: seqDB-4989
**************************************/
main();
function main ()
{
   var csName = COMMCSNAME;
   var clName = CHANGEDPREFIX + "_cl_4989";

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

   var options = { ShardingKey: { a: 1 }, ShardingType: "hash", ReplSize: 0, Group: srcGrName };
   var cl = commCreateCLByOption( db, csName, clName, options, false );
   insertData( db, csName, clName, 100 );

   println( "--start split, srcGrName :" + srcGrName + " , tarGrName : " + tarGrName );
   checkPercentOutOfBound( cl, srcGrName, tarGrName, -0.001 );
   checkPercentOutOfBound( cl, srcGrName, tarGrName, 0 );
   checkPercentOutOfBound( cl, srcGrName, tarGrName, 100.001 );

   commDropCL( db, csName, clName, true, true, "drop CL in the end." );
}

function checkPercentOutOfBound ( cl, srcGrName, tarGrName, percent )
{
   try
   {
      cl.split( srcGrName, tarGrName, percent )
      throw "expect fail but succeed, percent = " + percent;
   }
   catch( e )
   {
      if( e !== -6 )
      {
         throw buildException( "split()", e, "", '-6', e );
      }
   }
}