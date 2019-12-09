/***************************************************************************
@Description :seqDB-5583:删除Id索引，执行插入/更新/删除数据操作
@Modify list :
              2016-8-10  wuyan  Init
****************************************************************************/
var clName = CHANGEDPREFIX + "_5583";
function main ( db )
{
   // drop collection in the beginning
   commDropCL( db, COMMCSNAME, clName, true, true, "drop collection in the beginning" );

   // create collection
   var idxCL = commCreateCLByOption( db, COMMCSNAME, clName, { ReplSize: 0, Compressed: true }, true, true );

   //drop idIndex
   idxCL.dropIdIndex();

   //insert data
   try
   {
      var doc = [];
      for( var i = 0; i < 100; ++i )
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

   //update data
   try
   {
      idxCL.find( { _id: 3 } ).update( { $set: { a: "testa" } } );
   }
   catch( e )
   {
      if( -279 != e )
      {
         println( "update failed , rc = " + e );
         throw e;
      }
   }

   //remove data
   try
   {
      idxCL.remove( { _id: 3 } );
   }
   catch( e )
   {
      if( -279 != e )
      {
         println( "remove failed , rc = " + e );
         throw e;
      }
   }

   // create Idindex
   createIdIndex( idxCL, { SortBufferSize: 256 } );
   //update data and remove data
   try
   {
      idxCL.find( { _id: 3 } ).update( { $set: { a: "testa" } } );
      idxCL.remove( { _id: 2 } );
   }
   catch( e )
   {
      println( "update or remove failed , rc = " + e );
      throw e;
   }

   // inspect the index
   inspecIndex( idxCL, "$id", "_id", 1 );

   //check the result of find  
   var expRecs = '[{"_id":0,"a":"test0"},{"_id":1,"a":"test1"},{"_id":3,"a":"test3"}]';
   var rc = idxCL.find( { _id: { $lt: 4 } } ).sort( { _id: 1 } );
   checkCLData( expRecs, rc );




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

