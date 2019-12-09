/******************************************************************************
@Description : 1. Test db.createDomain(<name>,[option]).
               2. Test db.createCS() specify domain, CS locate in domain
                  group is correct or not .
@Modify list :
               2014-6-17  xiaojun Hu  Init
******************************************************************************/
function main ( db )
{
   var CSNAME = COMMCLNAME + "_csInGroup";
   var domName = COMMCLNAME + "_createCSInGroup"
   var groups = commGetGroups( db );
   for( var i = 0; i < groups.length; ++i )
   {
      var csRg = groups[i][0].GroupName;
      commDropCS( db, CSNAME, true, "clear environmen in the beginning" );
      try
      {
         db.dropDomain( domName );
      }
      catch( e )
      {
         if( -214 != e && -34 != e )
         {
            println( "failed to drop domain in beginnig, rc = " + e );
            throw e;
         }
      }
      try
      {
         db.createDomain( domName, [csRg] );
         var varCS = db.createCS( CSNAME, { "Domain": domName } );
         println( "success to create collection space" );
         // need to create cl, because when cs has no cl, the cs don't create in data group
         var cl = varCS.createCL( COMMCLNAME );
         println( "create collection successful" );
      }
      catch( e )
      {
         println( "failed to create domain and create CS, rc = " + e );
         throw e;
      }
      // Inspect the CS is located in
      db.invalidateCache();  // clean coord and data node cache
      var csGroups = commGetCSGroups( db, CSNAME );
      var csGroupWrong = false;
      if( csGroups.length > 1 || csGroups != csRg )
      {
         println( db.listDomains( { "Name": domName } ) );
         println( db.snapshot( 5, { "Name": CSNAME } ) );
         println( "expect cs group: " + csRg +
            ", actural cs group: " + csGroups );
         //throw "error, create CS located in wrong group" ;
         csGroupWrong = true;
         // sleep 10000ms, see the group that cs located in is changed or not
         sleep( 10000 );
         println( " =======< After sleep 10000 microseconds >=======" );
      }
      db.invalidateCache();  // clean coord and data node cache
      csGroups = commGetCSGroups( db, CSNAME );
      var retryTimes = 10;
      while( csGroups.length > 1 && 0 != retryTimes-- )
      {
         sleep( 3000 );
         csGroups = commGetCSGroups( db, CSNAME );
      }
      if( csGroups.length > 1 || csGroups != csRg )
      {
         println( "Warning: db.snapshot(5) may read from slave date node, " +
            "so, the info it showed may not up to date." )
         println( db.listDomains( { "Name": domName } ) );
         println( db.snapshot( 5, { "Name": CSNAME } ) );
         println( "expect cs group: " + csRg +
            ", actural cs group: " + csGroups );
         throw "error, create CS located in wrong group";
      }
      if( true == csGroupWrong )
      {
         println( db.listDomains( { "Name": domName } ) );
         println( db.snapshot( 5, { "Name": CSNAME } ) );
      }
   }
   // Clear the envioronment
   commDropCS( db, CSNAME, false, "clear environmen in the end" );
   try
   {
      db.dropDomain( domName );
   }
   catch( e )
   {
      println( "failed to drop domain in the end, rc = " + e );
      throw e;
   }
}
// Run Main
try
{
   if( false == commIsStandalone( db ) )
      main( db );
   else
      println( "run mode is : standalone" );
}
catch( e )
{
   throw e;
}
