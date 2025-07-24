/************************************
*@Description: seqDB-11353:seqDB-11357:rtnPredicate为[valA, valA]，数据类型为bool，数据均匀情况下的索引选择 
*@author:      chimanzhao
*@createdate:  2020.5.8
*@testlinkCase: seqDB-11357
**************************************/
testConf.clName = COMMCLNAME + "_11357";

main( test );

function test ( testPara )
{
   var dbcl = testPara.testCL;
   var fullclName = COMMCSNAME + "." + testConf.clName;
   dbcl.createIndex( "a", { a: 1 } );
   dbcl.createIndex( "b", { b: -1 } );
   dbcl.createIndex( "ab", { a: 1, b: 1 } );

   //设置查询条件
   var conds = [{ a: { $et: true } }, { a: { $et: false } }, { a: { $ne: true } }, { a: { $ne: false } }];
   var indexName = "";
   var scanType = "tbscan";
   var indexName1 = "a";
   var scanType1 = "ixscan";
   //不计算IO代价
   var docs = [];
   for( var i = 0; i < 100; i++ )
   {
      if( i % 2 )
      {
         docs.push( { a: true, b: i } )
      }
      else
      {
         docs.push( { a: false, b: i } )
      }
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
   testExplain( conds, dbcl, indexName1, scanType1 );
}

