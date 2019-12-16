/*******************************************************************************
@Description : Test Backup SDB by use default argument.[db.backupOffline()]
@Modify list :
               2014-6-20  xiaojun Hu Init
*******************************************************************************/

function main ( db )
{
   var backupName = "";   // in default backup, use time as backup name
   var alreadStart = false;
   var clName = COMMCLNAME + "_cl14050";
   commDropCL( db, csName, clName, true, true, "Drop CL in the beginning" );
   var cl = commCreateCL( db, csName, clName, {ReplSize: -1}, true, false,
      "Create collection in the beginning" );
   bakInsertData( cl );
   bakRemoveBackups( db, backupName, true );
   println( "Clear the backup in the beginning" );
   // Backup don't have options [Test]. -67:Backup always begin
   bakBackup( db );
   println( "default backup success" );
   // check backup operation is success or not
   try
   {
      checkBackupInfo( db, "check default backup failed" );
   } catch( e )
   {
      println( e );
      throw "check default backup failed";
   } finally
   {
      bakRemoveBackups( db, backupName, alreadStart );
   }
   // Drop collection
   commDropCL( db, csName, clName, true, false, "Drop CL in the end" );
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
