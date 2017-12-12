/*******************************************************************************
*@Description : [BUG] when run 'db.default.tt.find().sort({"":"a"});' command,
*                     sequoiadb process(data node) while core dump. [jira_523].
*@Modify list :
*               2015-2-10  xiaojun Hu  Init
*******************************************************************************/

function main( db )
{
   var insertNum = 100 ;
   var cl = commCreateCL( db, COMMCSNAME, COMMCLNAME, 0, true, true, false,
                          "failed to create collection in the beignning" ) ;
   // insert data
   idxAutoGenData( cl, insertNum) ;
   println( "success to insert data, number = " + insertNum ) ;

   // Test Point
   try
   {
      var list = cl.find().sort( {"":"a"} ) ;
      println( "get query sort: " + list ) ;
      throw "success should not be" ;
   }
   catch( e )
   {
      if( -6 != e )
      {
         println( "failed to test command: 'cl.find().sort( {\"\":\"a\"} )'. " + e ) ;
         throw e ;
      }
      else
      {
         println( "success to test: 'cl.find().sort( {\"\":\"a\"} )'" ) ;
      }
   }
}

// Main
try
{
   commDropCL( db, COMMCSNAME, COMMCLNAME, true, true,
               "clear collection in the beginning" ) ;
   main( db ) ;
   //commDropCL( db, COMMCSNAME, COMMCLNAME, false, false,
   //            "clear collection in the end, correct way" ) ;
   db.close() ;
}
catch( e )
{
   //commDropCL( db, COMMCSNAME, COMMCLNAME, false, false,
   //            "clear collection in the end, wrong way" ) ;
   db.close() ;
   throw e ;
}
