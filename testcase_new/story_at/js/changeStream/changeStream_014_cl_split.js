/***************************************************************************************************
 * @Description: change stream
 * @ATCaseID: changeStream_014
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
 *    change stream with split collection
 **************************************************************************************************/

var selectedGroup = selectGroupWithSecondary( db ) ;
testConf.clOpt = { Group: selectedGroup, ReplSize: -1, ShardingKey: {a:1}, ShardingType: "range" } ;
testConf.clName = COMMCLNAME + "_change_stream_014" ;
var holdCL = testConf.clName + "_hold" ;

var targetGroup = "" ;
var groups = commGetDataGroupNames( db ) ;
for ( var i = 0; i < groups.length; i++ )
{
   if ( groups[ i ] != selectedGroup )
   {
      targetGroup = groups[ i ] ;
      break ;
   }
}

mainLoop( wrapTest, testPara ) ;

function wrapTest( testPara )
{
   try
   {
      commDropCL( db, testConf.csName, holdCL, true, true ) ;
      test( testPara ) ;
   }
   finally
   {
      commDropCL( db, testConf.csName, holdCL, true, true ) ;
   }
}

function test( testPara )
{
   // select node
   var nodeSourceDB ;
   var nodeTargetDB ;
   if ( testPara.usePrimary )
   {
      nodeSourceDB = db.getRG( selectedGroup ).getMaster().connect() ;
      nodeTargetDB = db.getRG( targetGroup ).getMaster().connect() ;
   }
   else
   {
      nodeSourceDB = db.getRG( selectedGroup ).getSlave().connect() ;
      nodeTargetDB = db.getRG( targetGroup ).getSlave().connect() ;
   }

   var dbCS = db.getCS( testConf.csName ) ;
   var dbCL = dbCS.getCL( testConf.clName ) ;

   // create hold collection, so won't drop CS
   dbCS.createCL( holdCL, { AutoSplit: true, ShardingKey:{ a: 1 }, ReplSize: -1 } ) ;

   // prepare data
   var data = [] ;
   for ( i = 0 ; i < 100 ; ++ i )
   {
      data.push( { _id:i, a:i, b:i } ) ;
   }
   dbCL.insert( data ) ;
   dbCL.split( selectedGroup, targetGroup, 50 ) ;

   // start watch
   var nodeSourceCL = nodeSourceDB.getCS( testConf.csName ).getCL( testConf.clName ) ;
   var nodeTargetCS = nodeTargetDB.getCS( testConf.csName ) ;
   var curSource = nodeSourceCL.watch( StreamToken(), { MaxWaitTime:0 } ) ;
   var curTarget = nodeTargetCS.watch( StreamToken(), { MaxWaitTime:0 } ) ;

   // split
   dbCL.split( selectedGroup, targetGroup, 50 ) ;

   // check split result
   checkChangeStreamResult( curSource, testConf.csName, testConf.clName, "invalidatecata", "NonBusinessOP" ) ;
   for ( var i = 25 ; i < 50 ; ++ i )
   {
      checkChangeStreamDeleteResult( curSource, testConf.csName, testConf.clName, { _id: i }, "NonBusinessOP" ) ;
      checkChangeStreamInsertResult( curTarget, testConf.csName, testConf.clName, { _id: i, a: i, b: i }, "NonBusinessOP" ) ;
   }
   checkChangeStreamResult( curTarget, testConf.csName, testConf.clName, "invalidatecata", "NonBusinessOP" ) ;

   // split again
   dbCL.split( selectedGroup, targetGroup, 100 ) ;

   // check split result
   // have hold collection, must be drop CL
   checkChangeStreamErrorResult( curSource, true, SDB_DMS_NOTEXIST ) ;
   checkChangeStreamClosed( curSource ) ;
   for ( var i = 0 ; i < 25 ; ++ i )
   {
      checkChangeStreamInsertResult( curTarget, testConf.csName, testConf.clName, { _id: i, a: i, b: i }, "NonBusinessOP" ) ;
   }
   checkChangeStreamResult( curTarget, testConf.csName, testConf.clName, "invalidatecata", "NonBusinessOP" ) ;

   curSource.close() ;
   curTarget.close() ;
}
