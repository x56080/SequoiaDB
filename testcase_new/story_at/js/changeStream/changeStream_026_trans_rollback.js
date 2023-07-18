/***************************************************************************************************
 * @Description: change stream
 * @ATCaseID: changeStream_026
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
 *    change stream with transaction rollback
 **************************************************************************************************/

var selectedGroup = selectGroupWithSecondary( db ) ;
testConf.clOpt = { Group: selectedGroup, ReplSize: -1 } ;
testConf.clName = COMMCLNAME + "_change_stream_026" ;

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

   var dbA = new Sdb( COORDHOSTNAME, COORDSVCNAME );
   var dbCS = db.getCS( testConf.csName ) ;
   var dbCL = dbCS.getCL( testConf.clName ) ;

   // start watch
   var cur = nodeCL.watch( StreamToken(), { MaxWaitTime:0, ChangeTypes:"record|trans" } ) ;

   // transaction insert and remove the same record
   db.transBegin() ;
   dbCL.insert( { _id:1, a: 1, b: 1 } ) ;
   dbCL.remove( { _id: 1 } ) ;

   // another transaction insert the same record
   dbA.transBegin() ;
   dbA.getCS( testConf.csName ).getCL( testConf.clName ).insert( { _id: 1, a: 2, b: 2 } ) ;
   dbA.transCommit() ;

   // rollback the first transaction
   db.transRollback() ;

   // check result, a rollback record is generated
   checkChangeStreamInsertResult( cur, testConf.csName, testConf.clName, { _id: 1, a: 1, b: 1}, undefined, true ) ;
   checkChangeStreamDeleteResult( cur, testConf.csName, testConf.clName, { _id: 1 }, undefined, true ) ;
   checkChangeStreamInsertResult( cur, testConf.csName, testConf.clName, { _id: 1, a: 2, b: 2}, undefined, true ) ;
   checkChangeStreamResult( cur, undefined, undefined, "commit" ) ;
   checkChangeStreamResult( cur, undefined, undefined, "commit" ) ;
   checkChangeStreamResult( cur, undefined, undefined, "rollback" ) ;

   cur.close() ;
   dbA.close() ;
}
