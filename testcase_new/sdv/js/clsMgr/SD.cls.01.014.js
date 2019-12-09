/* *****************************************************************************
@Description: build catalog group and check its status
@author:      LY
***************************************************************************** */

function deploySdb ( tmpDb, hostname )
{
   if( undefined === tmpDb ) 
   {
      throw ( "the db is null, error" );
   }
   if( undefined === hostname ) 
   {
      throw ( "the hostname is null, error" );
   }

   // build catalog
   var catalog = generatePort( SPAREPORTSTOP );
   try
   {
      tmpDb.createCataRG( hostname, catalog, SPAREPORTPATH + catalog );
      return catalog;
   }
   catch( e )
   {
      throw buildException(
         "build catalog",
         e,
         "db.createCataRG( " + COORDHOSTNAME + ", " + catalog + ", " + SPAREPORTPATH + catalog + " )",
         "succ",
         "failure"
      );
   }
}

function checkResult ( tmpDb )
{
   if( undefined === tmpDb ) 
   {
      throw ( "the db is null, error" );
   }

   var ret = tmpDb.getCatalogRG().getDetail().current()
   if( 1 !== ret.toObj()["Status"] )
   {
      throw buildException(
         "check catalog catalog",
         "catalog status error",
         "db.getCatalogRG().getDetail().current().toObj()['Status'] not equal 1",
         1,
         ret.toObj()["Status"]
      );
   }
}

function main ( db )
{
   try
   {
      // generate temp coord port
      var tmpCoordPort = generatePort( SPAREPORTSTART );
      // get new cluster used hostname 
      var hostname = getHostName();
      // build temp coord and connect it
      var tmpDb = connectSdb( COORDHOSTNAME, CMSVCNAME, tmpCoordPort, false );
      // create catalog group 
      var catalog = deploySdb( tmpDb, hostname );
      // check catalog which creatting status
      checkResult( tmpDb );
      // clear catalog after test correct
      clean( tmpDb, hostname, catalog, "", "catalog_group" );
      // clear temp coord after test correct
      clean( tmpDb, hostname, tmpCoordPort, "", "tmp_coord" );
   }
   catch( e )
   {
      throw e;
   }
   finally
   {
      tmpDb.close();
      db.close();
   }
}


// run main
main( db );