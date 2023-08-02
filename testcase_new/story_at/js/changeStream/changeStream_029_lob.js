/***************************************************************************************************
 * @Description: change stream
 * @ATCaseID: changeStream_029
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
 *    change stream with LOB operations with mutiple sequences
 **************************************************************************************************/

var selectedGroup = selectGroupWithSecondary( db ) ;
testConf.csOpt = { LobPageSize: 262144 } ;
testConf.clOpt = { Group: selectedGroup, ReplSize: -1 } ;
testConf.clName = COMMCLNAME + "_change_stream_029" ;

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

   // start watch
   var cur = nodeCL.watch( StreamToken(), { MaxWaitTime:0, ChangeTypes:"lob" } ) ;
   var dbCS = db.getCS( testConf.csName ) ;
   var dbCL = dbCS.getCL( testConf.clName ) ;

   var arrayValue = new Array(262144)
   var stringValue = arrayValue.join('a')

   // put LOB
   var lobID = dbCL._putLobValue( stringValue ) ;

   // sequence 1
   var expectDesc = { Sequence: 1,
                      PageSize: 262144,
                      PageOffset: 0,
                      FileOffset: 262144 - 1024,
                      Length: 1024 } ;
   checkChangeStreamLobResult( cur, testConf.csName, testConf.clName, lobID, "lobwrite", expectDesc ) ;

   // sequence 0
   var expectDesc = { Sequence: 0,
                      PageSize: 262144,
                      PageOffset: 1024,
                      FileOffset: 0,
                      Length: 262144 - 1024 } ;
   checkChangeStreamLobResult( cur, testConf.csName, testConf.clName, lobID, "lobwrite", expectDesc ) ;

   // update LOB
   dbCL.truncateLob( lobID, 0 ) ;

   // sequence 1 is removed
   var expectDesc = { Sequence: 1,
                      PageSize: 262144,
                      PageOffset: 0,
                      FileOffset: 262144 - 1024 } ;
   checkChangeStreamLobResult( cur, testConf.csName, testConf.clName, lobID, "lobremove" ) ;

   // sequence 0 is updated
   var expectDesc = { Sequence: 0, PageSize: 262144 } ;
   checkChangeStreamLobResult( cur, testConf.csName, testConf.clName, lobID, "lobupdate", expectDesc ) ;

   // remove LOB
   dbCL.deleteLob( lobID ) ;

   // sequence 0 is removed
   var expectDesc = { Sequence: 0,
                      PageSize: 262144,
                      PageOffset: 1024,
                      FileOffset: 0 } ;
   checkChangeStreamLobResult( cur, testConf.csName, testConf.clName, lobID, "lobremove" ) ;

   cur.close() ;
}
