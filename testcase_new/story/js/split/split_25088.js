/******************************************************************************
 * @Description   : seqDB-25088:取消切分任务，检查task信息
 * @Author        : Zhang Yanan
 * @CreateTime    : 2022.01.27
 * @LastEditTime  : 2022.05.30
 * @LastEditors   : Zhang Yanan
 ******************************************************************************/
testConf.clName = COMMCLNAME + "_25088cl";
testConf.clOpt = { "ShardingKey": { "no": 1 }, "ShardingType": "hash" };
testConf.useSrcGroup = true;
testConf.useDstGroup = true;
testConf.skipOneGroup = true;
testConf.skipStandAlone = true;

main( test );
function test ( testPara )
{
   var varCL = testPara.testCL;
   var isTaskfinish = false;
   var expResultCode = -243;
   var expResultCodeDesc = "Task has been canceled";

   insertData( varCL, 5000 );
   var taskId = varCL.splitAsync( testPara.srcGroupName, testPara.dstGroupNames[0], 50 );
   var waitTime = parseInt( Math.random() * 5 );
   sleep( waitTime * 1000 );
   try
   {
      db.cancelTask( taskId );
   }
   catch( e )
   {
      if( e == SDB_TASK_ALREADY_FINISHED )
      {
         isTaskfinish = true;
      }
      else
      {
         throw new Error( "cancelTask error ! errorCode: " + e );
      }
   }

   try
   {
      // 等待任务状态刷新
      db.waitTasks( taskId );
   }
   catch( e )
   {
      if( e != SDB_TASK_HAS_CANCELED )
      {
         throw new Error( "waitTasks error ! errorCode: " + e );
      }
   }

   var taskInfo = db.getTask( taskId ).toObj();
   var actResultCode = taskInfo.ResultCode;
   var actResultCodeDesc = taskInfo.ResultCodeDesc;
   if( isTaskfinish )
   {
      expResultCode = 0;
      expResultCodeDesc = "Succeed";
   }

   assert.equal( actResultCode, expResultCode, "taskInfo=" + JSON.stringify( taskInfo ) );
   assert.equal( actResultCodeDesc, expResultCodeDesc, "taskInfo=" + JSON.stringify( taskInfo ) );
}