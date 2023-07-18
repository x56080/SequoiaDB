/***************************************************************************************************
 * @Description: change stream
 * @ATCaseID: changeStream_010
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
 *    change stream watch collection space from db
 **************************************************************************************************/

var selectedGroup = selectGroupWithSecondary( db ) ;
testConf.clOpt = { Group: selectedGroup, ReplSize: -1 } ;
testConf.clName = COMMCLNAME + "_change_stream_010" ;

var newCSName = COMMCSNAME + "_change_stream_010_new" ;
var newCLName = testConf.clName + "_new" ;

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
   var nodeStartDB ;
   var nodeResumeDB ;
   if ( testPara.usePrimary )
   {
      nodeStartDB = selectPrimaryForChangeStream( db, testConf.csName, testConf.clName ) ;
      nodeResumeDB = selectSecondaryForChangeStream( db, testConf.csName, testConf.clName ) ;
   }
   else
   {
      nodeStartDB = selectSecondaryForChangeStream( db, testConf.csName, testConf.clName ) ;
      nodeResumeDB = selectPrimaryForChangeStream( db, testConf.csName, testConf.clName ) ;
   }

   // start watch
   var options = { CollectionSpaces: [ testConf.csName ],  MaxWaitTime:0 } ;
   var cur = nodeStartDB.watch( StreamToken(), options ) ;
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

   // rename collection
   dbCS.renameCL( testConf.clName, newCLName ) ;
   checkChangeStreamResult( cur, testConf.csName, testConf.clName, "invalidatecata" ) ;
   checkChangeStreamResult( cur, testConf.csName, testConf.clName, "renamecl" ) ;

   // insert
   dbCL = dbCS.getCL( newCLName ) ;
   dbCL.insert( document ) ;
   checkChangeStreamInsertResult( cur, testConf.csName, newCLName, document ) ;

   // drop collection
   dbCS.dropCL( newCLName ) ;
   checkChangeStreamResult( cur, testConf.csName, newCLName, "invalidatecata" ) ;
   checkChangeStreamResult( cur, testConf.csName, newCLName, "deletecl" ) ;

   // rename collection space
   db.renameCS( testConf.csName, newCSName ) ;
   checkChangeStreamResult( cur, testConf.csName, undefined, "invalidatecata" ) ;
   var token = checkChangeStreamErrorResult( cur, false, SDB_DMS_CS_NOTEXIST ) ;
   checkChangeStreamClosed( cur ) ;

   // create collection
   dbCS = db.getCS( newCSName ) ;
   dbCL = dbCS.createCL( newCLName, testConf.clOpt ) ;
   dbCL.insert( document ) ;

   // resume
   options = { CollectionSpaces: [ newCSName ],  MaxWaitTime:0 } ;
   cur = nodeResumeDB.watch( StreamToken( token ), options ) ;

   // create collection
   checkChangeStreamResult( cur, newCSName, newCLName, "createcl" ) ;
   checkChangeStreamInsertResult( cur, newCSName, newCLName, document ) ;

   // drop collection space
   db.dropCS( newCSName ) ;
   checkChangeStreamResult( cur, newCSName, undefined, "invalidatecata" ) ;
   checkChangeStreamErrorResult( cur, false, SDB_DMS_CS_NOTEXIST ) ;
   checkChangeStreamClosed( cur ) ;
}
