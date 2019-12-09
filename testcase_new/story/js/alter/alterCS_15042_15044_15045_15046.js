/* *****************************************************************************
@discretion: alter cs, the cs exist cl, the test scenario is as follows:
test 15042: only test setDomain to add Domain, cl autosplit does not automate.
test 15044: alter domain, new domain contains the group of cl
test 15045: alter domain, new domain contains the group of cl
test 15046: remove domain
@author��2018-4-27 wuyan  Init
***************************************************************************** */
var csName = CHANGEDPREFIX + "_cs15042";
var clName = CHANGEDPREFIX + "_cs15042";
var domainName1 = CHANGEDPREFIX + "_domain15042";
var domainName2 = CHANGEDPREFIX + "_domain15044";
var domainName3 = CHANGEDPREFIX + "_domain15045";


main( db );
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
      if( 1 === groupNames.length )
      {
         println( "--least two groups" );
         return;
      }

      //clean environment before test
      commDropCS( db, csName, true, "drop cs" );
      commDropDomain( db, domainName1 );
      commDropDomain( db, domainName2 );
      commDropDomain( db, domainName3 );

      //create domain, cs, cl
      commCreateDomain( db, domainName1, [groupNames[0]] );
      commCreateDomain( db, domainName2, groupNames );
      commCreateDomain( db, domainName3, [groupNames[1]] );
      var dbcs = commCreateCS( db, csName, false, "create CS" );
      var dbcl = commCreateCLByOption( db, csName, clName, { Group: groupNames[0] } );

      //testcase15042:add domain to cs
      println( "---add domain to cs " );
      dbcs.setDomain( { Domain: domainName1 } );
      checkAlterCSResult( csName, "Domain", domainName1 );


      //testcase15044:alter domain, new domain contains the group of cl
      println( "---alter domain to cs" );
      dbcs.setDomain( { Domain: domainName2 } );
      checkAlterCSResult( csName, "Domain", domainName2 );

      //testcase15045:alter domain, new domain contains the group of cl
      println( "---alter domain to cs, alter fail" );
      alterDomainOfCSError( dbcs, domainName3 );
      checkAlterCSResult( csName, "Domain", domainName2 );


      //testcase15046:remove domain
      println( "---remove domain" );
      dbcs.removeDomain();
      checkRemoveDomainResult( csName );

      //clean
      commDropCS( db, csName, true, "clear cs" );
      commDropDomain( db, domainName1 );
      commDropDomain( db, domainName2 );
      commDropDomain( db, domainName3 );
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

function alterDomainOfCSError ( dbcs, domainName )
{
   try
   {
      dbcs.setDomain( { Domain: domainName } );
      throw "need throw error";
   }
   catch( e )
   {
      if( e != -216 )
      {
         throw buildException( "cannot be alter domian:", e );
      }
   }
}

function checkRemoveDomainResult ( csName )
{
   try
   {
      var rg = db.getRG( "SYSCatalogGroup" );
      var dbca = new Sdb( rg.getMaster() );
      var cur = dbca.SYSCAT.SYSCOLLECTIONSPACES.find( { "Name": csName } );
      while( cur.next() )
      {
         var tempinfo = cur.current().toObj();
      }
      var csInfo = JSON.stringify( tempinfo );

      //domain no exists return -1
      if( -1 !== csInfo.search( "Domain" ) )
      {
         throw buildException( "test domain", "check domain", "", "no exist doamin", csInfo.search( "Domain" ) );
      }
   }
   catch( e )
   {
      throw buildException( "check alter cs result:", e );
   }
   finally
   {
      if( dbca != null )
      {
         dbca.close()
      }
   }
}


