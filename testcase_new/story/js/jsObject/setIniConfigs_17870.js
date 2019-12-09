/******************************************************************************
*@Description : test Oma function: setIniConfigs getIniConfigs            
*               seqDB-17870:sdb和sdbcm的setIniConfigs支持ini格式，ini字段为多层对象嵌套 
*@Author      : 2019-1-18  XiaoNi Huang
*@Info        ：用例执行机可能没有装sequoiadb，本地测试时new Oma()会失败，只能用Cmd；
                而Cmd不能远程执行，所以远程测试时只能使用Remote
******************************************************************************/
main();

function main ()
{
   try
   {
      // init and prepare data
      println( "\n---Begin to init and prepare data" );
      var cmd = new Cmd();
      var oma = new Oma( COORDHOSTNAME, CMSVCNAME );
      var remote = new Remote( COORDHOSTNAME, CMSVCNAME );

      var filePath = WORKDIR + "/" + "config17870.ini";
      initWorkDir( cmd, remote );
      cmd.run( "rm -f " + filePath );

      var iniData = { "A.a1.a2.a3.a4.a5": 1, "B.b1": "2" };
      var expFileData = '["[A]","a1.a2.a3.a4.a5=\\"1\\"","","[B]","b1=\\"2\\"",""]';
      var expGetData = '{"A.a1.a2.a3.a4.a5":"1","B.b1":"2"}';

      // sdb test
      sdbSetIniConf( iniData, filePath );

      var actData = readLocalFile( cmd, filePath );
      checkResult( expFileData, actData );

      var actData = sdbGetIniConf( filePath );
      checkResult( expGetData, actData );

      // clear local data
      println( "\n---Begin to clear local data\n" );
      cmd.run( "rm -f " + filePath );


      // sdbcm test
      sdbcmSetIniConf( oma, iniData, filePath );

      var actData = readRemoteFile( remote, filePath );
      checkResult( expFileData, actData );

      var actData = sdbcmGetIniConf( oma, filePath );
      checkResult( expGetData, actData );

      // clear remote data
      println( "\n---Begin to clear remote data" );
      var file = remote.getFile();
      file.remove( filePath );
   }
   catch( e )
   {
      throw e;
   }
}

function sdbSetIniConf ( data, filePath )
{
   println( "\n---Begin to exec setIniConfigs with sdb" );
   Oma.setIniConfigs( data, filePath );
}

function sdbGetIniConf ( filePath )
{
   println( "\n---Begin to exec getIniConfigs with sdb" );
   var rc = Oma.getIniConfigs( filePath );
   return JSON.stringify( rc.toObj() );
}

function sdbcmSetIniConf ( oma, data, filePath )
{
   println( "\n---Begin to exec setIniConfigs with sdbcm" );
   oma.setIniConfigs( data, filePath );
}

function sdbcmGetIniConf ( oma, filePath )
{
   println( "\n---Begin to exec getIniConfigs with sdbcm" );
   var rc = oma.getIniConfigs( filePath );
   return JSON.stringify( rc.toObj() );
}

function readLocalFile ( cmd, filePath )
{
   println( "\n---Begin to read local file[" + filePath + "]" );
   var rc = cmd.run( 'cat ' + filePath ).split( "\n" );
   return JSON.stringify( rc );
}

function readRemoteFile ( remote, filePath )
{
   println( "\n---Begin to read remote[" + filePath + "]" );
   var file = remote.getFile( filePath );
   var rc = file.read().split( "\n" );
   return JSON.stringify( rc );
}

function checkResult ( expData, actData )
{
   println( "   Begin to check results" );
   if( expData !== actData )
   {
      throw buildException( "checkResult", null, "[checkResult]", expData, "  " + actData );
   }
}

function initWorkDir ( cmd, remote ) 
{
   // localhost
   try
   {
      cmd.run( "ls " + WORKDIR );
   }
   catch( e ) 
   {
      if( 2 === e )   // 2: No such file or directory
      {
         cmd.run( "mkdir -p " + WORKDIR );
      }
      else
      {
         throw e;
      }
   }

   // remote host
   var file = remote.getFile();
   var dirExist = file.exist( WORKDIR );
   if( false === dirExist )
   {
      commMakeDir( COORDHOSTNAME, WORKDIR );
   }
}