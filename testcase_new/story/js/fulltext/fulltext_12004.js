/******************************************************************************
 * @Description   : seqDB-12004:单键全文索引插入/更新/删除记录
 * @Author        : zhaoyu 
 * @CreateTime    : 2018.9.28
 * @LastEditTime  : 2023.05.11
 * @LastEditors   : wu yan
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.clName = COMMCLNAME + "_es_12004";
main( test );

function test ()
{
   var dbcl = testPara.testCL;
   var clName = testConf.clName;
   var indexName = "a_12004";
   var doc = [{ a: "string1", no: 1 },
   { a: "string2", b: 1, no: 2 },
   { b: 2, no: 3 }];
   dbcl.insert( doc );
   commCreateIndex( dbcl, indexName, { a: "text" } );
   dbcl.insert( doc );

   //all of record sync to ES
   var dbOperator = new DBOperator();
   checkFullSyncToES( COMMCSNAME, clName, indexName, 4 );

   var expectRecords = dbOperator.findFromCL( dbcl, { a: { $type: 2, $et: "string" } }, null, { _id: 1 } );
   var actRecords = dbOperator.findFromCL( dbcl, { "": { "$Text": { query: { match_all: {} } } } }, null, { _id: 1 } );
   checkResult( expectRecords, actRecords );

   //string update to string,sync ES
   dbcl.update( { $set: { a: "update" } }, { a: "string1" } );
   dbcl.insert( { a: "string3" } );
   checkFullSyncToES( COMMCSNAME, clName, indexName, 5 );
   var actRecords = dbOperator.findFromCL( dbcl, { "": { "$Text": { query: { match: { a: "update" } } } } }, null, { _id: 1 } );
   var expectRecords = dbOperator.findFromCL( dbcl, { a: "update" }, null, { _id: 1 } );
   checkResult( expectRecords, actRecords );

   //string update to int,sync ES
   dbcl.update( { $set: { a: 1 } }, { a: "update" } );
   checkFullSyncToES( COMMCSNAME, clName, indexName, 5 );
   var expectRecords = dbOperator.findFromCL( dbcl, { $or: [{ a: { $type: 2, $et: "int32" } }, { a: { $type: 2, $et: "string" } }] }, null, { _id: 1 } );
   var actRecords = dbOperator.findFromCL( dbcl, { "": { "$Text": { query: { match_all: {} } } } }, null, { _id: 1 } );
   checkResult( expectRecords, actRecords );

   //int update to int, sync ES
   dbcl.update( { $set: { a: 100 } }, { a: 1 } );
   checkFullSyncToES( COMMCSNAME, clName, indexName, 5 );
   var expectRecords = dbOperator.findFromCL( dbcl, { $or: [{ a: { $type: 2, $et: "int32" } }, { a: { $type: 2, $et: "string" } }] }, null, { _id: 1 } );
   var actRecords = dbOperator.findFromCL( dbcl, { "": { "$Text": { query: { match_all: {} } } } }, null, { _id: 1 } );
   checkResult( expectRecords, actRecords );

   //int update to string,sync ES
   dbcl.update( { $set: { a: "update" } }, { a: 100 } );
   checkFullSyncToES( COMMCSNAME, clName, indexName, 5 );
   var expectRecords = dbOperator.findFromCL( dbcl, { a: "update" }, null, { _id: 1 } );
   var actRecords = dbOperator.findFromCL( dbcl, { "": { "$Text": { query: { match: { a: "update" } } } } }, null, { _id: 1 } );
   checkResult( expectRecords, actRecords );

   dbcl.remove();
   checkFullSyncToES( COMMCSNAME, clName, indexName, 0 );
   var expectRecords = dbOperator.findFromCL( dbcl );
   var actRecords = dbOperator.findFromCL( dbcl, { "": { "$Text": { query: { match_all: {} } } } } );
   checkResult( expectRecords, actRecords );

}
