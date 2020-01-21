/************************************
*@description：seqDB-18390:主表执行切分 
*@author ：2019-6-6 wangkexin init; 2020-01-13 huangxiaoni modify
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
   var mCLName = CHANGEDPREFIX + "_split_m18390";
   var sCLName = CHANGEDPREFIX + "_split_s18390";

   // ready main-sub cl
   commDropCL( db, COMMCSNAME, mCLName, true, true, "drop maincl in the beginning." );
   commDropCL( db, COMMCSNAME, sCLName, true, true, "drop subcl in the beginning." );

   var options = { ShardingKey: { a: 1 }, ShardingType: "range", Group: srcGroupName, IsMainCL: true };
   var mcl = commCreateCL( db, COMMCSNAME, mCLName, options );

   var options = { ShardingKey: { b: 1 }, ShardingType: "hash" };
   commCreateCL( db, COMMCSNAME, sCLName, options );

   mcl.attachCL( COMMCSNAME + "." + sCLName, { LowBound: { a: 0 }, UpBound: { a: 200 } } );

   // split
   try
   {
      mcl.split( srcGroupName, dstGroupName, 50 );
      throw new Error( "expect failure but succeed." );
   }
   catch( e )
   {
      if( e.message !== "-246" )
      {
         throw e;
      }
   }

   commDropCL( db, COMMCSNAME, mCLName, true, true, "drop CL in the end." );
}