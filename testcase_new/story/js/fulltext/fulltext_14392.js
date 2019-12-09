/************************************
*@Description: 全文检索与普通查询的3层or组合验证  
*@author:      liuxiaoxuan
*@createdate:  2019.11.01
*@testlinkCase: seqDB-14392
**************************************/
function main ()
{
   if( commIsStandalone( db ) ) { return; }

   //create CL
   var clName = COMMCLNAME + "_ES_14392";
   commDropCL( db, COMMCSNAME, clName, true, true );

   var dbcl = commCreateCL( db, COMMCSNAME, clName );

   var textIndexName = "textIndex_14392";
   dbcl.createIndex( textIndexName, { "a": "text" } );

   // insert
   var objs = new Array();
   for( var i = 0; i < 5000; i++ )
   {
      objs.push( { a: "test_14392_A " + i, b: i } );
      objs.push( { a: "test_14392_B " + i, b: i + 5000 } );
      objs.push( { a: "test_14392_C " + i, b: i + 10000 } );
      objs.push( { a: "test_14392_D " + i, b: i + 15000 } );
   }
   dbcl.insert( objs );

   checkFullSyncToES( COMMCSNAME, clName, textIndexName, 20000 );

   // match 0 record
   var findNoneConf1 = { "$or": [{ "$or": [{ "$or": [{ "b": { "$lt": 0 } }, { "": { "$Text": { "query": { "match": { "a": "test_14392" } } } } }] }] }, { "a": { "$isnull": 1 } }] }; //or-or-or
   var findNoneConf2 = { "$or": [{ "$or": [{ "$and": [{ "b": { "$gte": 0 } }, { "a": { "$isnull": 1 } }] }] }, { "": { "$Text": { "query": { "match": { "a": "test_14392" } } } } }] }; //or-or-and
   var findNoneConf3 = { "$or": [{ "$or": [{ "$not": [{ "b": { "$gte": 0 } }, { "": { "$Text": { "query": { "exists": { "field": "a" } } } } }] }] }, { "a": { "$exists": 0 } }] } //or-or-not
   var findNoneConf4 = { "$or": [{ "$and": [{ "$or": [{ "b": { "$lt": 0 } }, { "a": { "$isnull": 1 } }] }] }, { "": { "$Text": { "query": { "match": { "a": "test_14392" } } } } }] }; //or-and-or
   var findNoneConf5 = { "$or": [{ "$and": [{ "$not": [{ "b": { "$gte": 0 } }, { "a": { "$isnull": 0 } }] }] }, { "": { "$Text": { "query": { "match": { "a": "test_14392" } } } } }] }; //or-and-not
   var findNoneConf6 = { "$or": [{ "$and": [{ "$and": [{ "b": { "$gt": 10000 } }, { "": { "$Text": { "query": { "match": { "a": "test_14392_A" } } } } }] }] }, { "a": { "$isnull": 1 } }] }; //or-and-and
   var actResult1 = dbOpr.findFromCL( dbcl, findNoneConf1 );
   var actResult2 = dbOpr.findFromCL( dbcl, findNoneConf2 );
   var actResult3 = dbOpr.findFromCL( dbcl, findNoneConf3 );
   var actResult4 = dbOpr.findFromCL( dbcl, findNoneConf4 );
   var actResult5 = dbOpr.findFromCL( dbcl, findNoneConf5 );
   var actResult6 = dbOpr.findFromCL( dbcl, findNoneConf6 );
   var expResult = [];
   checkResult( expResult, actResult1 );
   println( "---match 0 record for $or-$or-$or---" );
   checkResult( expResult, actResult2 );
   println( "---match 0 record for $or-$or-$and---" );
   checkResult( expResult, actResult3 );
   println( "---match 0 record for $or-$or-$not---" );
   checkResult( expResult, actResult4 );
   println( "---match 0 record for $or-$and-$or---" );
   checkResult( expResult, actResult5 );
   println( "---match 0 record for $or-$and-$not---" );
   checkResult( expResult, actResult6 );
   println( "---match 0 record for $or-$and-$and---" );

   // match some records
   var findSomeConf1 = { "$or": [{ "$or": [{ "$or": [{ "b": { "$lt": 5000 } }, { "": { "$Text": { "query": { "match": { "a": "test_14392_B" } } } } }] }, { b: { "$et": 10000 } }] }, { "a": { "$exists": 0 } }] }; //or-or-or
   var findSomeConf2 = { "$or": [{ "$and": [{ "$not": [{ "b": { "$gte": 10000 } }, { "": { "$Text": { "query": { "match": { "a": "test_14392_D" } } } } }] }, { b: { "$lt": 15000 } }] }, { "a": { "$exists": 0 } }] }; //or-and-not
   var actResult1 = dbOpr.findFromCL( dbcl, findSomeConf1, { 'a': '' }, { _id: 1 } );
   var actResult2 = dbOpr.findFromCL( dbcl, findSomeConf2, { 'a': '' }, { _id: 1 } );
   var expResult1 = dbOpr.findFromCL( dbcl, { "b": { "$lte": 10000 } }, { 'a': '' }, { _id: 1 } );
   var expResult2 = dbOpr.findFromCL( dbcl, { "b": { "$lt": 15000 } }, { 'a': '' }, { _id: 1 } );
   checkResult( expResult1, actResult1 );
   println( "---match some records for $or-$or-$or---" );
   checkResult( expResult2, actResult2 );
   println( "---match some records for $or-$and-$not---" );

   // match all records
   var findAllConf1 = { "$or": [{ "$or": [{ "$or": [{ "b": { "$gte": 5000 } }, { "": { "$Text": { "query": { "match": { "a": "test_14392_A" } } } } }] }] }, { "a": { "$exists": 0 } }] }; //or-or-or
   var findAllConf2 = { "$or": [{ "$not": [{ "$and": [{ "b": { "$gt": 15000 } }, { "": { "$Text": { "query": { "match": { "a": "test_14392_C" } } } } }] }] }, { "a": { "$isnull": 1 } }] }; //or-not-and
   var actResult1 = dbOpr.findFromCL( dbcl, findAllConf1, { 'a': '' }, { _id: 1 } );
   var actResult2 = dbOpr.findFromCL( dbcl, findAllConf2, { 'a': '' }, { _id: 1 } );
   var expResult = dbOpr.findFromCL( dbcl, null, { 'a': '' }, { _id: 1 } );
   checkResult( expResult, actResult1 );
   println( "---match all records for $or-$or-$or---" );
   checkResult( expResult, actResult2 );
   println( "---match all records for $or-$not-$and---" );

   var dbOperator = new DBOperator();
   var esIndexNames = dbOperator.getESIndexNames( COMMCSNAME, clName, textIndexName );
   commDropCL( db, COMMCSNAME, clName, true, true );
   //SEQUOIADBMAINSTREAM-3983
   checkIndexNotExistInES( esIndexNames );
}
try
{
   main();
}
catch( e )
{
   if( e.constructor === Error )
   {
      println( e.stack );
   }
   throw e;
}
;
