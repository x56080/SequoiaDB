/* *****************************************************************************
@discretion: cl asynchronous split , check the task id
@author：2018-11-06 wangkexin
***************************************************************************** */

main( db );
function main ( db )
{
   try
   {
      if( commGetGroupsNum( db ) < 2 )
      {
         println( "--least two groups" );
         return;
      }

      var clName = CHANGEDPREFIX + "_checktaskidcl16328";
      commDropCL( db, COMMCSNAME, clName, true, true, "clear collection in the beginning" );
      var options = { ShardingKey: { No: 1 }, ShardingType: "range", ReplSize: 0, Compressed: true };

      var dbcl = commCreateCLByOption( db, COMMCSNAME, clName, options, true, true );
      var recordNums = 30000;
      insertData( db, COMMCSNAME, clName, recordNums );

      var targetGroupNums = 2;
      var groupsInfo = getSplitGroups( COMMCSNAME, clName, targetGroupNums );
      var taskId = splitCL( COMMCSNAME, clName, groupsInfo );

      println( "---Begin to check result " );
      checkTaskId( taskId );

      commDropCL( db, COMMCSNAME, clName, true, true, "clear collection in the ending" );
   }
   catch( e )
   {
      throw e;
   }
}

function splitCL ( csName, clName, groupsInfo )
{
   try
   {
      println( "---Begin to splitAsync" );
      var dbcl = db.getCS( csName ).getCL( clName );
      var percent = 90;
      var srcGroupName = groupsInfo[0].GroupName;
      var dstGroupName = groupsInfo[1].GroupName;
      var taskId = dbcl.splitAsync( srcGroupName, dstGroupName, percent );
      return taskId;
   }
   catch( e )
   {
      throw buildException( "splitAsync datas", e )
   }
}

function checkTaskId ( taskId )
{
   try
   {
      println( "---Begin to check the task id" );
      var resultArr = db.listTasks( { "TaskID": taskId } ).toArray();
      if( resultArr.length !== 1 )
      {
         throw buildException( "check listTasks()", null, "check the task id",
            taskId, "not exist" );
      }
   }
   catch( e )
   {
      throw buildException( "check task id", e )
   }
}