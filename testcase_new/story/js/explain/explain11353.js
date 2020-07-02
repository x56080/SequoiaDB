/************************************
*@Description: seqDB-11353:seqDB-11353:rtnPredicate为[$minKey, $maxKey]的索引选择
*@author:      chimanzhao
*@createdate:  2020.4.25
*@testlinkCase: seqDB-11353
**************************************/
testConf.clName = COMMCLNAME + "_11353";

main( test );

function test ( testPara )
{
   var dbcl = testPara.testCL;
   dbcl.createIndex( "a", { a: 1 } );
   var fullclName = COMMCSNAME + "." + testConf.clName;

   //设置查询条件
   var conds = [{ b: 1 }, { $or: [{ a: 1 }, { c: 1 }] }, { $not: [{ a: 1 }, { c: 1 }] }];
   var indexName = "";
   var scanType = "tbscan";

   //不计算IO代价
   var docs = [];
   for( var i = 0; i < 1000; i++ )
   {
      docs.push( { a: i, b: i, c: -i } )
   }
   dbcl.insert( docs );

   testExplain( conds, dbcl, indexName, scanType );

   db.analyze( { Collection: fullclName } );
   var expNeedEvalIO = false;
   checkNeedEvalIO( dbcl, expNeedEvalIO );

   testExplain( conds, dbcl, indexName, scanType );

   //计算IO代价
   var docs = [];
   for( var i = 0; i < 50000; i++ )
   {
      docs.push( { d: i } )
   }
   dbcl.insert( docs );

   testExplain( conds, dbcl, indexName, scanType );

   db.analyze( { Collection: fullclName } );
   var expNeedEvalIO = true;
   checkNeedEvalIO( dbcl, expNeedEvalIO );

   testExplain( conds, dbcl, indexName, scanType );
}