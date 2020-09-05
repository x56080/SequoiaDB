/*****************************************************************************
@Description : The max index key is 64. The number of index is greater than 64
               will failed to created.
@Modify list :
               2014-5-19  xiaojun Hu  Create
               2016-3-4   yan wu Modify(增加测试点：创建第65个索引报错；删除无效检测结果）
******************************************************************************/
function main ( db )
{
   // drop collection in the beginning
   commDropCL( db, csName, clName, true, true, "drop collection in the beginning" );

   // create collection
   var idxCL = commCreateCL( db, csName, clName, {}, true, false, "create collection" );

   // insert data to SDB
   try
   {
      idxCL.insert( {
         no: 001, name: "A", age: 2,
         "object1": { "object2": { "object3": { "object4": { "object5": "5LayerObject" } } } }
      } );
      idxCL.insert( { no: 002, name: "a", age: 3, "a1": { "a2": { "a3": { "a4": { "a5": "5LayerObject_a" } } } } } );
      idxCL.insert( { no: 003, name: "B", age: 3, "b1": { "b2": { "b3": { "b4": { "b5": "5LayerObject_b" } } } } } );
      idxCL.insert( { no: 004, name: "C", age: 3, "c1": { "c2": { "c3": { "c4": { "c5": "5LayerObject_c" } } } } } );
      idxCL.insert( { no: 005, name: "D", age: 3, "d1": { "d2": { "d3": { "d4": { "d5": "5LayerObject_d" } } } } } );
      idxCL.insert( {
         no: 006, name: "E", age: 2, array1: [{
            "array2": [{
               "array3": [{
                  "array4": ["array5",
                     "temp4"]
               }, "temp3"]
            }, "temp2"]
         }, "temp1"]
      } );
      var i = 0;
      do
      {
         var count = idxCL.count();
         ++i;
      } while( i < 10 )
      if( 6 != count )
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
   for( i = 1; i <= 63; ++i )
   {
      var idxName = "noIndex" + i;
      var idxKey = "no" + i;
      var str = "{" + idxKey + ":1}";
      var keyObj = eval( "(" + str + ")" );
      idxCL.createIndex( idxName, keyObj, false, false );
   }
   //inspect the index that created
   for( i = 1; i <= 63; ++i )
   {
      var idxName = "noIndex" + i;
   }

   //create the index and geater than 64 error
   createIndex( idxCL, "testindex", { a: 1 }, false, false, -42 );

   /*var getIndex64 = undefined ;
   try
   {
      getIndex64 = idxCL.getIndex( "noIndex64" ) ;
   }
   catch(e)
   {
      getIndex64 = undefined ;
   }
   if ( undefined != getIndex64 )
   {
      println( "Error,create the index and geater than 64" ) ;
      throw "ErrCreate64Idx" ;
   }*/

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
   println( "Failed to create 64 indexes, rc=" + e );
   throw e;
}

