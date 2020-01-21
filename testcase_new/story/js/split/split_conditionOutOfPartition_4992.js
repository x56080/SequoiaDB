/************************************
*@description ： seqDB-4992:hash范围切分设置condition条件值超出partition范围
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
   var clName = CHANGEDPREFIX + "_split_4992";

   commDropCL( db, COMMCSNAME, clName, true, true, "drop CL in the beginning." );
   var options = { ShardingKey: { a: 1 }, ShardingType: "hash", Partition: 1024, Group: srcGroupName };
   var cl = commCreateCL( db, COMMCSNAME, clName, options );
   insertData( cl, 100 );

   var condition = { "Partition": 1024 };
   try
   {
      cl.split( srcGroupName, dstGroupName, condition );
      throw new Error( "expect fail but succeed, inavalid condition = " + JSON.stringify( condition ) );
   }
   catch( e )
   {
      if( e.message !== "-6" )
      {
         throw e;
      }
   }

   var condition = { "Partition": -1 };
   try
   {
      cl.split( srcGroupName, dstGroupName, condition );
      throw new Error( "expect fail but succeed, inavalid condition = " + JSON.stringify( condition ) );
   }
   catch( e )
   {
      if( e.message !== "-6" )
      {
         throw e;
      }
   }

   commDropCL( db, COMMCSNAME, clName, true, true, "drop CL in the end." );
}