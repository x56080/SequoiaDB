/******************************************************************************
@Description : 1. Test dom.alter(<options>), specify {AutoSplit:true}.
@Modify list :
               2014-6-18  xiaojun Hu  Init
******************************************************************************/

function main ( db )
{
   // Inspect run mode
   runMode = inspectRunMode( db );
   if( "standalone" == runMode )
      throw "RunMode_StandAlone";

   var domName = csName + "_DomAlterAutoSplit";
   // Clear domain in the beginning
   clearDomain( db, domName );
   println( "Clear domain in the beginning" );

   // Drop Collection space in the beginning
   commDropCS( db, csName, true,
      "clear collection space in the beginning" );

   // Create domain without group and autosplit
   createDomain( db, domName );

   // Alter the group to domain [Testing Point]
   try
   {
      var group = new Array();
      group = getGroup( db );
      //println( "Get Groups = " + group ) ;
      dom = db.getDomain( domName );
      dom.alter( { "Groups": group, AutoSplit: true } );
      //alterDomain( db, domName, { Groups : group , AutoSplit : true } ) ;
      //alterDomain( db, domName, { Groups : group } ) ;
   }
   catch( e )
   {
      println( "Failed to alter domain group, rc = " + e );
      throw e;
   }
   // Create collection space and collection
   commCreateCS( db, csName, false, "create CS specify domain",
      { "Domain": domName } );
   commCreateCL( db, csName, clName, {
      ShardingKey: { "No": -1 },
      ShardingType: "hash", Partition: 1024
   },
      false, false, "create collection in domain" );

   // Inspect data to SDB
   insertData( db, csName, clName, 1000 )
   println( "Success to insert 1000 records" );

   // inspect the AutoSplit is take effect or not
   inspectAutoSplit( db, csName, clName, domName )
   println( "Success to inspect the autosplit parameter" );

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
