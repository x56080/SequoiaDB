/******************************************************************************
@Description : Test create 5 domain, specify the five group and is not equal.
@Modify list :
               2014-6-17  xiaojun Hu  Init
******************************************************************************/

function main ( db )
{
   // Inspect the run mode
   runMode = inspectRunMode( db );
   if( "standalone" == runMode )
      throw "RunMode_StandAlone";

   // Domain names
   var name = csName + "_DomFiveDiffGroup";
   var domNames = new Array();
   var csname = new Array();
   var clname = new Array();
   for( var i = 0; i < 5; ++i )
   {
      domNames[i] = name + i;
      csname[i] = csName + i;
      clname[i] = clName + i;

      // Drop CS
      commDropCS( db, csname[i], clname[i], true,
         "clear collection space in the beginning" );

      // Drop domain in the beginning
      clearDomain( db, domNames[i] );

   }

   // Get group, inspect group and create domain
   try
   {
      var group = new Array();
      group = getGroup( db );
      if( 5 > group.length )
      {
         println( "Don't have enough group, count = " + group );
         throw "NoEnoughGroup";
      }
      // create 5 domain and group is not equal with each other [Testing Point]
      for( var i = 0; i < 5; ++i )
      {
         createDomain( db, domNames[i], [group[i]] );
         println( "Success to create domain = [" + domNames[i] + "]" );
      }
   }
   catch( e )
   {
      println( "Failed to create domain, rc = " + e );
      throw e;
   }

   // Inspect domain and create CS/CL
   for( var i = 0; i < 5; ++i )
   {
      // Inspect domain
      inspectDomain( db, domNames[i] );

      // Create CS in domain and create collection [Testing Point]
      try
      {
         commCreateCS( db, csname[i], false, "create CS specify domain",
            { "Domain": domNames[i] } );
         commCreateCL( db, csname[i], clname[i], {}, false, false,
            "create collection in domain" );
      }
      catch( e )
      {
         println( "Failed to create CS by specify domain, rc = " + e );
         throw e;
      }
   }

   for( var i = 0; i < 5; ++i )
   {
      println( "********Domain Name : [" + domNames[i] + "]********" );
      // Inspect domain
      inspectDomain( db, domNames[i] );

      // Insert data
      insertData( db, csname[i], clname[i], 2000 );

      // Query data
      queryData( db, csname[i], clname[i] );

      // Update data
      updateData( db, csname[i], clname[i] );

      // Remove data
      removeData( db, csname[i], clname[i] );

      // Drop domain in the end
      clearDomain( db, domNames[i] );
      println( "Success to clean domain : [" + domNames + "] in the end" );
   }
}

try
{
   main( db );
   db.close();
}
catch( e )
{
   if( "NoEnoughGroup" == e )
      println( "Don't have enough groups" );
   else if( "RunMode_StandAlone" == e )
      println( "WARNNING! Run Mode is : [ standalone ]" );
   else
      throw e;
}
