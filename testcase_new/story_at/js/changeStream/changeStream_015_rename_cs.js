/***************************************************************************************************
 * @Description: change stream
 * @ATCaseID: changeStream_015
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
 *    change stream with rename collection space
 **************************************************************************************************/

var selectedGroup = selectGroupWithSecondary( db ) ;
testConf.clOpt = { Group: selectedGroup, ReplSize: -1 } ;
testConf.clName = COMMCLNAME + "_change_stream_015" ;

var newCSName = testConf.csName + "_new" ;

mainLoop( wrapTest, testPara ) ;

function wrapTest( testPara )
{
   try
   {
      commDropCS( db, newCSName, true ) ;
      test( testPara ) ;
   }
   finally
   {
      commDropCS( db, newCSName, true ) ;
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

   // insert and rename collection space
   dbCL.insert( document ) ;
   db.renameCS( testConf.csName, newCSName ) ;

   // check result
   checkChangeStreamInsertResult( cur, testConf.csName, testConf.clName, document ) ;
   checkChangeStreamErrorResult( cur, true, SDB_DMS_CS_NOTEXIST, "Collection space [" + testConf.csName + "] has been renamed to [" + newCSName + "]" ) ;
   checkChangeStreamClosed( cur ) ;

   cur.close() ;

   db.dropCS( newCSName ) ;
}
