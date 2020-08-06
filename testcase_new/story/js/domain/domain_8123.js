/******************************************************************************
@Description : 1.Test dom.listCollectionSpace().create domain specify all groups.
               2.Test db.createCS("foo",{Domain:"domainName"})
@Modify list :
               2014-6-18  xiaojun Hu  Init
******************************************************************************/

function main ( db )
{
   // Inspect the domain name
   runMode = inspectRunMode( db );
   if( "standalone" == runMode )
      throw "RunMode_StandAlone";

   domName = csName + "_DomAllGroupsListCS";

   // Clear domain in the beginning
   clearDomain( db, domName );
   println( "Clear domain in the beginning" );

   // Drop Collection space in the beginning
   commDropCS( db, csName, true,
      "clear collection space in the beginning" );

   // Create domain by specify groups
   try
   {
      var group = new Array();
      group = getGroup( db );
      //println( "Get Groups = " + group ) ;
      createDomain( db, domName, group, { AutoSplit: true } );
   }
   catch( e )
   {
      println( "Failed to create domain group, rc = " + e );
      throw e;
   }

   // Create collection space and collection[Testing Point]
   commCreateCS( db, csName, false, "create CS specify domain",
      { "Domain": domName } );
   commCreateCL( db, csName, clName, {
      ShardingKey: { "No": -1 },
      ShardingType: "hash", Partition: 1024,
      ReplSize: 0
   },
      false, false, "create collection in domain" );

   // domain list collections and inspect [Testing Point]
   var dom = getDomain( db, domName );
   var domCSname = dom.listCollectionSpaces().current().toObj()["Name"];
   if( csName != domCSname )
   {
      println( "Failed to list collection spaces , domListCS = " + typeof ( domCSname ) );
      throw "ErrDomListCL";
   }
   println( "Success to list and inspect collection using domain" );


   // Clear domain in the end
   clearDomain( db, domName );
   println( "Clear domain in the end" );

}

try
{
   main( db );
   db.close();
}
catch( e )
{
   if( "RunMode_StandAlone" != e )
      throw e;
   else
      println( "WARNNING! Run Mode is : [ standalone ]" );
}
