/***************************************************************************************************
 * @Description: change stream
 * @ATCaseID: changeStream_001
 * @Author: He Guoming
 * @TestlinkCase: 无（由测试人员维护，在测试阶段如果有测试场景引用本和例，则在此处填写 Testlink 用例编号，
 *                    并在 Testlink 系统中标记本用例文件名）
 * @Change Activity:
 * Date       Who            Description
 * ========== ========== =========================================================
 * 06/01/2023 He Guoming change stream
 **************************************************************************************************/

/*********************************************测试用例***********************************************
 * 环境准备：正常集群环境即可
 * 测试场景：
 *    change stream with different arguments
 **************************************************************************************************/

testConf.clOpt = { ReplSize: -1 } ;
testConf.clName = COMMCLNAME + "_change_stream_001" ;

mainLoop( test, testPara ) ;

function test( testPara )
{
   var nodeDB = selectPrimaryForChangeStream( db, testConf.csName, testConf.clName ) ;
   var nodeCL = nodeDB.getCS( testConf.csName ).getCL( testConf.clName ) ;

   // MasWatiTime should be number
   assert.tryThrow(SDB_INVALIDARG, function () {
      nodeDB.watch( StreamToken(), { MaxWaitTime: "aaa" } );
   });

   // CacheSize should be number
   assert.tryThrow(SDB_INVALIDARG, function () {
      nodeDB.watch( StreamToken(), { CacheSize: "aaa" } );
   });

   // check range of CacheSize
   assert.tryThrow(SDB_INVALIDARG, function () {
      nodeDB.watch( StreamToken(), { CacheSize: 2049 } );
   });

   // check range of CacheSize
   assert.tryThrow(SDB_INVALIDARG, function () {
      nodeDB.watch( StreamToken(), { CacheSize: -50000 } );
   });

   // check name of Collections
   assert.tryThrow(SDB_INVALIDARG, function () {
      nodeDB.watch( StreamToken(), { Collections: "aaa" } );
   });

   // check existence of CollectionSpaces
   assert.tryThrow(SDB_DMS_CS_NOTEXIST, function () {
      nodeDB.watch( StreamToken(), { CollectionSpaces: "aaa" } );
   });

   // check existence of Collections
   assert.tryThrow(SDB_DMS_CS_NOTEXIST, function () {
      nodeDB.watch( StreamToken(), { Collections: "aaa.aaa" } );
   });

   // check existence of Collections
   assert.tryThrow(SDB_DMS_NOTEXIST, function () {
      nodeDB.watch( StreamToken(), { Collections: testConf.csName + ".aaa" } );
   });

   // check ChangeTypes
   assert.tryThrow(SDB_INVALIDARG, function () {
      nodeDB.watch( StreamToken(), { ChangeTypes: "AAA" } );
   });

   // check token
   assert.tryThrow(SDB_INVALIDARG, function () {
      nodeDB.watch( StreamToken( "aaaa" ) );
   });

   // unknown argument
   assert.tryThrow(SDB_INVALIDARG, function () {
      nodeDB.watch( StreamToken(), { AAA: "AAA" } );
   });

   // check ChangeTypes: case-insensitive
   var cur = nodeDB.watch( StreamToken(), { ChangeTypes: "DDL", MaxWaitTime: 0 } );
   cur.close() ;

   var cur = nodeDB.watch( StreamToken(), { ChangeTypes: "ddl|Record", MaxWaitTime: 0 } );
   cur.close() ;
}
