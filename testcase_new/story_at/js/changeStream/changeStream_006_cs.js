/***************************************************************************************************
 * @Description: change stream
 * @ATCaseID: changeStream_006
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
 *    change stream on collection space
 **************************************************************************************************/

var selectedGroup = selectGroupWithSecondary( db ) ;
testConf.clOpt = { Group: selectedGroup, ReplSize: -1 } ;
testConf.clName = COMMCLNAME + "_change_stream_006" ;

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
   var nodeCS = nodeDB.getCS( testConf.csName ) ;

   // start watch
   var cur = nodeCS.watch( StreamToken(), { MaxWaitTime:0 } ) ;
   var document = { _id:1, a:1, b:1 } ;
   var dbCS = db.getCS( testConf.csName ) ;
   var dbCL = dbCS.getCL( testConf.clName ) ;

   // insert
   dbCL.insert( document ) ;
   checkChangeStreamInsertResult( cur, testConf.csName, testConf.clName, document ) ;

   // update
   var documentKey = { _id:1 } ;
   var updateAction = { $set:{ a:2 } } ;
   dbCL.update( updateAction, documentKey ) ;
   checkChangeStreamUpdateResult( cur, testConf.csName, testConf.clName, documentKey, updateAction ) ;

   // delete
   dbCL.remove( documentKey ) ;
   checkChangeStreamDeleteResult( cur, testConf.csName, testConf.clName, documentKey ) ;

   // create index
   dbCL.createIndex( 'a', {a:1} ) ;
   checkChangeStreamResult( cur, testConf.csName, testConf.clName, "createix" ) ;

   // drop index
   dbCL.dropIndex( 'a' ) ;
   checkChangeStreamResult( cur, testConf.csName, testConf.clName, "deleteix" ) ;

   // alter collection
   dbCL.alter( { ShardingKey:{ a:1 }, AutoSplit: false } ) ;
   checkChangeStreamResult( cur, testConf.csName, testConf.clName, "invalidatecata" ) ;
   checkChangeStreamResult( cur, testConf.csName, testConf.clName, "alter" ) ;

   // alter collectino space
   dbCS.alter( { LobPageSize: 4096 } ) ;
   checkChangeStreamResult( cur, testConf.csName, "", "alter" ) ;

   // insert
   dbCL.insert( document ) ;
   checkChangeStreamInsertResult( cur, testConf.csName, testConf.clName, document ) ;

   // truncate
   dbCL.truncate() ;
   checkChangeStreamResult( cur, testConf.csName, testConf.clName, "truncatecl" ) ;

   cur.close() ;
}
