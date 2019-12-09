/******************************************************************************
*@Description : test Oma function: setIniConfigs getIniConfigs
                parameter:  EnableType, StrDelimiter                       
*               sdb和sdbcm的setIniConfigs/getIniConfigs接口参数EnableType和StrDelimiter测试 
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

      var filePath = WORKDIR + "/" + "config17869.ini";
      initWorkDir( cmd, remote );
      cmd.run( "rm -f " + filePath );

      var iniData = { "A.a1": 1, "A.a2": "2" };

      // sdb test
      // setIniConfigs[e.g: {EnableType: true, StrDelimiter: true}]
      sdbSetIniConf( iniData, filePath, true, true );

      var expData = '["[A]","a1=1","a2=\\"2\\"",""]';
      var actData = readLocalFile( cmd, filePath );
      checkResult( expData, actData );

      // getIniConfigs[e.g: {EnableType: true, StrDelimiter: true}]
      var expData = '{"A.a1":1,"A.a2":"2"}';
      var actData = sdbGetIniConf( filePath, true, true );
      checkResult( expData, actData );

      var actData = sdbGetIniConf( filePath, true, false );
      checkResult( expData, actData );

      var expData = '{"A.a1":"1","A.a2":"2"}';
      var actData = sdbGetIniConf( filePath, false, true );
      checkResult( expData, actData );

      var actData = sdbGetIniConf( filePath, false, false );
      checkResult( expData, actData );

      // setIniConfigs
      sdbSetIniConf( iniData, filePath, true, false );

      var expData = '["[A]","a1=1","a2=\'2\'",""]';
      var actData = readLocalFile( cmd, filePath );
      checkResult( expData, actData );

      // getIniConfigs
      var expData = '{"A.a1":1,"A.a2":"\'2\'"}';
      var actData = sdbGetIniConf( filePath, true, true );
      checkResult( expData, actData );

      var expData = '{"A.a1":1,"A.a2":"2"}';
      var actData = sdbGetIniConf( filePath, true, false );
      checkResult( expData, actData );

      var expData = '{"A.a1":"1","A.a2":"\'2\'"}';
      var actData = sdbGetIniConf( filePath, false, true );
      checkResult( expData, actData );

      var expData = '{"A.a1":"1","A.a2":"2"}';
      var actData = sdbGetIniConf( filePath, false, false );
      checkResult( expData, actData );

      // clear local data
      println( "\n---Begin to clear local data\n" );
      cmd.run( "rm -f " + filePath );


      // sdbcm test
      // setIniConfigs
      sdbcmSetIniConf( oma, iniData, filePath, true, null );

      var expData = '["[A]","a1=1","a2=2",""]';
      var actData = readRemoteFile( remote, filePath );
      checkResult( expData, actData );

      var expData = '{"A.a1":1,"A.a2":2}';
      var actData = sdbcmGetIniConf( oma, filePath, true, true );
      checkResult( expData, actData );

      // setIniConfigs
      sdbcmSetIniConf( oma, iniData, filePath, false, null );

      var expData = '["[A]","a1=1","a2=2",""]';
      var actData = readRemoteFile( remote, filePath );
      checkResult( expData, actData );

      var expData = '{"A.a1":"1","A.a2":"2"}';
      var actData = sdbcmGetIniConf( oma, filePath, false, false );
      checkResult( expData, actData );

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

function sdbSetIniConf ( data, filePath, enableType, strDelimiter )
{
   println( "\n---Begin to exec setIniConfigs with sdb, paramVal[" + enableType + "," + strDelimiter + "]" );
   Oma.setIniConfigs( data, filePath, { "EnableType": enableType, "StrDelimiter": strDelimiter } );
}

function sdbGetIniConf ( filePath, enableType, strDelimiter )
{
   println( "\n---Begin to exec getIniConfigs with sdb, paramVal[" + enableType + "," + strDelimiter + "]" );
   var rc = Oma.getIniConfigs( filePath, { "EnableType": enableType, "StrDelimiter": strDelimiter } );
   return JSON.stringify( rc.toObj() );
}

function sdbcmSetIniConf ( oma, data, filePath, enableType, strDelimiter )
{
   println( "\n---Begin to exec setIniConfigs with sdbcm, paramVal[" + enableType + "," + strDelimiter + "]" );
   oma.setIniConfigs( data, filePath, { "EnableType": enableType, "StrDelimiter": strDelimiter } );
}

function sdbcmGetIniConf ( oma, filePath, enableType, strDelimiter )
{
   println( "\n---Begin to exec getIniConfigs with sdbcm, paramVal[" + enableType + "," + strDelimiter + "]" );
   var rc = oma.getIniConfigs( filePath, { "EnableType": enableType, "StrDelimiter": strDelimiter } );
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