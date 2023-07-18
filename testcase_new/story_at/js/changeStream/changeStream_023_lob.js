/***************************************************************************************************
 * @Description: change stream
 * @ATCaseID: changeStream_023
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
 *    change stream with LOB operations
 **************************************************************************************************/

var selectedGroup = selectGroupWithSecondary( db ) ;
testConf.clOpt = { Group: selectedGroup, ReplSize: -1 } ;
testConf.clName = COMMCLNAME + "_change_stream_023" ;

mainLoop( test, testPara ) ;

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

   // not support yet
   assert.tryThrow(SDB_OPTION_NOT_SUPPORT, function () {
      nodeCL.watch( StreamToken(), { MaxWaitTime:0, ChangeTypes:"lob" } ) ;
   });

   return ;

   // start watch
   var cur = nodeCL.watch( StreamToken(), { MaxWaitTime:0, ChangeTypes:"lob" } ) ;
   var dbCS = db.getCS( testConf.csName ) ;
   var dbCL = dbCS.getCL( testConf.clName ) ;

   // put LOB
   var lobID = dbCL._putLobValue( "aaa" ) ;
   checkChangeStreamLobResult( cur, testConf.csName, testConf.clName, lobID, "lobwrite" ) ;

   // update LOB
   dbCL.truncateLob( lobID, 0 ) ;
   checkChangeStreamLobResult( cur, testConf.csName, testConf.clName, lobID, "lobupdate" ) ;

   // remove LOB
   dbCL.deleteLob( lobID ) ;
   checkChangeStreamLobResult( cur, testConf.csName, testConf.clName, lobID, "lobremove" ) ;

   cur.close() ;
}
