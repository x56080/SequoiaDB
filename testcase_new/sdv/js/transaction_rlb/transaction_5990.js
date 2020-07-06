/*******************************************************************************
*@Description: seqDB-5990:事务功能关闭，执行事务操作
*@Author:      2020-4-25 Zhao Xiaoni
********************************************************************************/
testConf.skipStandAlone = true;

main( test );

function test()
{
   var db = new Sdb( COORDHOSTNAME, COORDSVCNAME );
   try
   {
      db.updateConf( { "transactionon": false } );
      throw "Excute updateConf should be failed!";
   }
   catch( e )
   {
      if( e !== -264 )
      {
         throw new Error( e );
      }
   }
   restartCoord();

   db = new Sdb( COORDHOSTNAME, COORDSVCNAME );
   try
   {
      db.transBegin();
      throw "Excute transBegin should be failed!";
   }
   catch( e )
   {
      if( e !== -253 )
      {
         throw new Error( e );
      }
   }

   try
   {
      db.deleteConf( { "transactionon": 1 } );
      throw "Excute deleteConf should be failed!";
   }
   catch( e )
   {
      if( e !== -264 )
      {
         throw new Error( e );
      }
   }
 
   restartCoord();
}

function restartCoord()
{
   var remote = new Remote( COORDHOSTNAME, CMSVCNAME );
   var cmd = remote.getCmd();
   var installDir = commGetInstallPath();
   var command = installDir + "/bin/sdbstop -p " + COORDSVCNAME;
   cmd.run( command );

   command = installDir + "/bin/sdbstart -p " + COORDSVCNAME;
   cmd.run( command );
}
