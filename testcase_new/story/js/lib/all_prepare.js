/* *****************************************************************************
@discretion: Prepare before all test-case
@modify list:
   2014-3-1 Jianhui Xu  Init
***************************************************************************** */


var db = new Sdb( COORDHOSTNAME, COORDSVCNAME );

try
{
   main( db );
}
catch( e )
{
   println( "Before all test-cases environment prepare failed: " + e );
}

function main ( db )
{
   // 0. 生成 basic_operation 目录下的文件
   var cmd = new Cmd();
   var currentUser = System.getCurrentUser().toObj().user;
   // >2  mt-runtest
   var isMtRuntest = cmd.run( "ps -ef | grep mt-runtest | wc -l" ) > 2 ? true : false;
   if( currentUser == "jenkins" )
   {
      cmd.run( "/opt/sequoiadb/bin/sdb -f /tmp/ci/testcase/story/js/lib/basic_operation/generateFiles.js -e \"DIRPATH='/tmp/ci/testcase/story/js/lib/basic_operation/'\"" )
   } else if( !isMtRuntest )
   {
      cmd.run( "bin/sdb -f testcase_new/story/js/lib/basic_operation/generateFiles.js -e \"DIRPATH='testcase_new/story/js/lib/basic_operation/'\"" )
   } else
   {
      try
      {
         //flock -w 10 -x ./local_test_report/.generateFiles.lock -c "bin/sdb -f testcase_new/story/js/lib/basic_operation/generateFiles.js -e \"DIRPATH='testcase_new/story/js/lib/basic_operation/'\""
         // -w wait 10s 
         // -x 排它锁
         cmd.run( "flock -w 10 -x ./local_test_report/.generateFiles.lock -c ", "\"bin/sdb -f testcase_new/story/js/lib/basic_operation/generateFiles.js -e \\\"DIRPATH=\'testcase_new/story/js/lib/basic_operation/'\\\" \" " )
      } catch( e ) { }
   }

   // 1. 删除名称含 local_test 的 cs
   var cols = commGetSnapshot( db, SDB_SNAP_COLLECTIONSPACES, { "Name": Regex( CHANGEDPREFIX, "i" ) }, { "Name": "" } );
   for( var i = 0; i < cols.length; ++i )
   {
      commDropCS( db, cols[i].Name, true, " before all test-cases" );
   }

   // 2. 创建 dummycl 
   var opt = {};
   if( !commIsStandalone( db ) )
   {
      opt = { ShardingKey: { a: 1 }, ShardingType: 'hash', AutoSplit: true };
   }
   commCreateCL( db, COMMCSNAME, COMMDUMMYCLNAME, opt, true, true, "Create dummy collection" );

   // 3. 创建临时目录 /tmp/jstest
   commMakeDir( COORDHOSTNAME, WORKDIR );

   // 4. 删除名称含 local_test 的 backup
   var backups = commGetBackups( db, CHANGEDPREFIX );
   for( var j = 0; j < backups.length; ++j )
   {
      try
      {
         db.removeBackup( { "Name": backups[j] } );
      }
      catch( e )
      {
         println( "Drop backup " + backups[j] + " failed before test-case: " + e );
      }
   }

   // 5. 删除名称含 local_test 的 domain
   var cursor = db.listDomains( { "Name": Regex( CHANGEDPREFIX, "i" ) }, { "Name": "" } );
   var domains = commCursor2Array( cursor );
   for( var j = 0; j < domains.length; ++j )
   {
      commDropDomain( db, domains[i].Name );
   }

   // 6. 删除名称含 local_test 的 procedure
   var procedures = commGetProcedures( db, CHANGEDPREFIX );
   for( var j = 0; j < procedures.length; ++j )
   {
      try
      {
         db.removeProcedure( procedures[j] );
      }
      catch( e )
      {
         println( "Drop procedure " + procedures[j] + " failed before test-case: " + e );
      }
   }


}


