/******************************************************************************
@Description : 1. collection altered Compress, fail
@Modify list :
               2014-07-10 pusheng Ding  Init
               2019-10-21  luweikang modify
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
   var clName1 = "alter8175_1";
   var clName2 = "alter8175_2";
   var clName3 = "alter8175_3";
   var clName4 = "alter8175_4";
   commDropCL( db, COMMCSNAME, clName1 );
   commDropCL( db, COMMCSNAME, clName2 );
   commDropCL( db, COMMCSNAME, clName3 );
   commDropCL( db, COMMCSNAME, clName4 );

   var cl1 = commCreateCL( db, COMMCSNAME, clName1, { Compressed: false } );
   var cl2 = commCreateCL( db, COMMCSNAME, clName2, { ShardingKey: { id: 1 }, ShardingType: "hash", Compressed: false } );
   var cl3 = commCreateCL( db, COMMCSNAME, clName3, { ShardingKey: { id: 1 }, ShardingType: "range", Compressed: false } );
   var cl4 = commCreateCL( db, COMMCSNAME, clName4, { ShardingKey: { id: 1 }, ShardingType: "range", IsMainCL: true, Compressed: false } );

   cl1.alter( { "Compressed": true } );
   cl2.alter( { "Compressed": true } );
   cl3.alter( { "Compressed": true } );

   try
   {
      cl4.alter( { "Compressed": true } );
      throw "ERR_ALTER_MAINCL";
   }
   catch( e )
   {
      if( e.message != -32 )
      {
         throw e;
      }
   }

   //check cl snapshot
   var snap1 = db.snapshot( 8, { Name: COMMCSNAME + "." + clName1 } );
   var compressionType = snap1.current().toObj()['CompressionType'];
   if( compressionType !== 1 )
   {
      throw new Error( "check compressionType, \nexpect: 1, \nbut found: " + compressionType );
   }

   var snap2 = db.snapshot( 8, { Name: COMMCSNAME + "." + clName2 } );
   var compressionType = snap2.current().toObj()['CompressionType'];
   if( compressionType !== 1 )
   {
      throw new Error( "check compressionType, \nexpect: 1, \nbut found: " + compressionType );
   }

   var snap3 = db.snapshot( 8, { Name: COMMCSNAME + "." + clName3 } );
   var compressionType = snap3.current().toObj()['CompressionType'];
   if( compressionType !== 1 )
   {
      throw new Error( "check compressionType, \nexpect: 1, \nbut found: " + compressionType );
   }

   commDropCL( db, COMMCSNAME, clName1 );
   commDropCL( db, COMMCSNAME, clName2 );
   commDropCL( db, COMMCSNAME, clName3 );
   commDropCL( db, COMMCSNAME, clName4 );
}

