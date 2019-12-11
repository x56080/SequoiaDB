/* *****************************************************************************
@discretion: alter cs name/pageSize/lobpagesize/domain
@author��2018-4-28 wuyan  Init
***************************************************************************** */
var csName = CHANGEDPREFIX + "_cs15049";
var domainName = CHANGEDPREFIX + "_domain15049";

try
{
   main( db );
}
catch( e )
{
   if( e.constructor === Error )
   {
      println( e.stack );
   }
   throw e;
}

function main ( db )
{
   try
   {
      if( true == commIsStandalone( db ) )
      {
         println( "run mode is standalone" );
         return;
      }
      var groupNames = getGroupName( db );

      //clean environment before test
      commDropCS( db, csName, true, "drop cs" );
      commDropDomain( db, domainName );

      //create domain, cs, cl
      commCreateDomain( db, domainName, [groupNames[0]] );
      var dbcs = commCreateCS( db, csName, false, "create CS" );

      //alter pageSize /domain/lobpageSize
      println( "---alter pagesize/lobpageSize/Domain " );
      var pageSize = 16384;
      var lobpageSize = 131072;
      dbcs.alter( { PageSize: pageSize, LobPageSize: lobpageSize, Domain: domainName } );
      checkAlterCSResult( csName, "PageSize", pageSize );
      checkAlterCSResult( csName, "Domain", domainName );
      checkAlterCSResult( csName, "LobPageSize", lobpageSize );

      //alter cs name
      println( "---alter cs name" );
      alterCSNameError( dbcs )
      checkAlterCSResult( csName, "Name", csName );

      //clean
      commDropCS( db, csName, true, "clear cs" );
      commDropDomain( db, domainName );
   }
   catch( e )
   {
      throw e;
   }
   finally
   {
      if( db != null )
      {
         db.close()
      }
   }
}

function alterCSNameError ( dbcs )
{
   try
   {
      dbcs.alter( { Name: "testcs" } );
      throw new Error( "need throw error" );
   }
   catch( e )
   {
      if( e.message != -32 )
      {
         throw e;
      }
   }
}

