/***************************************************************************
@Description :seqDB-5573:使用离线方式创建Id索引
@Modify list :
              2016-8-10  wuyan  Init
****************************************************************************/
var clName = CHANGEDPREFIX + "_5573";
function main ( db )
{
   // drop collection in the beginning
   commDropCL( db, COMMCSNAME, clName, true, true, "drop collection in the beginning" );

   // create collection
   var idxCL = commCreateCL( db, COMMCSNAME, clName, { AutoIndexId: false, ReplSize: 0, Compressed: true }, true, true );

   // create Idindex
   createIdIndex( idxCL, { SortBufferSize: 128 } );

   // inspect the index
   inspecIndex( idxCL, "$id", "_id", 1 );

   //after create index, insert data
   try
   {
      var doc = [];
      for( var i = 0; i < 1000; ++i )
      {
         doc.push( { _id: i, a: "test" + i } );
      }
      idxCL.insert( doc );
   }
   catch( e )
   {
      println( "Failed to insert date after create index : " + e );
      throw e;
   }

   //test find by index 
   checkExplain( idxCL, { _id: 8 } );

   //check the result of find  
   var expRecs = '[{"_id":999,"a":"test999"}]';
   var rc = idxCL.find( { _id: { $gt: 998 } } ).sort( { _id: 1 } );
   checkCLData( expRecs, rc );

   //drop idIndex
   idxCL.dropIdIndex();
   // inspect the index
   commCheckIndexConsistency( idxCL, "$id", false );


   // drop collection in clean
   commDropCL( db, COMMCSNAME, clName, false, false,
      "drop colleciton in the end" );
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

