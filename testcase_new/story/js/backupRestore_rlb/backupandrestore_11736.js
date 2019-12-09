/*******************************************************************************
*@Description : Backup all and restore for cluster
*@Modify list :
*               2018-1-10  wenjing Wang Init
*******************************************************************************/
function backupTestCase11736 ()
{

}

backupTestCase11736.prototype = new backupTestCase( db );
backupTestCase11736.prototype.constructor = backupTestCase11736;
backupTestCase11736.prototype.csName = COMMCSNAME + "_11736";
backupTestCase11736.prototype.clName = COMMCLNAME + "_11736";
backupTestCase11736.prototype.reInit =
   function()
   {

   }

backupTestCase11736.prototype.createShardingCL =
   function()
   {
      this.groupNames = [];
      this.groups = commGetGroups( this.db );
      var hosts = getAllHosts( this.groups );
      createBackupRestoreGroup( db, hosts );
      this.group = db.getRG( backupandrestoreGroup ).getDetail().next().toObj();

      for( var i = 0; i < this.groups.length; ++i )
      {
         this.groupNames.push( this.groups[i][0].GroupName );
      }
      this.groupNames.push( backupandrestoreGroup );
      this.domainName = "backup11736";

      commDropCS( this.db, this.csName, true, "dropCS in the beginning" );
      commDropDomain( this.db, this.domainName );
      commCreateDomain( this.db, this.domainName, this.groupNames, { AutoSplit: true } );

      var csOpt = { Domain: this.domainName, LobPageSize: 4096 };
      commCreateCS( this.db, this.csName, false, "create cs in begin", csOpt );
      var clOpt = { ShardingType: 'hash', ShardingKey: { no: 1 }, ReplSize: -1 }
      this.cl = commCreateCLByOption( this.db, this.csName, this.clName, clOpt, true, false,
         "Create collection in the beginning" );
   }

backupTestCase11736.prototype.init =
   function()
   {
      this.createShardingCL();
      println( JSON.stringify( this.group ) );
      for( var i = 0; i < this.group.Group.length; ++i )
      {
         if( this.group.PrimaryNode === this.group.Group[i].NodeID || i === this.group.Group.length - 1 )
         {
            var hostName = this.group.Group[i].HostName;
            var svcName = this.group.Group[i].Service[0].Name;
            var dbPath = this.group.Group[i].dbpath;
            println( "init " + hostName + svcName + dbPath );
            this.nodeinfo = new nodeInfo( this.group.GroupName, hostName, svcName, dbPath );
            this.cmd = getCmdByHostName( this.localCmd, hostName );
            break;
         }
      }

      return true;
   }

backupTestCase11736.prototype.execTest =
   function( backupName, path )
   {
      var bakInfo = new backUpInfo( backupName, this.nodeinfo.dbPath + "bakfile" );
      this.groupNames.push( "SYSCatalogGroup" );
      this.docs = bakInsertData( this.cl );
      println( "write docs: " + this.docs.length );
      this.oids.push( sdbPutLob( this.cl, path ) );
      println( "putLob: " + this.oids[0] );

      // ȫ������
      bakBackup( this.db, { "Name": backupName } );
      this.checkBackupRes( bakInfo, 1, this.groupNames );

      this.docs = bakInsertData( this.cl );
      println( "write docs: " + this.docs.length );
      this.oids.push( sdbPutLob( this.cl, path ) );
      println( "putLob: " + this.oids[0] );

      for( var i = 0; i < 1000; ++i )
      {
         commCreateCL( this.db, this.csName, this.clName + "_" + i, -1, true, true, false,
            "Create collection for backup" );
      }

      bakBackup( this.db, { "Name": backupName, EnsureInc: true } );
      this.checkBackupRes( bakInfo, 2, this.groupNames );
      if( this.group !== undefined )
      {
         println( "backup on " + this.group.GroupName );
         this.removeNodeExceptPrimary();
      }
      println( "start restore..." );
      sdbRestore( this.sdb, this.cmd, bakInfo, this.nodeinfo );
      this.checkResult( 2 );
   }

backupTestCase11736.prototype.tearDown =
   function()
   {
      bakRemoveBackups( this.db, CHANGEDPREFIX, true );
      commDropCS( this.db, this.csName, true, "dropCS in the end" )
      commDropDomain( this.db, this.domainName );
   }

function main ( db )
{
   if( commIsStandalone( db ) )
   {
      return;
   }

   try
   {
      var testBackUp = new backupTestCase11736();
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

      var backupDir = "/tmp/ci/rsrvnodelog/11736";
      File.mkdir( backupDir );
      for( var i = 0; i < this.logSourcePaths.length; i++ )
      {
         File.scp( this.logSourcePaths[i], backupDir + "/sdbdiag" + i + ".log" );
      }
      throw e;
   }
   finally
   {
      commDropCS( this.db, backupTestCase11736.prototype.csName, true, "finally: dropCS in the end" );
      db.removeRG( backupandrestoreGroup );
   }
}

main( db )
