/***************************************************************************
@Description :First insert data ,and then create index. The index have
              unique ,but don't have enforced arguments.
@Modify list :
              2014-5-15  xiaojun Hu  Create
****************************************************************************/
function main ( db )
{
   // drop collection in the beginning
   commDropCL( db, csName, clName, true, true, "drop collection in the beginning" );

   // create collection
   var idxCL = commCreateCL( db, csName, clName, -1, true, true, false, "create collection" );

   // insert data to SDB
   try
   {
      idxCL.insert( { no: 001, name: "A", age: 2 } );
      idxCL.insert( { no: 002, name: "B", age: 2 } );
      idxCL.insert( { no: 003, name: "C", "姓名": "张" } );
      idxCL.insert( { no: 004, name: "C", "姓名": "张" } );
      count = idxCL.count();
      if( 4 != count )
      {
         println( "Wrong number of record :" + count );
         throw "ErrNumRecord"
      }
   }
   catch( e )
   {
      println( "Failed to insert date after create index : " + e );
      throw e;
   }

   // create index
   createIndex( idxCL, "noIndex", { no: 1 }, true, false );
   createIndex( idxCL, "nameIndex", { name: -1 }, true, false, -38 );
   createIndex( idxCL, "姓名索引", { "姓名": 1 }, true, false, -38 );
   createIndex( idxCL, "ageIndex", { "age": 1 }, true, false, -38 );

   // inspect the index
   try
   {
      inspecIndex( idxCL, "noIndex", "no", 1, true, false );
      inspecIndex( idxCL, "nameIndex", "name", -1, true, false );
      inspecIndex( idxCL, "姓名索引", "姓名", 1, true, false );
      inspecIndex( idxCL, "ageIndex", "age", 1, true, false );
      println( "Can go end of memory." );
   }
   catch( e )
   {
      if( "ErrIdxName" != e )
      {
         throw e;
      }
   }

   //insert data after create index
   try
   {
      idxCL.insert( { no: 005, name: "D", "姓名": "汉", age: 8 } );
      var count = idxCL.count();
      if( 5 != count )
      {
         println( "Wrong number of record :" + count );
         throw "ErrNumRecord";
      }
   }
   catch( e )
   {
      println( "Failed to insert date after create index : " + e );
      throw e;
   }

   // drop collection in clean
   commDropCL( db, csName, clName, false, false,
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

