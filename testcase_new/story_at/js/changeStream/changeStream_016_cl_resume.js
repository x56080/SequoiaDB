/***************************************************************************************************
 * @Description: change stream
 * @ATCaseID: changeStream_016
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
 *    change stream resume on collection in different node
 **************************************************************************************************/

var selectedGroup = selectGroupWithSecondary( db ) ;
testConf.clOpt = { Group: selectedGroup, ReplSize: -1 } ;
testConf.clName = COMMCLNAME + "_change_stream_016" ;

mainLoop( test, testPara ) ;

function test( testPara )
{
   // select node
   var nodeStartDB ;
   var nodeResumeDB ;
   if ( testPara.usePrimary )
   {
      nodeStartDB = selectPrimaryForChangeStream( db, testConf.csName, testConf.clName ) ;
      nodeResumeDB = selectSecondaryForChangeStream( db, testConf.csName, testConf.clName ) ;
   }
   else
   {
      nodeStartDB = selectSecondaryForChangeStream( db, testConf.csName, testConf.clName ) ;
      nodeResumeDB = selectPrimaryForChangeStream( db, testConf.csName, testConf.clName ) ;
   }
   var nodeStartCL = nodeStartDB.getCS( testConf.csName ).getCL( testConf.clName ) ;
   var nodeResumeCL = nodeResumeDB.getCS( testConf.csName ).getCL( testConf.clName ) ;

   // start watch
   var cur = nodeStartCL.watch( StreamToken(), { MaxWaitTime:0 } ) ;
   var document = { _id:1, a:1, b:1 } ;
   var dbCL = db.getCS( testConf.csName ).getCL( testConf.clName ) ;

   // insert
   dbCL.insert( document ) ;
   var token = checkChangeStreamInsertResult( cur, testConf.csName, testConf.clName, document ) ;

   // a batch of operations
   var documentKey = { _id:1 } ;
   var updateAction = { $set:{ a:2 } } ;
   dbCL.update( updateAction, documentKey ) ;
   dbCL.remove( documentKey ) ;
   dbCL.createIndex( 'a', {a:1} ) ;
   dbCL.dropIndex( 'a' ) ;
   dbCL.alter( { ShardingKey:{ a:1 }, AutoSplit: false } ) ;

   // close change stream
   cur.close() ;

   // resume change stream from another node
   cur = nodeResumeCL.watch( StreamToken( token ), { MaxWaitTime:0 } ) ;

   // check result
   checkChangeStreamUpdateResult( cur, testConf.csName, testConf.clName, documentKey, updateAction ) ;
   checkChangeStreamDeleteResult( cur, testConf.csName, testConf.clName, documentKey ) ;
   checkChangeStreamResult( cur, testConf.csName, testConf.clName, "createix" ) ;
   checkChangeStreamResult( cur, testConf.csName, testConf.clName, "deleteix" ) ;
   checkChangeStreamResult( cur, testConf.csName, testConf.clName, "invalidatecata" ) ;
   checkChangeStreamResult( cur, testConf.csName, testConf.clName, "alter" ) ;

   cur.close() ;
}
