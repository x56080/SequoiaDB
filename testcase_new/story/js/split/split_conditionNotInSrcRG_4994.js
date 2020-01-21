/************************************
*@description ：seqDB-4994:range分区组进行范围切分，其中condition字段值不在源分区组内
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
   var clName = CHANGEDPREFIX + "_split_4994";

   commDropCL( db, COMMCSNAME, clName, true, true, "drop CL in the beginning." );
   var options = { ShardingKey: { a: 1 }, ShardingType: "range", Group: srcGroupName };
   var cl = commCreateCL( db, COMMCSNAME, clName, options );
   insertData( cl, 100 );

   //test a : condition为null
   var condition = null;
   try
   {
      cl.split( srcGroupName, dstGroupName, condition )
      throw new Error( "expect fail but succeed, condition = " + condition );
   }
   catch( e )
   {
      if( e.message !== "-6" )
      {
         throw e;
      }
   }

   //test b : condition为空串
   condition = '';
   try
   {
      cl.split( srcGroupName, dstGroupName, condition )
      throw new Error( "expect fail but succeed, condition = " + condition );
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