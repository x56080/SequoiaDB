/***************************************************************************************************
 * @Description: change stream
 * @ATCaseID: changeStream_009
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
 *    change stream watch collection from db
 **************************************************************************************************/

var selectedGroup = selectGroupWithSecondary( db ) ;
testConf.clOpt = { Group: selectedGroup, ReplSize: -1 } ;
testConf.clName = COMMCLNAME + "_change_stream_009" ;

var newCLName = testConf.clName + "_new" ;

mainLoop( wrapTest, testPara ) ;

function wrapTest( testPara )
{
   try
   {
      commDropCL( db, testConf.csName, newCLName, true, true ) ;
      test( testPara ) ;
   }
   finally
   {
      commDropCL( db, testConf.csName, newCLName, true, true ) ;
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

   // start watch
   var options = { Collections: [ testConf.csName + "." + testConf.clName ],  MaxWaitTime:0 } ;
   var cur = nodeDB.watch( StreamToken(), options ) ;
   var document = { _id:1, a:1, b:1 } ;
   var dbCS = db.getCS( testConf.csName ) ;
   var dbCL = dbCS.getCL( testConf.clName ) ;

   // insert
   dbCL.insert( document ) ;
   checkChangeStreamInsertResult( cur, testConf.csName, testConf.clName, document ) ;

   // update
   var documentKey = { _id:1 } ;
   var updateAction = { $set:{ a:2 } } ;
   dbCL.update( updateAction, documentKey ) ;
   checkChangeStreamUpdateResult( cur, testConf.csName, testConf.clName, documentKey, updateAction ) ;

   // delete
   dbCL.remove( documentKey ) ;
   checkChangeStreamDeleteResult( cur, testConf.csName, testConf.clName, documentKey ) ;

   // rename collection
   dbCS.renameCL( testConf.clName, newCLName ) ;
   checkChangeStreamResult( cur, testConf.csName, testConf.clName, "invalidatecata" ) ;
   var token = checkChangeStreamErrorResult( cur, false, SDB_DMS_NOTEXIST ) ;
   checkChangeStreamClosed( cur ) ;

   // resume
   options = { Collections: [ testConf.csName + "." + newCLName ],  MaxWaitTime:0 } ;
   cur = nodeDB.watch( StreamToken( token ), options ) ;

   // isnert
   dbCL = dbCS.getCL( newCLName ) ;
   dbCL.insert( document ) ;
   checkChangeStreamInsertResult( cur, testConf.csName, newCLName, document ) ;

   // drop collection
   dbCS.dropCL( newCLName ) ;
   checkChangeStreamResult( cur, testConf.csName, newCLName, "invalidatecata" ) ;
   checkChangeStreamErrorResult( cur, false, SDB_DMS_NOTEXIST ) ;
   checkChangeStreamClosed( cur ) ;
}
