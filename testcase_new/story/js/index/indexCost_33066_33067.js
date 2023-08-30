/******************************************************************************
 * @Description   : seqDB-33066:不带limit排序查询，通过代价计算选择查询计划
 *                  seqDB-33067:带limit排序查询，通过代价计算选择查询计划
 *                  seqDB-33068:带limit+skip排序查询，且limit+skip <= optstartcostlimit（默认值）情况下通过代价计算选择查询计划
 *                  seqDB-33069:带limit+skip排序查询，且limit+skip > optstartcostlimit（默认值）情况下通过代价计算选择查询计划
 * @Author        : wuyan
 * @CreateTime    : 2022.08.30
 * @LastEditTime  : 2023.08.30
 * @LastEditors   : wu yan
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.clName = COMMCLNAME + "_index33066";

main( test );
function test ()
{
   var cl = testPara.testCL;
   cl.createIndex( "a", { "a": 1 } );
   cl.createIndex( "ab", { "a": 1, "b": 1 } );
   cl.createIndex( "ac", { "a": 1, "c": 1 } );
   cl.createIndex( "bc", { "b": 1, "c": 1 } );
   cl.createIndex( "abc", { "a": 1, "b": 1, "c": 1 } );

   //空表执行explain
   testExplain( cl, {}, { a: -1 }, "a", false );
   testExplain( cl, {}, { a: 1 }, "a", false );
   testExplain( cl, {}, { a: 1, b: 1 }, "ab", false );
   testExplain( cl, {}, { a: -1, c: -1 }, "ac", false );
   testExplain( cl, {}, { b: 1, c: 1 }, "bc", false );
   testExplain( cl, {}, { a: 1, b: 1, c: 1 }, "abc", false );
   testExplain( cl, {}, { d: 1, a: 1 }, "", false );

   var data = [];
   for( i = 0; i < 100000; ++i )
   {
      data.push( { a: i, b: Math.round( ( Math.random() * 100000 ) ), c: i, d: i } )
   }
   cl.insert( data );

   testExplain( cl, {}, { a: -1 }, "a", false );
   testExplain( cl, {}, { a: 1 }, "a", false );
   testExplain( cl, {}, { a: 1, b: 1 }, "ab", false );
   testExplain( cl, {}, { a: -1, c: -1 }, "ac", false );
   testExplain( cl, {}, { b: 1, c: 1 }, "bc", false );
   testExplain( cl, {}, { a: 1, b: 1, c: 1 }, "abc", false );
   testExplain( cl, {}, { d: 1, a: 1 }, "", false );

   db.analyze( { Collection: COMMCSNAME + "." + testConf.clName } );

   //执行analyze后，通过代价计算选择查询计划
   testExplain( cl, {}, { a: -1 }, "a", true );
   testExplain( cl, {}, { a: 1 }, "a", true );
   testExplain( cl, {}, { a: 1, b: 1 }, "ab", true );
   testExplain( cl, {}, { a: -1, c: -1 }, "ac", true );
   testExplain( cl, {}, { b: 1, c: 1 }, "bc", true );
   testExplain( cl, {}, { a: 1, b: 1, c: 1 }, "abc", true );
   testExplain( cl, {}, { d: 1, a: 1 }, "", true );
}

function testExplain ( cl, query, orderBy, expectedIndexName, affectedByAnalyze )
{
   var indexName;
   //不带limit排序查询，通过代价计算选择查询计划走表扫描为 “”
   indexName = cl.find( query ).sort( orderBy ).explain().current().toObj()["IndexName"];
   assert.equal( indexName, affectedByAnalyze ? "" : expectedIndexName );

   //limit排序查询，通过代价计算选择查询计划走索引扫描
   indexName = cl.find( query ).sort( orderBy ).limit( 1 ).explain().current().toObj()["IndexName"];
   assert.equal( indexName, expectedIndexName );

   //limit排序查询，通过代价计算选择查询计划limit大于optstartcostlimit（默认200），走表扫描
   indexName = cl.find( query ).sort( orderBy ).limit( 500 ).explain().current().toObj()["IndexName"];
   assert.equal( indexName, affectedByAnalyze ? "" : expectedIndexName );

   //limit+skip < optstartcostlimit(默认值100)，通过代价计算选择查询计划选择索引扫描
   indexName = cl.find( query ).sort( orderBy ).limit( 1 ).skip( 1 ).explain().current().toObj()["IndexName"];
   assert.equal( indexName, expectedIndexName );

   //limit+skip = optstartcostlimit(默认值100)，通过代价计算选择查询计划选择索引扫描
   indexName = cl.find( query ).sort( orderBy ).limit( 99 ).skip( 1 ).explain().current().toObj()["IndexName"];
   assert.equal( indexName, expectedIndexName );

   //limit+skip > optstartcostlimit(默认值100)，通过代价计算选择查询计划选择表扫描
   indexName = cl.find( query ).sort( orderBy ).limit( 1 ).skip( 100 ).explain().current().toObj()["IndexName"];
   assert.equal( indexName, affectedByAnalyze ? "" : expectedIndexName );

   //limit+skip > optstartcostlimit(默认值100)，通过代价计算选择查询计划选择表扫描
   indexName = cl.find( query ).sort( orderBy ).limit( 100 ).skip( 1 ).explain().current().toObj()["IndexName"];
   assert.equal( indexName, affectedByAnalyze ? "" : expectedIndexName );
}