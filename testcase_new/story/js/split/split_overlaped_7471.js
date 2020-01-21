/******************************************************************************
@description : seqDB-7471:指定相同范围条件重复切分
@author : 2014-07-04  PuSheng Ding init; 2016-02-18 Yan Wu modify; 2020-1-14 XiaoNi Huang modify
******************************************************************************/
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
   var clName = CHANGEDPREFIX + "_split_7471";

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
   var clName1 = CHANGEDPREFIX + "_split_hash_5001";
   var clName2 = CHANGEDPREFIX + "_split_range_5001";

   commDropCL( db, COMMCSNAME, clName1, true, true, "drop CL in the begin." );
   commDropCL( db, COMMCSNAME, clName2, true, true, "drop CL in the begin." );
   var cl1 = commCreateCL( db, COMMCSNAME, clName1, { ShardingKey: { a: 1 }, ShardingType: "hash", Group: srcGroupName } );
   var cl2 = commCreateCL( db, COMMCSNAME, clName2, { ShardingKey: { a: 1 }, ShardingType: "range", Group: srcGroupName } );
   cl2.insert( { "a": 1 } );

   // split, the same condition
   splitHash( cl1, srcGroupName, dstGroupName );
   splitRange( cl2, srcGroupName, dstGroupName );

   commDropCL( db, COMMCSNAME, clName1, false, false, "drop CL in the end." );
   commDropCL( db, COMMCSNAME, clName2, false, false, "drop CL in the end." );
}

function splitHash ( cl, srcGroupName, dstGroupName )
{
   cl.split( srcGroupName, dstGroupName, { "Partition": 10 }, { "Partition": 20 } );
   try
   {
      cl.split( srcGroupName, dstGroupName, { "Partition": 10 }, { "Partition": 30 } );
      throw new Error( "expect fail but succeed" );
   }
   catch( e )
   {
      if( e.message !== "-176" )
      {
         throw e;
      }
   }

   cl.split( srcGroupName, dstGroupName, { "Partition": 40 }, { "Partition": 60 } );
   try
   {
      cl.split( srcGroupName, dstGroupName, { "Partition": 50 }, { "Partition": 60 } );
      throw new Error( "expect fail but succeed" );
   }
   catch( e )
   {
      if( e.message !== "-176" )
      {
         throw e;
      }
   }
}

function splitRange ( cl, srcGroupName, dstGroupName )
{
   cl.split( srcGroupName, dstGroupName, { "a": 10 }, { "a": 20 } );
   try
   {
      cl.split( srcGroupName, dstGroupName, { "a": 10 }, { "a": 30 } );
      throw new Error( "expect fail but succeed" );
   }
   catch( e )
   {
      if( e.message !== "-176" )
      {
         throw e;
      }
   }

   cl.split( srcGroupName, dstGroupName, { "a": 40 }, { "a": 60 } );
   try
   {
      cl.split( srcGroupName, dstGroupName, { "a": 50 }, { "a": 60 } );
      throw new Error( "expect fail but succeed" );
   }
   catch( e )
   {
      if( e.message !== "-176" )
      {
         throw e;
      }
   }
}