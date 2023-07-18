/***************************************************************************************************
 * @Description: change stream
 * @ATCaseID: changeStream_027
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
 *    change stream on collection
 **************************************************************************************************/

var selectedGroup = selectGroupWithSecondary( db ) ;
testConf.clOpt = { Group: selectedGroup, ReplSize: -1 } ;
testConf.clName = COMMCLNAME + "_change_stream_027" ;

mainLoop( wrapTest, testPara ) ;

function wrapTest( testPara )
{
   try
   {
      db.updateConf( { logwritemod: "full" } ) ;
      test( testPara ) ;
   }
   finally
   {
      db.deleteConf( {logwritemod:1} ) ;
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
   var dbCL = db.getCS( testConf.csName ).getCL( testConf.clName ) ;

   // insert
   dbCL.insert( document ) ;
   checkChangeStreamInsertResult( cur, testConf.csName, testConf.clName, document ) ;

   // update
   var documentKey = { _id:1 } ;
   var updateAction = { $set:{ a:2, b:3 } } ;
   dbCL.update( updateAction, documentKey ) ;
   checkChangeStreamUpdateResult( cur, testConf.csName, testConf.clName, documentKey, { $replace:{_id:1, a:2, b:3} } ) ;

   // update multiple keys
   updateAction = { $set:{ a:3, b:3 }, $inc:{ c:1 } } ;
   dbCL.update( updateAction, documentKey ) ;
   checkChangeStreamUpdateResult( cur, testConf.csName, testConf.clName, documentKey, {$replace: { _id:1, a:3, b:3, c:1 } } ) ;

   cur.close() ;
}
