/*******************************************************************************
@Description : Test db.backupOffline().Specify [IsSubDir].{IsSubDir:false}
@Modify list :
               2014-6-20  xiaojun Hu Init
*******************************************************************************/
function main ( db )
{
   var alreadStart = false;
   var path = "";
   var clName = COMMCLNAME + "_cl14054";
   commDropCL( db, COMMCSNAME, clName, true, true,
      "Drop CL in the beginning" );
   var cl = commCreateCL( db, COMMCSNAME, clName, -1, true, true, false,
      "Create collection in the beginning" );
   bakInsertData( cl );
   bakRemoveBackups( db, CHANGEDPREFIX, true );
   println( "Clear the backup in the beginning" );
   // Backup Offline specify the [groups]
   if( false == commIsStandalone( db ) )
   {
      var groups = commGetGroups( db );
      for( var i = 0; i < groups.length; ++i )
      {
         var bakName = CHANGEDPREFIX + "_bak_" + i;
         var backup = { "IsSubDir": true };
         backup["Name"] = bakName;
         backup["Gruop"] = groups[i][0].GroupName;

         commPrint( backup );
         bakBackup( db, backup );
         checkBackupInfo( db, "", bakName, path, alreadStart );
         bakRemoveBackups( db, bakName, alreadStart, path );
      }
      println( "groupID backup success" );
   }
   else
   {
      var bakName = CHANGEDPREFIX + "bakStandalone";
      var backup = { "IsSubDir": true };
      backup["Name"] = bakName;
      bakBackup( db, backup );
      checkBackupInfo( db, "", bakName, path, alreadStart );
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
