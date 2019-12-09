/* *****************************************************************************
@discretion: rename cl ,the cl is spliting
@author��2018-10-15 wuyan  Init
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

      var clName = CHANGEDPREFIX + "_renamecl16067";
      var newCLName = CHANGEDPREFIX + "_newcl16067";
      commDropCL( db, COMMCSNAME, clName, true, true, "clear collection in the beginning" );
      commDropCL( db, COMMCSNAME, newCLName, true, true, "clear collection in the beginning" );
      var options = { ShardingKey: { no: 1 }, ShardingType: "range", ReplSize: 0, Compressed: true };

      var dbcl = commCreateCLByOption( db, COMMCSNAME, clName, options, true, true );
      var recordNums = 10000;
      insertData( dbcl, recordNums );

      var targetGroupNums = 2;
      var groupsInfo = getSplitGroups( COMMCSNAME, clName, targetGroupNums );
      var taskId = splitCL( COMMCSNAME, clName );
      renameCL( COMMCSNAME, clName, newCLName );

      println( "---begin to check result " );
      checkSplitResult( COMMCSNAME, clName, taskId, recordNums, groupsInfo );
      checkRenameCLResult( COMMCSNAME, newCLName, clName );

      commDropCL( db, COMMCSNAME, clName, true, true, "clear collection in the ending" );
   }
   catch( e )
   {
      throw e;
   }
}

function splitCL ( csName, clName )
{
   try
   {
      println( "---Begin to split" );
      var dbcl = db.getCS( csName ).getCL( clName );
      var percent = 50;
      var targetGroupNums = 2;
      var groupsInfo = getSplitGroups( COMMCSNAME, clName, targetGroupNums );
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

function renameCL ( csName, oldCLName, newCLName )
{
   try
   {
      println( "---Begin to rename cl " );
      db.getCS( csName ).renameCL( oldCLName, newCLName );
      throw "need throw error";
   }
   catch( e )
   {
      if( e != -334 )
      {
         throw buildException( "rename cl16067:", e );
      }
   }
}

function checkSplitResult ( csName, clName, taskId, expRecordNums, groupsInfo )
{
   try
   {
      //waiting for split 
      var sleepInteval = 10;
      var sleepDuration = 0;
      var maxSleepDuration = 100000;

      while( ( db.listTasks( { "TaskID": taskId } ).next() !== undefined ) && sleepDuration < maxSleepDuration )
      {
         sleep( sleepInteval );
         sleepDuration += sleepInteval;
      }
      println( "---waiting split time:" + sleepDuration );
      //check the record nums      
      var dbcl = db.getCS( csName ).getCL( clName );
      var count = dbcl.count();
      if( count != expRecordNums )
      {
         throw buildException( "check datas", null, "check the new cl record nums",
            expRecordNums, count );
      }

      //test record nums of split groups
      for( var i = 0; i < 2; i++ )
      {
         try
         {
            var sdb = new Sdb( groupsInfo[i].HostName, groupsInfo[i].svcname );
            var cl = sdb.getCS( csName ).getCL( clName );
            var num = cl.count();
            println( num )

            if( Number( num ) !== expRecordNums / 2 )			
            {
               throw buildException( "checkClSplitRecordNums", "count wrong", "count()", expRecordNums / 2, num )
            }
         }
         catch( e )
         {
            throw buildException( "checkClSplitResult()", e );
         }
         finally
         {
            if( sdb !== undefined )
            {
               sdb.close();
            }
         }
      }
   }
   catch( e )
   {
      throw buildException( "checksplit", e )
   }
}
