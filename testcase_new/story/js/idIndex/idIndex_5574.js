/***************************************************************************
@Description :seqDB-5574:在主子表中创建离线模式Id索引
@Modify list :
              2016-8-10  wuyan  Init
****************************************************************************/
var clName = CHANGEDPREFIX + "_5574";

function main ( db )
{
   commDropCL( db, COMMCSNAME, clName, true, true, "drop cl in the beginning" );
   commDropCL( db, COMMCSNAME, "subCL1", true, true, "drop cl in the beginning" );
   commDropCL( db, COMMCSNAME, "subCL2", true, true, "drop cl in the beginning" );
   try
   {
      //@ clean start:
      if( true == commIsStandalone( db ) )
      {
         println( "run mode is standalone" );
         return;
      }

      // create collection
      var options = { ShardingKey: { a: 1 }, ShardingType: "range", ReplSize: 0, Compressed: true, IsMainCL: true }
      var mainCL = commCreateCLByOption( db, COMMCSNAME, clName, options, true, true );
      var subCL1 = commCreateCLByOption( db, COMMCSNAME, "subCL1", { ShardingKey: { a: 1 }, ShardingType: "hash", AutoIndexId: false, ReplSize: 0, Compressed: true }, true, true );
      var subCL2 = commCreateCLByOption( db, COMMCSNAME, "subCL2", { ShardingKey: { a: 1 }, ShardingType: "hash", AutoIndexId: false, ReplSize: 0, Compressed: true }, true, true );

      mainCL.attachCL( COMMCSNAME + ".subCL1", { LowBound: { a: 0 }, UpBound: { a: 100 } } );
      mainCL.attachCL( COMMCSNAME + ".subCL2", { LowBound: { a: 100 }, UpBound: { a: 200 } } );
   }
   catch( e )
   {
      throw e;
   }
   // create Idindex
   createIdIndex( mainCL, { SortBufferSize: 128 } );

   // inspect the index
   inspecIndex( mainCL, "$id", "_id", 1 );

   //after create index, insert data
   try
   {
      var doc = [];
      for( var i = 0; i < 200; ++i )
      {
         doc.push( { _id: i, a: i, b: "test" + i } );
      }
      mainCL.insert( doc );
   }
   catch( e )
   {
      println( "Failed to insert date after create index : " + e );
      throw e;
   }

   //check the result of find  
   var expRecs = '[{"_id":170,"a":170,"b":"test170"}]';
   var rc = mainCL.find( { _id: 170 } ).hint( { "": "$id" } );
   checkCLData( expRecs, rc );

   //drop idIndex
   mainCL.dropIdIndex();
   // inspect the index
   commCheckIndex( mainCL, "$id", false );


   // drop collection in clean   
   commDropCL( db, COMMCSNAME, "subCL1", false, false, "drop colleciton in the end" );
   commDropCL( db, COMMCSNAME, "subCL2", false, false, "drop colleciton in the end" );
   commDropCL( db, COMMCSNAME, clName, false, false, "drop colleciton in the end" );

}

try
{
   main( db );
   db.close();
}
catch( e )
{
   throw e;
}

