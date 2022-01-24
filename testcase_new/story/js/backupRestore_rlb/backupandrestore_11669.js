/*******************************************************************************
*@Description : Backup (EnsureInc=true) and restore for data node
*@Modify list :
*               2018-1-10  wenjing Wang Init
*******************************************************************************/
testConf.skipStandAlone = true;
function backupTestCase11669 () { }

backupTestCase11669.prototype = new backupTestCase( db );
backupTestCase11669.prototype.constructor = backupTestCase11669;
backupTestCase11669.prototype.clName = COMMCLNAME + "_11669";
backupTestCase11669.prototype.reInit =
   function()
   {
      if( this.group === undefined )
      {
         this.sdb = new Sdb( COORDHOSTNAME, COORDSVCNAME );
         this.db = this.sdb
      }
      else
      {
         this.db = new Sdb( this.nodeinfo.hostName, this.nodeinfo.svcName );
      }
   }

backupTestCase11669.prototype.execTest =
   function( backupName, path )
   {
      this.docs = bakInsertData( this.cl );
      this.oids.push( sdbPutLob( this.cl, path ) );
      bakBackup( this.db, { "Name": backupName } );
      if( this.nodeinfo !== undefined )
      {
         var bakInfo = new backUpInfo( backupName, this.nodeinfo.dbPath + "bakfile" );
      }
      else
      {
         var dbPath = db.snapshot( 6 ).current().toObj()["Disk"]["DatabasePath"];
         var bakInfo = new backUpInfo( backupName, dbPath + "bakfile" );
      }

      var times = 1;
      this.checkBackupRes( bakInfo, times++, [this.group.GroupName] );
      bakInsertData( this.cl, this.docs );
      this.oids.push( sdbPutLob( this.cl, path ) );
      bakBackup( this.db, { "Name": backupName, EnsureInc: true } );
      this.checkBackupRes( bakInfo, times++, [this.group.GroupName] );

      bakInsertData( this.cl, this.docs );
      this.oids.push( sdbPutLob( this.cl, path ) );
      bakBackup( this.db, { "Name": backupName, EnsureInc: true } );
      if( this.group !== undefined )
      {
         this.removeNodeExceptPrimary();
      }
      this.checkBackupRes( bakInfo, times, [this.group.GroupName] );
      sdbRestore( this.sdb, this.cmd, bakInfo, this.nodeinfo );
      this.checkResult( times );
   }

// main( test );

function test ()
{
   try
   {
      if( commIsStandalone( db ) )
      {
         return;
      }

      var testBackUp = new backupTestCase11669();
      if( testBackUp.setUp() )
      {
         testBackUp.test();
      }
      testBackUp.tearDown();
   }
   catch( e )
   {
      if( e instanceof Error )
      {
         println( e.fileName + ":" + e.lineNumber + " throw " + e.message );
         println( e.stack );
      }

      var backupDir = "/tmp/ci/rsrvnodelog/11669";
      File.mkdir( backupDir );
      for( var i = 0; i < this.logSourcePaths.length; i++ )
      {
         File.scp( this.logSourcePaths[i], backupDir + "/sdbdiag" + i + ".log" );
      }
      throw e;
   }
   finally
   {
      commDropCL( db, COMMCSNAME, backupTestCase11669.prototype.clName, true, true, "finally ：Drop CL in the end" );
      db.removeRG( backupandrestoreGroup );
   }
}
