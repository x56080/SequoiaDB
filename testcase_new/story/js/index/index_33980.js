/*
 * @Description   : seqDB-33980:指定hint查询表数据范围外的数据
 * @Author        : huangxiaoni
 * @CreateTime    : 2024-01-25
 * @LastEditors   : huangxiaoni
 * @LastEditTime  : 2024-01-29
 */
testConf.clName = CHANGEDPREFIX + "_33980";

main( test );
function test ( testPara )
{
   var cl = testPara.testCL;
   var indexName = "idx_33980";

   cl.createIndex( indexName, { a: 1, b: 1, c: 1 } );
   var docs = [];
   for( i = 0; i < 100000; i++ )
   {
      docs.push( { a: i, b: i, c: i, d: i } )
   };
   cl.insert( docs );

   // explain检查IndexRead
   var cursor = cl.find( { $or: [{ a: 1, b: 1 }, { a: 2, b: 2 }], c: 2000000 } ).hint( { "$Range": { "IsAllEqual": true, "PrefixNum": 3, "IndexValue": [{ "1": 1, "2": 1, "3": 2000000 }, { "1": 2, "2": 2, "3": 2000000 }] }, "": indexName } ).explain( { Run: true } );
   var indexRead = cursor.next().toObj().IndexRead;
   assert.equal( indexRead, 7 );

   // find检查返回数据正确性
   var cursor = cl.find( { $or: [{ a: 1, b: 1 }, { a: 2, b: 2 }], c: 2000000 } ).hint( { "$Range": { "IsAllEqual": true, "PrefixNum": 3, "IndexValue": [{ "1": 1, "2": 1, "3": 2000000 }, { "1": 2, "2": 2, "3": 2000000 }] }, "": indexName } );
   commCompareResults( cursor, [] );
}