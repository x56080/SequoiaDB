/******************************************************************************
 * @Description   : seqDB-24311 :: 创建/删除索引，renameCS/CL名   
 * @Author        : wu yan
 * @CreateTime    : 2021.08.03
 * @LastEditTime  : 2021.12.02
 * @LastEditors   : Wu Yan
 ******************************************************************************/
testConf.skipStandAlone = true;

main( test );
function test ( testPara )
{
   var csName = COMMCSNAME + "_index24311";
   var newCSName = COMMCSNAME + "_newcs_index24311";
   var clName = COMMCLNAME + "_index24311";
   var newCLName = COMMCLNAME + "_newcl_index24311";
   var indexName = "Index_24311a";
   commDropCS( db, csName, true, "clean cs in the beginning." );
   commDropCS( db, newCSName, true, "clean cs in the beginning." );
   var dbcs = commCreateCS( db, csName, false, "create cs in the beginning." );
   var dbcl = dbcs.createCL( clName, { ReplSize: 0 } );

   var recordNum = 1000;
   var expRecs = insertBulkData( dbcl, recordNum );
   var createTaskId = dbcl.createIndexAsync( indexName, { a: 1, b: 1 } );
   waitTaskFinish( csName, clName, "Create index" );
   var dropTaskId = dbcl.dropIndexAsync( indexName );
   waitTaskFinish( csName, clName, "Drop index" );

   db.renameCS( csName, newCSName );
   checkListTask( createTaskId, newCSName, clName, indexName, "Create index" );
   checkListTask( dropTaskId, newCSName, clName, indexName, "Drop index" );
   //TODO:http://jira.web:8080/browse/SEQUOIADBMAINSTREAM-7762
   //checkSnapTask( createTaskId, newCSName, clName, indexName, "Create index" );
   // checkSnapTask( dropTaskId, newCSName, clName, indexName, "Drop index" );
   checkNoTask( csName, clName, "Create index" );
   checkNoTask( csName, clName, "Drop index" );

   var dbcs = db.getCS( newCSName );
   dbcs.renameCL( clName, newCLName );
   checkListTask( createTaskId, newCSName, newCLName, indexName, "Create index" );
   checkListTask( dropTaskId, newCSName, newCLName, indexName, "Drop index" );
   checkSnapTask( createTaskId, newCSName, newCLName, indexName, "Create index" );
   checkSnapTask( dropTaskId, newCSName, newCLName, indexName, "Drop index" );

   checkNoTask( newCSName, clName, "Create index" );
   checkNoTask( newCSName, clName, "Drop index" );

   commDropCS( db, newCSName, true, "clean cs in the ending." );
}

function checkListTask ( taskId, csName, clName, indexName, taskTypeDesc )
{
   var cursor = db.listTasks( { "TaskID": taskId } );
   var taskInfo;
   var count = 0;
   while( cursor.next() )
   {
      taskInfo = cursor.current().toObj();
      count++;
   }
   cursor.close();

   assert.equal( count, 1, "check task num must be 1!" )
   assert.equal( taskInfo.Name, csName + "." + clName, "check name error! \ntask=" + JSON.stringify( taskInfo ) );
   assert.equal( taskInfo.IndexName, indexName, "check indexName error!\ntask=" + JSON.stringify( taskInfo ) );
   assert.equal( taskInfo.TaskTypeDesc, taskTypeDesc, "check taskType error!\ntask=" + JSON.stringify( taskInfo ) );
}

function checkSnapTask ( taskId, csName, clName, indexName, taskTypeDesc )
{
   var cursor = db.snapshot( SDB_SNAP_TASKS, { "TaskID": taskId } );
   while( cursor.next() )
   {
      var taskInfo = cursor.current().toObj();
      assert.equal( taskInfo.Name, csName + "." + clName, "check name error! \ntask=" + JSON.stringify( taskInfo ) );
      assert.equal( taskInfo.IndexName, indexName, "check indexName error!\ntask=" + JSON.stringify( taskInfo ) );
      assert.equal( taskInfo.TaskTypeDesc, taskTypeDesc, "check taskType error!\ntask=" + JSON.stringify( taskInfo ) );
   }
   cursor.close();
}
