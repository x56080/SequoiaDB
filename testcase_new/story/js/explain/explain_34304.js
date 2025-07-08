/************************************
*@Description: seqDB-34304: 查询带 limit、skip 检查访问计划信息
*@author:      linsuqiang
*@createdate:  2025.7.8
*@testlinkCase: seqDB-34304
**************************************/
testConf.clName = COMMCLNAME + "_34304";

main( test );

function test ( testPara )
{
   // prepare
   var dbcl = testPara.testCL;
   var docs = [];
   const recordCount = 50000;
   const bulkSize = 1000;
   for( var i = 0; i < recordCount; i++ )
   {
      docs.push( { a: i, b: i, c: -i } );
      if ( docs.length >= bulkSize )
      {
         dbcl.insert( docs );
         docs = [];
      }
   }
   dbcl.createIndex( "a", { a: 1 } );
   dbcl.createIndex( "b", { b: 1 } );

   // test explain
   checkExplain( dbcl, { a: { $et: 100 } }, "a", "ixscan", {}, {}, 1, 1 );
   checkExplain( dbcl, { a: { $gte: 100 } }, "a", "ixscan", {}, {}, 1, 1 );
   checkExplain( dbcl, { a: { $gte: 100 } }, "", "tbscan", {}, {}, 40000, 1 );
   checkExplain( dbcl, { a: { $gte: 100, $lte: 110 } }, "a", "ixscan", {}, {}, 1, 1 );
   checkExplain( dbcl, { a: { $gte: 100, $lte: 110 } }, "a", "ixscan", {}, {}, 40000, 1 );
   checkExplain( dbcl, {}, "a", "ixscan", { a: 1 }, {}, 1, 0 );
   checkExplain( dbcl, {}, "", "tbscan", { a: 1 }, {}, 1, 10000 );
   checkExplain( dbcl, {}, "a", "ixscan", { a: -1 }, {}, 1, 0 );
   checkExplain( dbcl, {}, "", "tbscan", { a: -1 }, {}, 5000, 5000 );
}
