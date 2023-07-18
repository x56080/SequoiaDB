/***************************************************************************************************
 * @Description: change stream with insert, update, delete
 * @ATCaseID: changeStream_018
 * @Author: He Guoming
 * @TestlinkCase: 无（由测试人员维护，在测试阶段如果有测试场景引用本和例，则在此处填写 Testlink 用例编号，
 *                    并在 Testlink 系统中标记本用例文件名）
 * @Change Activity:
 * Date       Who            Description
 * ========== ========== =========================================================
 * 06/01/2023 He Guoming change stream with insert, update, delete
 **************************************************************************************************/

/*********************************************测试用例***********************************************
 * 环境准备：正常集群环境即可
 * 测试场景：
 *    change stream snapshot
 **************************************************************************************************/

testConf.clOpt = { ReplSize: -1 } ;
testConf.clName = COMMCLNAME + "_change_stream_018" ;

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
   var cur = nodeCL.watch( StreamToken(), { MaxWaitTime:0 } ) ;
   var document = { _id:1, a:1, b:1 } ;
   var dbCL = db.getCS( testConf.csName ).getCL( testConf.clName ) ;

   // insert
   dbCL.insert( document ) ;
   checkChangeStreamInsertResult( cur, testConf.csName, testConf.clName, document ) ;

   // snapshot
   var snapshotCur = db.snapshot( SDB_SNAP_STREAMS, { "Options.Collections": testConf.csName + "." + testConf.clName } ) ;
   assert.equal( snapshotCur.size(), 1, "expected snapshot number is different" ) ;
   snapshotCur.close() ;

   // snapshot from SQL
   var snapshotCur = db.exec( 'select * from $SNAPSHOT_STREAMS AS A where A.Options.Collections = "' + testConf.csName + "." + testConf.clName + '"' ) ;
   assert.equal( snapshotCur.size(), 1, "expected snapshot number is different" ) ;
   snapshotCur.close() ;

   cur.close() ;
}
