/******************************************************************************
 * @Description   : seqDB-26617:更新符使用rename更新字段名
 * @Author        : liuli
 * @CreateTime    : 2022.06.13
 * @LastEditTime  : 2022.06.13
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.clName = COMMCLNAME + "26617";
main( test );

function test ( args )
{
   var cl = args.testCL;

   var docs = [];
   var expResult = [];
   var recsNum = 1000;
   for( var i = 0; i < recsNum; i++ )
   {
      docs.push( { a: i, b: i } );
      expResult.push( { b: i, c: i } );
   }
   cl.insert( docs );

   assert.tryThrow( [SDB_INVALIDARG], function() 
   {
      cl.update( { $rename: { "a": "a" } } );
   } );

   var cursor = cl.find().sort( { b: 1 } );
   commCompareResults( cursor, docs );

   cl.update( { $rename: { "a": "c" } } );

   var cursor = cl.find().sort( { b: 1 } );
   commCompareResults( cursor, expResult );
}
