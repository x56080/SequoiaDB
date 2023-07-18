/***************************************************************************************************
 * @Description: change stream
 * @ATCaseID: changeStream_025
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
 *    change stream with transaction
 **************************************************************************************************/

var selectedGroup = selectGroupWithSecondary( db ) ;
testConf.clOpt = { Group: selectedGroup, ReplSize: -1 } ;
testConf.clName = COMMCLNAME + "_change_stream_025" ;
var clNameA = testConf.clName + "_A" ;

mainLoop( wrapTest, testPara ) ;

function wrapTest( testPara )
{
   try
   {
      commDropCL( db, testConf.csName, clNameA, true, true ) ;
      test( testPara ) ;
   }
   finally
   {
      commDropCL( db, testConf.csName, clNameA, true, true ) ;
   }
}

function test( testPara )
{
   // select node
   var nodeDB ;
   if ( testPara.usePrimary )
   {
      nodeDB = selectPrimaryForChangeStream( db, testConf.csName, testConf.clName ) ;
   }
   else
   {
      nodeDB = selectSecondaryForChangeStream( db, testConf.csName, testConf.clName ) ;
   }
   var nodeCL = nodeDB.getCS( testConf.csName ).getCL( testConf.clName ) ;

   var dbCS = db.getCS( testConf.csName ) ;
   var dbCL = dbCS.getCL( testConf.clName ) ;
   var dbCLA = dbCS.createCL( clNameA, testConf.clOpt ) ;

   // start watch
   var cur = nodeCL.watch( StreamToken(), { MaxWaitTime:0, ChangeTypes:"record|trans" } ) ;

   var document = { _id:1, a:1, b:1 } ;
   var documentKey = { _id:1 } ;
   var updateAction = { $set:{ a:2 } } ;

   // non-transaction
   dbCL.insert( document ) ;
   dbCL.update( updateAction, documentKey ) ;
   dbCL.remove( documentKey ) ;

   // transaction on different collection
   db.transBegin() ;
   dbCLA.insert( document ) ;
   dbCLA.update( updateAction, documentKey ) ;
   dbCLA.remove( documentKey ) ;
   db.transCommit() ;

   // transaction
   db.transBegin() ;
   dbCL.insert( document ) ;
   dbCL.update( updateAction, documentKey ) ;
   dbCL.remove( documentKey ) ;
   db.transCommit() ;

   // transaction rollback
   db.transBegin() ;
   dbCL.insert( document ) ;
   dbCL.update( updateAction, documentKey ) ;
   dbCL.remove( documentKey ) ;
   db.transRollback() ;

   // check result
   checkChangeStreamInsertResult( cur, testConf.csName, testConf.clName, document ) ;
   checkChangeStreamUpdateResult( cur, testConf.csName, testConf.clName, documentKey, updateAction ) ;
   checkChangeStreamDeleteResult( cur, testConf.csName, testConf.clName, documentKey ) ;
   checkChangeStreamInsertResult( cur, testConf.csName, testConf.clName, document, undefined, true ) ;
   checkChangeStreamUpdateResult( cur, testConf.csName, testConf.clName, documentKey, updateAction, undefined, true ) ;
   checkChangeStreamDeleteResult( cur, testConf.csName, testConf.clName, documentKey, undefined, true ) ;
   checkChangeStreamResult( cur, undefined, undefined, "commit" ) ;
   checkChangeStreamResult( cur, undefined, undefined, "commit" ) ;

   checkChangeStreamInsertResult( cur, testConf.csName, testConf.clName, document, undefined, true ) ;
   checkChangeStreamUpdateResult( cur, testConf.csName, testConf.clName, documentKey, updateAction, undefined, true ) ;
   checkChangeStreamDeleteResult( cur, testConf.csName, testConf.clName, documentKey, undefined, true ) ;

   checkChangeStreamInsertResult( cur, testConf.csName, testConf.clName, { _id:1, a:2, b:1 }, undefined, true, true ) ;
   checkChangeStreamUpdateResult( cur, testConf.csName, testConf.clName, documentKey, { $set: {a:1} }, undefined, true, true ) ;
   checkChangeStreamDeleteResult( cur, testConf.csName, testConf.clName, documentKey, undefined, true, true ) ;

   cur.close() ;
}
