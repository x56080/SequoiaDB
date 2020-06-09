/************************************
*@Description: seqDB-11361:rtnPredicate为[valA, valB]，唯一索引下的索引选择
*@author:      chimanzhao
*@createdate:  2020.5.12
*@testlinkCase: seqDB-11361
**************************************/
testConf.clName = COMMCLNAME + "_11361";

main( test )

function test ( testPara )
{
   var dbcl = testPara.testCL;
   dbcl.createIndex( "a", { a: 1 }, true );
   dbcl.createIndex( "b", { b: -1 }, true );
   dbcl.createIndex( "ab", { a: 1, b: 1 }, true );

   //设置查询条件
   var conds = [{ a: { $gt: 250 } }, { a: { $gte: 250 } }, { a: { $lt: 250 } }, { a: { $lte: 250 } }];
   var indexName = "a";
   var scanType = "ixscan";
   var indexName1 = "";
   var scanType1 = "tbscan";

   //不计算IO代价
   var docs = [];
   for( var i = 0; i < 1000; i++ )
   {
      docs.push( { a: i, b: i, c: -i } )
   }
   dbcl.insert( docs );
   testExplain( conds, dbcl, indexName1, scanType1 );

   db.analyze();
   var expNeedEvalIO = false;
   checkNeedEvalIO( dbcl, expNeedEvalIO );

   testExplain( conds, dbcl, indexName1, scanType1 );

   //计算IO代价
   var docs = [];
   for( var i = 0; i < 50000; i++ )
   {
      docs.push( { d: i } )
   }
   dbcl.insert( docs );
   testExplain( conds, dbcl, indexName1, scanType1 );

   db.analyze();
   var expNeedEvalIO = true;
   checkNeedEvalIO( dbcl, expNeedEvalIO );

   testExplain( conds, dbcl, indexName, scanType );
}

