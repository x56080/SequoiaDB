/******************************************************************************
*@Description: 分区表修改AutoSplit字段
*@author:      luweikang
*@createdate:  2019.11.15
*@testlinkCase:seqDB-20260
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
   if( commIsStandalone( db ) )
   {
      println( "Run mode is standalone" );
      return;
   }
   var groupName = commGetGroups( db );
   if( groupName.length < 2 )
   {
      println( "group num less 2" );
      return;
   }

   var clName = "alter20260";
   commDropCL( db, COMMCSNAME, clName );

   var cl = commCreateCL( db, COMMCSNAME, clName, { ShardingKey: { id: 1 }, ShardingType: "hash", AutoSplit: true } );

   //alters AutoSplit
   cl.alter( { AutoSplit: true } );
   checkSnapshot( db, SDB_SNAP_CATALOG, COMMCSNAME, clName, "AutoSplit", true );

   cl.setAttributes( { AutoSplit: true } );
   checkSnapshot( db, SDB_SNAP_CATALOG, COMMCSNAME, clName, "AutoSplit", true );

   try
   {
      cl.alter( { AutoSplit: false } );
      throw new Error( "ERR_ALTER_CL" );
   }
   catch( e )
   {
      if( e.message != -32 )
      {
         throw e;
      }
   }
   checkSnapshot( db, SDB_SNAP_CATALOG, COMMCSNAME, clName, "AutoSplit", true );

   try
   {
      cl.setAttributes( { AutoSplit: false } );
      throw new Error( "ERR_ALTER_CL" );
   }
   catch( e )
   {
      if( e.message != -32 )
      {
         throw e;
      }
   }
   checkSnapshot( db, SDB_SNAP_CATALOG, COMMCSNAME, clName, "AutoSplit", true );

   //clean test-env
   commDropCL( db, COMMCSNAME, clName );
}

