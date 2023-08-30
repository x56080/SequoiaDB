/******************************************************************************
 * @Description   : seqDB-33068:带limit+skip排序查询，且limit+skip <= optstartcostlimit情况下通过代价计算选择查询计划
 *                  seqDB-33069:带limit+skip排序查询，且limit+skip > optstartcostlimit情况下通过代价计算选择查询计划
 * @Author        : wuyan
 * @CreateTime    : 2022.08.30
 * @LastEditTime  : 2023.08.30
 * @LastEditors   : wu yan
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.clName = COMMCLNAME + "_index33068";

main( test );
function test ()
{
   var cl = testPara.testCL;
   cl.createIndex( "a", { "a": 1 } );
   cl.createIndex( "ab", { "a": 1, "b": 1 } );
   cl.createIndex( "ac", { "a": 1, "c": 1 } );
   cl.createIndex( "bc", { "b": 1, "c": 1 } );
   cl.createIndex( "abc", { "a": 1, "b": 1, "c": 1 } );

   var data = [];
   for( i = 0; i < 100000; ++i )
   {
      data.push( { a: i, b: Math.round( ( Math.random() * 100000 ) ), c: i, d: i } )
   }
   cl.insert( data );

   db.analyze( { Collection: COMMCSNAME + "." + testConf.clName } );
   try
   {
      //设置optstartcostlimit为5000
      db.updateConf( { optstartcostlimit: 5000 } );
      //执行analyze后，通过代价计算选择查询计划   
      testExplain( cl, {}, { a: -1 }, "a" );
      testExplain( cl, {}, { a: 1 }, "a" );
      testExplain( cl, {}, { a: 1, b: 1 }, "ab" );
      testExplain( cl, {}, { a: -1, c: -1 }, "ac" );
      testExplain( cl, {}, { b: 1, c: 1 }, "bc" );
      testExplain( cl, {}, { a: 1, b: 1, c: 1 }, "abc" );
      testExplain( cl, {}, { d: 1, a: 1 }, "" );
   }
   finally
   {
      db.deleteConf( { optstartcostlimit: 1 } );
   }
}

function testExplain ( cl, query, orderBy, expectedIndexName )
{
   var indexName;
   //limit+skip < optstartcostlimit，通过代价计算选择查询计划选择索引扫描
   indexName = cl.find( query ).sort( orderBy ).limit( 4000 ).explain().current().toObj()["IndexName"];
   assert.equal( indexName, expectedIndexName );

   indexName = cl.find( query ).sort( orderBy ).limit( 1000 ).skip( 100 ).explain().current().toObj()["IndexName"];
   assert.equal( indexName, expectedIndexName );

   //limit+skip = optstartcostlimit，通过代价计算选择查询计划选择索引扫描
   indexName = cl.find( query ).sort( orderBy ).limit( 5000 ).explain().current().toObj()["IndexName"];
   assert.equal( indexName, expectedIndexName );

   indexName = cl.find( query ).sort( orderBy ).limit( 4999 ).skip( 1 ).explain().current().toObj()["IndexName"];
   assert.equal( indexName, expectedIndexName );

   indexName = cl.find( query ).sort( orderBy ).limit( 1 ).skip( 4999 ).explain().current().toObj()["IndexName"];
   assert.equal( indexName, expectedIndexName );

   //limit+skip > optstartcostlimit，通过代价计算选择查询计划选择表扫描
   indexName = cl.find( query ).sort( orderBy ).limit( 5001 ).explain().current().toObj()["IndexName"];
   assert.equal( indexName, "" );

   indexName = cl.find( query ).sort( orderBy ).limit( 1 ).skip( 5000 ).explain().current().toObj()["IndexName"];
   assert.equal( indexName, "" );

   indexName = cl.find( query ).sort( orderBy ).limit( 5000 ).skip( 1 ).explain().current().toObj()["IndexName"];
   assert.equal( indexName, "" );
}