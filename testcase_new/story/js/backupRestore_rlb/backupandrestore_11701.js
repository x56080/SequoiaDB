/*******************************************************************************
*@Description : Backup all and remove for data node
*@Modify list :
*               2018-1-10  wenjing Wang Init
*******************************************************************************/
function backupTestCase11701 ()
{

}

backupTestCase11701.prototype = new backupTestCase( db );
backupTestCase11701.prototype.constructor = backupTestCase11701;
backupTestCase11701.prototype.clName = COMMCLNAME + "_11701";
backupTestCase11701.prototype.execTest =
   function( backupName, path )
   {
      this.docs = bakInsertData( this.cl );
      println( "write docs: " + this.docs.length );
      this.oids.push( sdbPutLob( this.cl, path ) );
      println( "putLob: " + this.oids[0] );

      bakBackup( this.db, { "Name": backupName } );
      if( this.group !== undefined )
      {
         println( "backup on " + this.group.GroupName );
         this.removeNodeExceptPrimary();
      }

      if( this.nodeinfo !== undefined )
      {
         var bakInfo = new backUpInfo( backupName, this.nodeinfo.dbPath + "bakfile" );
      }
      else
      {
         var dbPath = db.snapshot( 6 ).current().toObj()["Disk"]["DatabasePath"];
         var bakInfo = new backUpInfo( backupName, dbPath + "bakfile" );
      }
      if( this.group !== undefined )
      {
         this.checkBackupRes( bakInfo, 1, [this.group.GroupName] );
      }
      else
      {
         this.checkBackupRes( bakInfo, 1 );
      }

      bakRemoveBackups( this.db, backupName, false );
      if( this.group !== undefined )
      {
         this.checkBackupRes( bakInfo, 0, [this.group.GroupName] );
      }
      else
      {
         this.checkBackupRes( bakInfo, 0 );
      }

      println( "check backup dir..." );
      if( !IsBakPathEmpty( this.cmd, bakInfo.bakPath ) )
      {
         throw new Error( "removeBackup expect backPath is empty, but real is not" );
      }
      println( " finish execTest... " );
   }

function main ( db )
{
   try
   {
      var testBackUp = new backupTestCase11701();
      if( testBackUp.setUp() )
      {
         testBackUp.test();
      }
      testBackUp.tearDown();
   }
   catch( e )
   {
      var tmp = new Error( "tmp" )
      if( e.constructor === tmp.constructor )
      {
         println( e.fileName + ":" + e.lineNumber + " throw " + e.message );
         println( e.stack );
      }

      var backupDir = "/tmp/ci/rsrvnodelog/11701";
      File.mkdir( backupDir );
      for( var i = 0; i < this.logSourcePaths.length; i++ )
      {
         File.scp( this.logSourcePaths[i], backupDir + "/sdbdiag" + i + ".log" );
      }
      throw e;
   }
   finally
   {
      commDropCL( db, this.csName, backupTestCase11701.prototype.clName, true, true, "finally ：Drop CL in the end" );
      db.removeRG( backupandrestoreGroup );
   }
}

main( db )
