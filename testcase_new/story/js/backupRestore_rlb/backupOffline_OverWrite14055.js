/*******************************************************************************
@Description : 1.Test backupOffline, specify["OverWrite"]
@Modify list :
               2014-6-20  xiaojun Hu  Init
*******************************************************************************/

function main ( db )
{
   var alreadStart = false;
   var path = "";
   var clName = COMMCLNAME + "_cl14055";
   commDropCL( db, csName, clName, true, true, "Drop CL in the beginning" );
   var cl = commCreateCL( db, COMMCSNAME, clName, {ReplSize: -1}, true, false,
      "Create collection in the beginning" );
   bakInsertData( cl );
   bakRemoveBackups( db, CHANGEDPREFIX, true );
   println( "Clear the backup in the beginning" );
   // Backup specify the GroupName/Path/Description
   var runMode = commIsStandalone( db );
   // In standalone,GroupName can be specified all kinds.{"GroupName":""/"abcde"}
   var groups = commGetGroups( db );
   if( false == runMode )
   {
      for( var i = 0; i < groups.length; ++i )
      {
         println( "Begin to backup group = [" + groups[i][0].GroupName + "]" );
         var bakName = CHANGEDPREFIX + "BAK" + i;
         var backup = { EnsureInc: false, OverWrite: false, Description: "backup description" };
         backup["GroupName"] = groups[i][0].GroupName;
         backup["Name"] = bakName;
         commPrint( backup );
         bakBackup( db, backup );
         println( "Backup offline first" );
         commPrint( backup );
         bakBackupByCheckError( db, backup );
         println( "Backup offline second" );
         backup["OverWrite"] = true;
         commPrint( backup );
         bakBackup( db, backup );
         println( "Backup offline thrid" );
         checkBackupInfo( db, "", bakName, path, true );
      }
      bakRemoveBackups( db, bakName, alreadStart, path );
      println( "Backup Over in group" );
   }
   else
   {
      var bakName = COMMCLNAME + "BAKstandalone";
      var bakName = CHANGEDPREFIX + "BAK" + i;
      var backup = { EnsureInc: false, OverWrite: false, Description: "backup description" };
      backup["Name"] = bakName;
      commPrint( backup );
      bakBackup( db, backup );
      println( "Backup offline first" );
      commPrint( backup );
      bakBackupByCheckError( db, backup );
      println( "Backup offline second" );
      backup["OverWrite"] = true;
      commPrint( backup );
      bakBackup( db, backup );
      println( "Backup offline thrid" );
      checkBackupInfo( db, "", bakName, path, true );
      println( "Backup offline third" );
      bakRemoveBackups( db, bakName, alreadStart, path );
   }

   println( "Clear backup Over in the end" );
   commDropCL( db, COMMCSNAME, clName, true, false, "Drop CL in the end" );
}

try
{
   main( db );
   db.close();
}
catch( e )
{
   throw e;
}

