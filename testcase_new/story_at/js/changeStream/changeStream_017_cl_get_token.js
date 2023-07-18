/***************************************************************************************************
 * @Description: change stream
 * @ATCaseID: changeStream_017
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
 *    change stream with get token
 **************************************************************************************************/

var selectedGroup = selectGroupWithSecondary( db ) ;
testConf.clOpt = { Group: selectedGroup, ReplSize: -1 } ;
testConf.clName = COMMCLNAME + "_change_stream_017" ;

var csName = COMMCSNAME + "_change_stream_017_new" ;

mainLoop( wrapTest, testPara ) ;

function wrapTest( testPara )
{
   try
   {
      commDropCS( db, csName, true ) ;
      test( testPara ) ;
   }
   finally
   {
      commDropCS( db, csName, true ) ;
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
   // get token
   var token = nodeDB.getChangeStreamToken() ;

   // a batch of operations
   var clName = testConf.clName ;
   db.createCS( csName ).createCL( clName, testConf.clOpt ) ;
   var nodeCL = nodeDB.getCS( csName ).getCL( clName ) ;
   var document = { _id:1, a:1, b:1 } ;
   var dbCL = db.getCS( csName ).getCL( clName ) ;
   dbCL.insert( document ) ;
   var documentKey = { _id:1 } ;
   var updateAction = { $set:{ a:2 } } ;
   dbCL.update( updateAction, documentKey ) ;
   dbCL.remove( documentKey ) ;
   dbCL.createIndex( 'a', {a:1} ) ;
   dbCL.dropIndex( 'a' ) ;
   dbCL.alter( { ShardingKey:{ a:1 }, AutoSplit: false } ) ;

   // watch from token
   var cur = nodeCL.watch( token, { MaxWaitTime:0 } ) ;

   // check results
   checkChangeStreamResult( cur, csName, undefined, "createcs" ) ;
   checkChangeStreamResult( cur, csName, clName, "createcl" ) ;
   checkChangeStreamInsertResult( cur, csName, clName, document ) ;
   checkChangeStreamUpdateResult( cur, csName, clName, documentKey, updateAction ) ;
   checkChangeStreamDeleteResult( cur, csName, clName, documentKey ) ;
   checkChangeStreamResult( cur, csName, clName, "createix" ) ;
   checkChangeStreamResult( cur, csName, clName, "deleteix" ) ;
   checkChangeStreamResult( cur, csName, clName, "invalidatecata" ) ;
   checkChangeStreamResult( cur, csName, clName, "alter" ) ;

   cur.close() ;

   db.dropCS( csName ) ;
}
