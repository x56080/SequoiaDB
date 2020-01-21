/************************************
*@description： seqDB-5001:range分区组进行范围切分，其中endcondition字段值不在源分区组内_ST.split.01.024
*@author ：2019-5-30 wangkexin init; 2020-01-13 huangxiaoni modify
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
   if( true == commIsStandalone( db ) )
   {
      println( "---Is standalone." );
      return;
   }

   if( commGetGroupsNum( db ) < 2 )
   {
      println( "---Least two groups" );
      return;
   }

   var groupNames = commGetDataGroupNames( db );
   var srcGroupName = groupNames[0];
   var dstGroupName = groupNames[1];
   var clName = CHANGEDPREFIX + "_split_5001";

   commDropCL( db, COMMCSNAME, clName, true, true, "drop CL in the beginning." );
   var options = { ShardingKey: { a: 1 }, ShardingType: "range", Group: srcGroupName };
   var cl = commCreateCL( db, COMMCSNAME, clName, options );
   insertData( cl, 100 );

   //test a : endcondition 为null
   var endcondition = null;
   try
   {
      cl.split( srcGroupName, dstGroupName, 0, endcondition )
      throw new Error( "expect fail but succeed, endcondition = " + endcondition );
   }
   catch( e )
   {
      if( e.message !== "-6" )
      {
         throw e;
      }
   }

   //test b : endcondition 为空串
   endcondition = '';
   try
   {
      cl.split( srcGroupName, dstGroupName, endcondition )
      throw new Error( "expect fail but succeed, endcondition = " + endcondition );
   }
   catch( e )
   {
      if( e.message !== "-259" )
      {
         throw e;
      }
   }

   commDropCL( db, COMMCSNAME, clName, true, true, "drop CL in the end." );
}