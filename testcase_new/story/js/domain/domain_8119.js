/******************************************************************************
@Description : 1. Test dom.alter(<options>), specify alter Groups.
@Modify list :
               2014-6-18  xiaojun Hu  Init
******************************************************************************/

function main ( db )
{
   // Inspect run mode
   runMode = inspectRunMode( db );
   if( "standalone" == runMode )
      throw "RunMode_StandAlone";

   var domName = csName + "_DomAlterSpecifyGroup";

   // Clear domain in the beginning
   clearDomain( db, domName );
   println( "Clear domain in the beginning" );

   // Alter the group to domain [Testing Point]
   try
   {
      var group = new Array();
      group = getGroup( db );
      //println( "Get Groups = " + group ) ;

      // Create domain without group and autosplit
      createDomain( db, domName );
      println( "Success to create domain" );

      //println( "Get Groups = " + group ) ;
      dom = db.getDomain( domName );

      dom.alter( { Groups: group } );

   }
   catch( e )
   {
      println( "Failed to alter domain group, rc = " + e );
      throw e;
   }

   // inspect the alter
   println( "Begin to inspect the alter group : [ " + group + " ]" );
   for( var i = 0; i < group.length; ++i )
   {
      inspectAlter( db, group[i], domName );
      println( "group in : " + group[i] );
   }

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
