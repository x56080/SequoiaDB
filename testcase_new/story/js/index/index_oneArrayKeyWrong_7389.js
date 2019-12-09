/*******************************************************************************
@Description : When create composite index, there can be only one nest array as a
               index key.
@Modify list :
               2014-5-19  xiaojun Hu  Create
*******************************************************************************/
function main ( db )
{
   // drop collection in the beginning
   commDropCL( db, csName, clName, true, true, "drop collection in the beginning" );

   // create collection
   var idxCL = commCreateCL( db, csName, clName, -1, true, true, false, "create collection" );

   // insert data to SDB
   try
   {
      idxCL.insert( { no: 001, name: "A", score: [60, 70, 80], coutry: { china: { guangdong: "guanzhou" } }, age: 15, major: ["English", "Chinese", "Physics"], "class": { grade: "NO.1" } } );
      idxCL.insert( { no: 002, name: "B", score: [60, 71, 80], coutry: { china: { guangdong: "shenzhen" } }, age: 17, major: ["English", "Chinese", "Physics"], "class": { grade: "NO.2" } } );
      idxCL.insert( { no: 003, name: "C", score: [62, 70, 80], coutry: { china: { guangdong: "huizhou" } }, age: 18, major: ["English", "History", "Physics"], "class": { grade: "NO.3" } } );
      idxCL.insert( { no: 004, name: "D", score: [60, 70, 85], coutry: { china: { guangdong: "foshan" } }, age: 17, major: ["English", "Chinese", "Physics"], "class": { grade: "NO.4" } } );
      idxCL.insert( { no: 005, name: "E", score: [65, 75, 85], coutry: { china: { guangdong: "zhuhai" } }, age: 18, major: ["English", "Chinese", "Physics"], "class": { grade: "NO.5" } } );
      var i = 0;
      do
      {
         var count = idxCL.count();
         ++i;
      } while( i < 10 )
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

   // create index
   try
   {
      idxCL.createIndex( "comIndex1", { "score": 1, "coutry.china.guangdong": 1, age: 1 }, true, true );
      //create index for SDB, have two more array,failed
      idxCL.createIndex( "comIndex3", { "score": 1, "coutry.china.guangdong": 1, age: 1, "class.grade": 1 }, true, true );
      idxCL.createIndex( "comIndex2", { "score": 1, "coutry.china.guangdong": 1, age: 1, "major.1": 1 }, true, true );
   }
   catch( e )
   {
      if( -37 != e )
      {
         println( "Wrong when excute the create index, rc=" + e );
         throw e;
      }
   }

   // inspect index
   try
   {
      inspecIndex( idxCL, "comIndex1", "score", 1, true, true );
      inspecIndex( idxCL, "comIndex3", "score", 1, true, true );
   }
   catch( e )
   {
      if( "ErrIdxName" != e )
      {
         throw e;
      }
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

