/*******************************************************************************
@Description : 1.Test backupOffline, specify [EnsureInc] to backup.
@Modify list :
               2014-3-16  Jianhui Xu  Init
               2014-6-20  xiaojun Hu  Changed
*******************************************************************************/
//globle variable
var backupName = CHANGEDPREFIX + "_bk";

// main entry
function main ()
{
   var alreadStart = false;
   var clName = COMMCLNAME + "_cl14052";
   // clean
   commDropCL( db, COMMCSNAME, clName, true, true, "drop cl in begin" );
   bakRemoveBackups( db, CHANGEDPREFIX, true );
   // create cl
   var varCL = commCreateCL( db, COMMCSNAME, clName, -1, true, true, false, "create cl in begin" );
   // insert data
   bakInsertData( varCL );
   // define backup object array
   var grpArray = commGetCSGroups( db, COMMCSNAME );
   var bkObjArray = [];
   println( JSON.stringify( grpArray ) );
   bkObjArray.push( { Name: backupName, Description: "default path full backup", MaxDataFileSize: 32, GroupName: grpArray } );
   bkObjArray.push( { Name: backupName, Description: "default path full backup", OverWrite: true, MaxDataFileSize: 32, GroupName: grpArray } );
   bkObjArray.push( { Name: backupName, Description: "default path inc backup", EnsureInc: true, MaxDataFileSize: 32, GroupName: grpArray } );

   // Run Mode
   var runMode = commIsStandalone( db );
   println( "The runmode : [" + runMode + "]" );
   // backup
   for( var i = 0; i < bkObjArray.length; ++i )
   {
      commPrint( bkObjArray[i] );
      bakBackup( db, bkObjArray[i] );
      varCL.insert( { times: i } );
   }

   if( i != bkObjArray.length )
   {
      throw "expected to complete " + bkObjArray.length + " times, real completed " + i + " times";
   }

   checkBackupInfo( db, "Check ensure inc backup failed", backupName, "", false, { EnsureInc: true } );
   // remove backup
   bakRemoveBackups( db, backupName, alreadStart );
   // clean - drop cl
   commDropCL( db, COMMCSNAME, clName, false, false, "drop cl in clean" );
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
