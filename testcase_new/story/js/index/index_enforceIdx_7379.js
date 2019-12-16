/***************************************************************************
@Description :First create index , and then insert data. The index have
              unique and enforced arguments. enforced unique index.
@Modify list :
              2014-5-15  xiaojun Hu  Create
              2016-3-4   yan wu Modify(增加结果检测（查看访问计划是否走索引、走索引查询数据是否正确,选择其中一条记录检查）)
****************************************************************************/
function main ( db )
{
   // drop collection in the beginning
   commDropCL( db, csName, clName, true, true, "drop collection in the beginning" );

   // create collection
   var idxCL = commCreateCL( db, csName, clName, {}, true, false, "create collection" );

   // create index
   createIndex( idxCL, "noIndex", { no: 1 }, true, true );
   createIndex( idxCL, "nameIndex", { name: -1 }, true, true );
   createIndex( idxCL, "姓名索引", { "姓名": 1 }, true, true );
   createIndex( idxCL, "ageIndex", { "age": 1 }, true, true );

   // inspect the index
   try
   {
      inspecIndex( idxCL, "noIndex", "no", 1, true, true );
      inspecIndex( idxCL, "nameIndex", "name", -1, true, true );
      inspecIndex( idxCL, "姓名索引", "姓名", 1, true, true );
      inspecIndex( idxCL, "ageIndex", "age", 1, true, true );
      println( "Can go end of program." );
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
      idxCL.insert( { no: 001, name: "A", "姓名": "李", age: 1 } );
      idxCL.insert( { no: 002, name: "B", "姓名": "王", age: 2 } );
      idxCL.insert( { no: 003, name: "C", "姓名": "张", age: 3 } );
      idxCL.insert( { no: 004, name: "D", "姓名": "庄", age: 5 } );
      idxCL.insert( { no: 005, name: "E", "姓名": "汉", age: 8 } );
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

   //test find by index 
   checkExplain( idxCL, { "姓名": "李" } );

   //check the result of find  
   checkResult( idxCL, { "姓名": "李" } );

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

