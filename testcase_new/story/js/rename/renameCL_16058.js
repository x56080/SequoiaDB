/* *****************************************************************************
@discretion: rename cl after split cl
@author��2018-10-12 wuyan  Init
***************************************************************************** */

main( db );
function main ( db )
{
   try
   {
      //@ clean before
      if( true == commIsStandalone( db ) )
      {
         println( "run mode is standalone" );
         return;
      }
      //less two groups no split
      var allGroupName = getGroupName( db, true );
      if( 1 === allGroupName.length )
      {
         println( "--least two groups" );
         return;
      }

      var clName = CHANGEDPREFIX + "_renamecl16058";
      var newCLName = CHANGEDPREFIX + "_newcl16058";
      commDropCL( db, COMMCSNAME, clName, true, true, "clear collection in the beginning" );
      commDropCL( db, COMMCSNAME, newCLName, true, true, "clear collection in the beginning" );
      var options = { ShardingKey: { no: 1 }, ShardingType: "range", ReplSize: 0, Compressed: true };
      var dbcl = commCreateCLByOption( db, COMMCSNAME, clName, options, true, true );

      //insert records 
      var recordNums = 2000;
      insertData( dbcl, recordNums );

      println( "---begin to split" );
      var percent = 50;
      var targetGroupNums = 2;
      var groupsInfo = getSplitGroups( COMMCSNAME, clName, targetGroupNums );
      var srcGroupName = groupsInfo[0].GroupName;
      var dstGroupName = groupsInfo[1].GroupName;
      splitCL( dbcl, srcGroupName, dstGroupName );

      println( "---begin to rename cl " );
      db.getCS( COMMCSNAME ).renameCL( clName, newCLName );
      var newcl = db.getCS( COMMCSNAME ).getCL( newCLName );

      println( "---begin to check result " );
      checkRenameCLResult( COMMCSNAME, clName, newCLName );
      checkDatas( COMMCSNAME, newCLName, recordNums, groupsInfo );

      commDropCL( db, COMMCSNAME, newCLName, true, true, "clear collection in the ending" );
   }
   catch( e )
   {
      throw e;
   }
}

function checkDatas ( csName, newCLName, expRecordNums, groupsInfo )
{
   try
   {
      //check the record nums      
      var dbcl = db.getCS( csName ).getCL( newCLName );
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
            var cl = sdb.getCS( csName ).getCL( newCLName );
            var num = cl.count();

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
               sdb == undefined;
            }
         }
      }
   }
   catch( e )
   {
      throw buildException( "checkDatas", e )
   }
}
