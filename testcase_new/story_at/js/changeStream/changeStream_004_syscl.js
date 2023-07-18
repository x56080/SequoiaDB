/***************************************************************************************************
 * @Description: change stream
 * @ATCaseID: changeStream_004
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
 *    change stream on system collection
 **************************************************************************************************/

var selectedGroup = selectGroupWithSecondary( db ) ;
testConf.clOpt = { Group: selectedGroup, ReplSize: -1 } ;
testConf.clName = COMMCLNAME + "_change_stream_004" ;

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
   var nodeCL = nodeDB.getCS( "SYSSTAT" ).getCL( "SYSCOLLECTIONSTAT" ) ;

   // start watch
   var cur = nodeCL.watch( StreamToken(), { MaxWaitTime:0 } ) ;

   // prepare data
   var dbCL = db.getCS( testConf.csName ).getCL( testConf.clName ) ;
   data = [] ;
   for ( var i = 0 ; i < 10000 ; i++ )
   {
      data.push( { _id:i, a:i, b:i } ) ;
   }
   dbCL.insert( data ) ;

   // analyze
   db.analyze( { Collection: testConf.csName + "." + testConf.clName } ) ;

   // check insert statistics result for analyze
   checkChangeStreamInsertResult( cur, "SYSSTAT", "SYSCOLLECTIONSTAT" ) ;

   cur.close() ;
}
