/*******************************************************************************
@Description : 1.Test backupOffline, specify["GroupName","Path","Description"]
@Modify list :
               2014-6-20  xiaojun Hu  Init
*******************************************************************************/
function main ( db )
{
   var alreadStart = false;
   var path = "";
   var clName = COMMCLNAME + "_cl14051";
   // Clear the collection space in the beginning
   commDropCL( db, COMMCSNAME, clName, true, true,
      "clean collection space in the beginning" );
   var cl = commCreateCL( db, COMMCSNAME, clName, { ReplSize: -1 }, true, false,
      "create collection in the beginning" );
   // Insert data to SDB
   bakInsertData( cl );
   // Clear backup in the begnning
   bakRemoveBackups( db, CHANGEDPREFIX, true );
   println( "Clear the backup in the beginning" );

   // Backup specify the GroupName/Path/Description
   if( false == commIsStandalone( db ) )
   {
      println( "run mode is group" );
      var groups = commGetGroups( db );
      println( "groups.length:" + groups.length );
      for( var i = 0; i < groups.length; ++i )
      {
         var j = groups[i][0].PrimaryPos;
         path = groups[i][j].dbpath;
         for( var j = 1; j < groups[i].length; ++j )
         {
            if( path !== groups[i][j].dbpath )
            {
               path = "";
               break;
            }
         }
         if( undefined == path )
            throw "failed to get primary node path";

         var bakName = CHANGEDPREFIX + getDateString();
         bakName += i;
         var backup = { "Description": "backup description", "EnsureInc": false, "OverWrite": false };
         backup["Name"] = bakName;
         if( path !== "" )
         {
            backup["Path"] = path;
         }
         println( groups[i][0].GroupName );
         println( JSON.stringify( backup, "", 3 ) );

         bakBackup( db, backup );
         checkBackupInfo( db, "check description backup failed", bakName, path, alreadStart );
         bakRemoveBackups( db, bakName, alreadStart );
         println( "description backup success" );
      }
   }
   else   // run mode standalone
   {
      println( "run mode is standalone" );
      var bakName = COMMCSNAME + "bakstandalone";
      var backup = { "Description": "backup description", "EnsureInc": false, "OverWrite": false };
      backup["Name"] = bakName;
      bakBackup( db, backup );
      checkBackupInfo( db, "check description backup failed", bakName, path, alreadStart );
      // backup and check over, then remove backup
      bakRemoveBackups( db, bakName, alreadStart, path );
      println( "description backup success" );
   }

   // Clear backup in the end
   println( "Clear backup Over in the end" );
   commDropCL( db, COMMCSNAME, clName, true, false, "Drop CL in the " );
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
