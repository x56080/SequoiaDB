/******************************************************************************
@Description : 1. Test db.createDomain(<name>,[option]).
               2. Test db.createCS() specify domain, CS locate in domain
                  group is correct or not .
@Modify list :
               2014-6-17  xiaojun Hu  Init
******************************************************************************/
testConf.skipStandAlone = true;
main( test );
function test ()
{
   var CSNAME = COMMCLNAME + "_8118";
   var domName = COMMCLNAME + "_8118"
   var groups = commGetGroups( db );
   for( var i = 0; i < groups.length; ++i )
   {
      var csRg = groups[i][0].GroupName;
      commDropCS( db, CSNAME, true, "clear environmen in the beginning" );
      commDropDomain( db, domName );
      db.createDomain( domName, [csRg] );
      var varCS = db.createCS( CSNAME, { "Domain": domName } );
      // need to create cl, because when cs has no cl, the cs don't create in data group
      var cl = varCS.createCL( COMMCLNAME );

      // Inspect the CS is located in
      db.invalidateCache();  // clean coord and data node cache
      var csGroups = commGetCSGroups( db, CSNAME );
      var csGroupWrong = false;
      if( csGroups.length > 1 || csGroups != csRg )
      {
         //throw "error, create CS located in wrong group" ;
         csGroupWrong = true;
         // sleep 10000ms, see the group that cs located in is changed or not
         sleep( 10000 );
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
         throw new Error( "error, create CS located in wrong group" );
      }
      if( true == csGroupWrong )
      {
      }
   }
   // Clear the envioronment
   commDropCS( db, CSNAME, false, "clear environmen in the end" );
   db.dropDomain( domName );
}