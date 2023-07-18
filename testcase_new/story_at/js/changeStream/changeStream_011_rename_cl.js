/***************************************************************************************************
 * @Description: change stream
 * @ATCaseID: changeStream_011
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
 *    change stream with rename collection
 **************************************************************************************************/

var selectedGroup = selectGroupWithSecondary( db ) ;
testConf.clOpt = { Group: selectedGroup, ReplSize: -1 } ;
testConf.clName = COMMCLNAME + "_change_stream_011" ;
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
   var nodeCL = nodeDB.getCS( testConf.csName ).getCL( testConf.clName ) ;

   // start watch
   var cur = nodeCL.watch( StreamToken(), { MaxWaitTime:0 } ) ;
   var document = { _id:1, a:1, b:1 } ;
   var dbCS = db.getCS( testConf.csName ) ;
   var dbCL = dbCS.getCL( testConf.clName ) ;

   // insert and rename
   dbCL.insert( document ) ;
   dbCS.renameCL( testConf.clName, newCLName ) ;

   // check result
   checkChangeStreamInsertResult( cur, testConf.csName, testConf.clName, document ) ;
   checkChangeStreamErrorResult( cur, true, SDB_DMS_NOTEXIST, "Collection [" + testConf.csName + "." + testConf.clName + "] has been renamed to [" + testConf.csName + "." + newCLName + "]" ) ;
   checkChangeStreamClosed( cur ) ;

   cur.close() ;
}
