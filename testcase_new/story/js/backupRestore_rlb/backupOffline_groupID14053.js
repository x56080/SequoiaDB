/*******************************************************************************
@Description : Test db.backupOffline().Specify [GroupID].{GroupID:[1000,1001]}
@Modify list :
               2014-6-20  xiaojun Hu Init
*******************************************************************************/
function main ( db )
{
   var alreadStart = false;
   var path = "";
   var clName = COMMCLNAME + "_cl14053";
   commDropCL( db, COMMCSNAME, clName, true, true, "Drop CL in the beginning" );
   var cl = commCreateCL( db, COMMCSNAME, clName, {ReplSize: -1}, true, false,
      "Create collection in the beginning" );
   bakInsertData( cl );
   bakRemoveBackups( db, CHANGEDPREFIX, true );
   println( "Clear the backup in the beginning" );
   // Backup Offline specify the [GroupID]
   var groupID = commGetGroups( db );
   for( var i = 0; i < groupID.length; ++i )
   {
      if( groupID.length == 1 ) continue;
      var bakName = CHANGEDPREFIX + "_bak_" + i;
      var backup = { "GroupID": [] };
      backup["GroupID"].push( groupID[i][0].GroupID );
      backup["Name"] = bakName;
      commPrint( backup );
      bakBackup( db, backup );
      checkBackupInfo( db, "", bakName, path, alreadStart );
      bakRemoveBackups( db, bakName, alreadStart, path );
   }
   println( "Clear backup Over in the end" );
   commDropCL( db, COMMCSNAME, clName, true, false, "Drop CL in the end" );
}

try
{
   if( false == commIsStandalone( db ) )
   {
      main( db );
      db.close();
   }
   else
      println( "This is standalone mode" );
}
catch( e )
{
   throw e;
}
