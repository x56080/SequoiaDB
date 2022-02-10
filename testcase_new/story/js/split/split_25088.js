/******************************************************************************
 * @Description   : seqDB-25088:取消切分任务，检查task信息
 * @Author        : Zhang Yanan
 * @CreateTime    : 2022.01.27
 * @LastEditTime  : 2022.01.27
 * @LastEditors   : Zhang Yanan
 ******************************************************************************/
testConf.clName = COMMCLNAME + "_25088cl";
testConf.clOpt = { "ShardingKey": { "no": 1 }, "ShardingType": "hash" };
testConf.useSrcGroup = true
testConf.useDstGroup = true
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
   var waitTime = getRandomInt( 0, 5 );
   sleep( waitTime * 1000 );
   try
   {
      db.cancelTask( taskId );
   } catch( e )
   {
      if( commCompareErrorCode( e, SDB_TASK_ALREADY_FINISHED ) )
      {
         isTaskfinish = true;
      } else
      {
         throw new Error( "Unexpected error ! errorCode: " + e );
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
   assert.equal( actResultCode, expResultCode );
   assert.equal( actResultCodeDesc, expResultCodeDesc );
}

function getRandomInt ( min, max )
{
   var range = max - min;
   var value = min + parseInt( Math.random() * range );
   return value;
}