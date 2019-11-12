/*******************************************************************
*@Description : test use import importOnce in procedure
*               seqDB-12816:在存储过程中调用import importOnce
*@author      : Liang XueWang 
*******************************************************************/
// js file to import/importOnce in procedure, no permission
var noPermFile     = WORKDIR + "/noPermFile_12816.js" ;  

main();
function main()
{
   if( commIsStandalone( db ) )
   {
      println( "Run mode is Standalone, can't use procedure" ) ;
      return ;
   }
   if( getCoordUser() === "root" )
   {
      println( COORDHOSTNAME + ":" + COORDSVCNAME + " user is root" ) ;
      return ;
   }

   // creat import importOnce file
   createNoPermFile() ;

   // create procedure to import file and test
   try
   {
      checkProcedure( "testImportNoPermFile12816" );
      db.createProcedure( function testImportNoPermFile12816( file ) { return import( file ) } ) ;
      db.eval( "testImportNoPermFile12816( \"" + noPermFile + "\" )" ) ;
      throw 0 ;
   }
   catch( e )
   {
      if( e !== -3 )
      { 
         throw buildException( "main", e, 
               "test import file " + noPermFile + " in procedure", -3, e ) ;    
      }
   }
   db.removeProcedure( "testImportNoPermFile12816" ) ;

   // create procedure to importOnce file and test
   try
   {
      checkProcedure( "testImportOnceNoPermFile12816" );
      db.createProcedure( function testImportOnceNoPermFile12816( file ) { return importOnce( file ) } ) ;
      db.eval( "testImportOnceNoPermFile12816( \"" + noPermFile + "\" )" ) ;
      throw 0 ;
   }
   catch( e )
   {
      if( e !== -3 )
      {
         throw buildException( "main", e, 
               "test importOnce file " + noPermFile + " in procedure", -3, e ) ;
      }    
   }
   db.removeProcedure( "testImportOnceNoPermFile12816" ) ; 
   
   // remove file
   var remote = new Remote(COORDHOSTNAME, CMSVCNAME);
   var cmd = remote.getCmd();
   cmd.run("rm -rf " + noPermFile);
}

function createNoPermFile()
{
   try
   {
      var remote = new Remote(COORDHOSTNAME, CMSVCNAME);
      var cmd = remote.getCmd();
      cmd.run( "rm -rf " + noPermFile ) ;
      initWorkDir( cmd, remote );
      var file = remote.getFile(noPermFile, 0744, SDB_FILE_CREATE | SDB_FILE_READWRITE);
      file.write( "1+2" ) ;
      file.close() ;
      file.chmod( noPermFile, 0000 ) ;
   }
   catch( e )
   {
      throw buildException( "createNoPermFile", null, 
            "create file " + noPermFile, 0, e ) ;
   }
}

function checkProcedure( procedureName )
{
    var cursor = db.listProcedures({name:procedureName});
    if(cursor.next())
    {
        db.removeProcedure( procedureName ) ;
    }
}
