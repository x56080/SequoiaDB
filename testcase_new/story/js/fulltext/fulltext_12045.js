/******************************************************************************
 * @Description   : seqDB-12045:带from/size进行全文检索
 * @Author        : liuxiaoxuan 
 * @CreateTime    : 2018.10.10
 * @LastEditTime  : 2023.05.16
 * @LastEditors   : wu yan
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.clName = COMMCLNAME + "_es_12045";

main( test );

function test ()
{
   var dbcl = testPara.testCL;
   var clName = testConf.clName;
   var textIndexName = "textIndex_12045";
   dbcl.createIndex( textIndexName, { "a": "text" } );

   // insert
   var objs = new Array();
   for( var i = 0; i < 10001; i++ )
   {
      objs.push( { a: "test_12045_" + i } );
   }
   dbcl.insert( objs );

   checkFullSyncToES( COMMCSNAME, clName, textIndexName, 10001 );

   var esOpr = new ESOperator();
   var dbOpr = new DBOperator();
   var esIndexNames = dbOpr.getESIndexNames( COMMCSNAME, clName, textIndexName );

   // from 
   var findCond = { "": { "$Text": { "query": { "match_all": {} }, "from": 9990 } } };
   var searchCond = '{"query":{"match_all":{}}, "from": 9990}'
   var actResult = dbOpr.findFromCL( dbcl, findCond, { "a": { "$include": 1 } } );
   var expResult = esOpr.findFromES( esIndexNames[0], searchCond );
   actResult.sort( compare( "a" ) );
   expResult.sort( compare( "a" ) );
   checkResult( expResult, actResult );

   // size
   var findCond = { "": { "$Text": { "query": { "match_all": {} }, "size": 10000 } } };
   var searchCond = '{"query":{"match_all":{}}, "size": 10000}'
   var actResult = dbOpr.findFromCL( dbcl, findCond, { "a": { "$include": 1 } } );
   var expResult = esOpr.findFromES( esIndexNames[0], searchCond );
   actResult.sort( compare( "a" ) );
   expResult.sort( compare( "a" ) );
   checkResult( expResult, actResult );

   // from+size < 10000
   var findCond = { "": { "$Text": { "query": { "match_all": {} }, "from": 1, "size": 9990 } } };
   var searchCond = '{"query":{"match_all":{}}, "from": 1, "size": 9990}'
   var actResult = dbOpr.findFromCL( dbcl, findCond, { "a": { "$include": 1 } } );
   var expResult = esOpr.findFromES( esIndexNames[0], searchCond );
   actResult.sort( compare( "a" ) );
   expResult.sort( compare( "a" ) );
   checkResult( expResult, actResult );

   // from+size > 10000, should fail  
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      var rec = dbcl.find( { "": { "$Text": { "query": { "match_all": {} }, "from": 0, "size": 10001 } } } );
      rec.next();
   } );
}
