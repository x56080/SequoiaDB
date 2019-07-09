/******************************************************************************
@Description : Test create 10 domain,the domains' group is equal with each other.
@Modify list :
               2014-6-24  xiaojun Hu  Init
******************************************************************************/

function main( db )
{
   // Inspect the run mode
   var runMode = inspectRunMode( db ) ;
   if( "standalone" == runMode )
      throw "RunMode_StandAlone" ;

   var domName = new Array() ;
   var csname = new Array() ;
   var clname = new Array() ;
   var name = csName + "_DomFiveSameGroup" ;
   for( var i = 0 ; i < 10 ; ++i )
   {
      domName[i] = name + i ;
      csname[i] = csName + i ;
      clname[i] = clName + i ;

      // Drop CS
      commDropCS( db, csname[i], clname[i], true,
                  "clear collection space in the beginning" ) ;

      // Drop domain
      clearDomain( db, domName[i] ) ;
   }

   // Get groups and create domain [Testing Point]
   try
   {
      var group = new Array() ;
      group = getGroup( db ) ;
      println( "Get Group :" + group ) ;
      // create 10 domains and specify same groups [Testing point]
      for( var i = 0 ; i < 10 ; ++i )
      {
         //createDomain( db, domName[i], group ) ;
         db.createDomain( domName[i], group ) ;
         println( "Success to create Domain : [" + domName[i] + "]" +
                  "Specify group : [" + group + "]" ) ;
      }
   }
   catch ( e )
   {
      if( -1109 != e )
      {
         println( "Failed to create domain, rc = " + e ) ;
         throw e ;
      }
      else
         println( "The mode is standalong, not group" ) ;
   }

   // inspect domain and create CS/CL
   for( var i = 0 ; i < 10 ; ++i )
   {
      // Inspect domain
      inspectDomain( db, domName[i] ) ;

      // Create CS in domain and create collection
      try
      {
         commCreateCS( db, csname[i], false, "create CS specify domain",
                       { "Domain" : domName[i] } ) ;
         commCreateCL( db, csname[i], clname[i], -1, true, false, false,
                       "create collection in domain" ) ;
      }
      catch ( e )
      {
            println( "Failed to create CS by specify domain, rc = " + e ) ;
            throw e ;
      }
   }

   for( var i = 0 ; i < 10 ; ++i )
   {
      println( "||***** Domain Name : [" + domName[i] + "]*****||" ) ;
      // Insert data
      insertData( db, csname[i], clname[i], 1000 ) ;

      // Query data
      queryData( db, csname[i], clname[i] ) ;

      // Update data
      updateData( db, csname[i], clname[i] ) ;

      // Remove data
      removeData( db, csname[i], clname[i] ) ;

      // Clear domain in the end and drop cs
      clearDomain( db, domName[i] ) ;
      println( "Success to clean domain : [" + domName[i] + "] in the end" ) ;
   }
}

try
{
   main( db ) ;
   db.close() ;
}
catch ( e )
{
   if( "RunMode_StandAlone" != e )
      throw e ;
   else
      println( "WARNNING! Run Mode is : [ standalone ]" ) ;
}
