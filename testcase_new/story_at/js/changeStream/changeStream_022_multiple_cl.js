/***************************************************************************************************
 * @Description: change stream
 * @ATCaseID: changeStream_022
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
 *    change stream on multiple collections
 **************************************************************************************************/

var selectedGroup = selectGroupWithSecondary( db ) ;
testConf.clOpt = { Group: selectedGroup, ReplSize: -1 } ;
testConf.clName = COMMCLNAME + "_change_stream_022" ;

var clNameA = testConf.clName + "_A" ;
var csNameB = testConf.csName + "_B" ;
var clNameB = testConf.clName + "_B" ;

mainLoop( wrapTest, testPara ) ;

function wrapTest( testPara )
{
   try
   {
      commDropCL( db, testConf.csName, clNameA, true, true ) ;
      commDropCS( db, csNameB, true ) ;
      test( testPara ) ;
   }
   finally
   {
      commDropCL( db, testConf.csName, clNameA, true, true ) ;
      commDropCS( db, csNameB, true ) ;
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

   // start watch
   var options = { Collections: [ testConf.csName + "." + testConf.clName, csNameB + "." + clNameB ], MaxWaitTime:0 }
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

   // check results from different collections
   checkChangeStreamInsertResult( cur, testConf.csName, testConf.clName, document ) ;
   checkChangeStreamInsertResult( cur, csNameB, clNameB, document ) ;
   checkChangeStreamUpdateResult( cur, testConf.csName, testConf.clName, documentKey, updateAction ) ;
   checkChangeStreamUpdateResult( cur, csNameB, clNameB, documentKey, updateAction ) ;
   checkChangeStreamDeleteResult( cur, testConf.csName, testConf.clName, documentKey ) ;
   checkChangeStreamDeleteResult( cur, csNameB, clNameB, documentKey ) ;

   cur.close() ;

   db.dropCS( csNameB ) ;
}
