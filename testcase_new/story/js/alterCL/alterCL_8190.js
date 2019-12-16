/******************************************************************************
*@Description : 1. main-collection altered to partition-collection
*@Modify list :
*               2014-07-09 pusheng Ding  Init
*               2015-03-28 xiaojun Hu    Changed
*               2019-10-21  luweikang modify
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

   var mainCLName = "alter8190_main";
   var subCLName = "alter8190_sub";

   commDropCL( db, COMMCSNAME, mainCLName );
   commDropCL( db, COMMCSNAME, subCLName );

   var maincl = commCreateCL( db, COMMCSNAME, mainCLName, { IsMainCL: true, ShardingKey: { id: 1 }, ShardingType: "range", ReplSize: 1 } );
   commCreateCL( db, COMMCSNAME, subCLName, { ReplSize: 1 } );

   //alters shardingType
   try
   {
      maincl.alter( { ShardingType: "range" } );
      throw new Error( "ERR_ALTER_CL" );
   }
   catch( e )
   {
      if( e.message != -32 )
      {
         throw new Error( "alter main cl shardingType, \nexp: -32, \nbut found: " + e );
      }
   }

   try
   {
      maincl.alter( { ShardingType: "hash" } );
      throw new Error( "ERR_ALTER_CL" );
   }
   catch( e )
   {
      if( e.message != -32 )
      {
         throw e;
      }
   }

   maincl.attachCL( COMMCSNAME + "." + subCLName, { LowBound: { id: MinKey() }, UpBound: { id: MaxKey() } } );
   var data = [];
   for( var i = 0; i < 1000; i++ )
   {
      data.push( { "id": i, "text": "test alter " + i } );
   }
   maincl.insert( data );

   //alters shardingType
   try
   {
      maincl.alter( { ShardingType: "range" } );
      throw new Error( "ERR_ALTER_CL" );
   }
   catch( e )
   {
      if( e.message != -32 )
      {
         throw e;
      }
   }

   try
   {
      maincl.alter( { ShardingType: "hash" } );
      throw new Error( "ERR_ALTER_CL" );
   }
   catch( e )
   {
      if( e.message != -32 )
      {
         throw e;
      }
   }

   var num = maincl.count();
   if( num != 1000 )
   {
      throw new Error( "check recordNum, \nexpect: 1000, \nbut found: " + num );
   }

   //clean test-env
   commDropCL( db, COMMCSNAME, mainCLName );
   commDropCL( db, COMMCSNAME, subCLName );
}

