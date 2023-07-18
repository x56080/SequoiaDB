/***************************************************************************************************
 * @Description: change stream
 * @ATCaseID: changeStream_005
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
 *    change stream not catch up
 **************************************************************************************************/

var selectedGroup = selectGroupWithSecondary( db ) ;
testConf.clOpt = { Group: selectedGroup, ReplSize: -1 } ;
testConf.clName = COMMCLNAME + "_change_stream_005" ;

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
   var cur = nodeCL.watch( StreamToken(), { MaxWaitTime:0, CacheSize:0 } ) ;

   // prepare data
   var array = new Array( 10000000 ) ;
   var string = array.join('a') ;
   var dbCS = db.getCS( testConf.csName ) ;
   var dbCL = dbCS.getCL( testConf.clName ) ;
   for ( var i = 0 ; i < 25 ; ++ i )
   {
      dbCL.insert( { a:i, b:string } ) ;
   }

   // check change stream error result
   checkChangeStreamErrorResult( cur, true, SDB_STREAM_NOT_CATCHUP ) ;

   // check change stream closed
   checkChangeStreamClosed( cur ) ;

   cur.close() ;
}
