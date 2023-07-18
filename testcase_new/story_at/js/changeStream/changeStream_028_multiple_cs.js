/***************************************************************************************************
 * @Description: change stream
 * @ATCaseID: changeStream_028
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
 *    change stream on multiple collections and multiple collection spaces
 **************************************************************************************************/

var selectedGroup = selectGroupWithSecondary( db ) ;
testConf.clOpt = { Group: selectedGroup, ReplSize: -1 } ;
testConf.clName = COMMCLNAME + "_change_stream_028" ;

var clNameA = testConf.clName + "_A" ;
var csNameB = COMMCSNAME + "_change_stream_028_B" ;
var clNameB = testConf.clName + "_B" ;
var csNameC = COMMCSNAME + "_change_stream_028_C" ;
var clNameC1 = testConf.clName + "_C1" ;
var clNameC2 = testConf.clName + "_C2" ;

mainLoop( wrapTest, testPara ) ;

function wrapTest( testPara )
{
   try
   {
      commDropCL( db, testConf.csName, clNameA, true, true ) ;
      commDropCS( db, csNameB, true ) ;
      commDropCS( db, csNameC, true ) ;
      test( testPara ) ;
   }
   finally
   {
      commDropCL( db, testConf.csName, clNameA, true, true ) ;
      commDropCS( db, csNameB, true ) ;
      commDropCS( db, csNameC, true ) ;
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

   // prepare data
   var dbCS = db.getCS( testConf.csName ) ;
   var dbCL = dbCS.getCL( testConf.clName ) ;

   var dbCLA = dbCS.createCL( clNameA, testConf.clOpt ) ;

   var dbCSB = db.createCS( csNameB ) ;
   var dbCLB = dbCSB.createCL( clNameB, testConf.clOpt ) ;

   var dbCSC = db.createCS( csNameC ) ;
   var dbCLC1 = dbCSC.createCL( clNameC1, testConf.clOpt ) ;
   var dbCLC2 = dbCSC.createCL( clNameC2, testConf.clOpt ) ;

   // start watch
   var options = { Collections: [ testConf.csName + "." + testConf.clName, csNameB + "." + clNameB ], CollectionSpaces: [ csNameC, csNameB ], MaxWaitTime:0 }
   var cur = nodeDB.watch( StreamToken(), options ) ;

   // a batch of operations on different collections
   var document = { _id:1, a:1, b:1 } ;
   var documentKey = { _id:1 } ;
   var updateAction = { $set:{ a:2 } } ;
   dbCLA.insert( document ) ;
   dbCLA.update( updateAction, documentKey ) ;
   dbCLA.remove( documentKey ) ;
   dbCL.insert( document ) ;
   dbCLB.insert( document ) ;
   dbCL.update( updateAction, documentKey ) ;
   dbCLB.update( updateAction, documentKey ) ;
   dbCL.remove( documentKey ) ;
   dbCLB.remove( documentKey ) ;

   dbCLC1.insert( document ) ;
   dbCLC2.insert( document ) ;

   dbCSC.renameCL( clNameC1, clNameC1 + "_new" ) ;
   dbCSB.renameCL( clNameB, clNameB + "_new" ) ;

   // check results from different collections
   checkChangeStreamInsertResult( cur, testConf.csName, testConf.clName, document ) ;
   checkChangeStreamInsertResult( cur, csNameB, clNameB, document ) ;
   checkChangeStreamUpdateResult( cur, testConf.csName, testConf.clName, documentKey, updateAction ) ;
   checkChangeStreamUpdateResult( cur, csNameB, clNameB, documentKey, updateAction ) ;
   checkChangeStreamDeleteResult( cur, testConf.csName, testConf.clName, documentKey ) ;
   checkChangeStreamDeleteResult( cur, csNameB, clNameB, documentKey ) ;

   checkChangeStreamInsertResult( cur, csNameC, clNameC1, document ) ;
   checkChangeStreamInsertResult( cur, csNameC, clNameC2, document ) ;

   checkChangeStreamResult( cur, csNameC, clNameC1, "invalidatecata" ) ;
   checkChangeStreamResult( cur, csNameC, clNameC1, "renamecl" ) ;

   checkChangeStreamErrorResult( cur, true, SDB_DMS_NOTEXIST, "Collection [" + csNameB + "." + clNameB + "] has been renamed to [" + csNameB + "." + clNameB + "_new]" ) ;
   checkChangeStreamClosed( cur ) ;

   db.dropCS( csNameB ) ;
   db.dropCS( csNameC ) ;
}
