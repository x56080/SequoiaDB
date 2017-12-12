/*******************************************************************************
*@Description : test query when using regex option.
*               [ Revision: 15549, JIRA: SEQUOIADBMAINSTREAM-332]
*@Modify list :
*               2014-6-20  xiaojun Hu  Init
*******************************************************************************/
function main( db )
{
   var cl = commCreateCL( db, COMMCSNAME, COMMCLNAME, -1, true, true, false,
                          "create collection in the beginning" ) ;
   //insert data
   var queryNum = 0 ;
   var begVal = "a" ;
   var endVal = "z" ;
   var midVal = "m" ;
   for( var i = begVal.charCodeAt(0) ; i <= endVal.charCodeAt(0) ; ++i )
   {
      var urlVal = String.fromCharCode(i) ;
      //println( "char :" + urlVal ) ;
      if( i < midVal.charCodeAt(0) )
      {
         cl.insert( { "url": "http://test." + urlVal + ".com" } ) ;
         queryNum++ ;
      }
      else
         cl.insert( { "url": "http://test." + urlVal + ".cn" }  ) ;
   }
   var cnt = 0 ;
   do
   {
      cnt ++ ;
      sleep( 1 ) ;
   }while( 26 != cl.count() || cnt < 1000 ) ;
   if( 26 != cl.count() )
   {
      println( "query record numbers = " + cl.count() ) ;
      throw "ErrInsert" ;
   }
   println( "insert " + cl.count() + " records successful" ) ;

   // query by use regex
   try
   {
      var regCount1 = cl.count( {"url":{"$options":"",
                                        "$regex":"http://test.*.com*"}} ) ;
      var regCount2 = cl.count( {"url":{"$regex":"http://test.*.com*",
                                        "$options":""}} ) ;
      if( parseInt(regCount1) != parseInt(regCount2) )
      {
         println( "regex query1 = " + regCount1 + " not equal regex query2 = "
                  + regCount2 ) ;
         throw "ErrRegexQuery" ;
      }
      if( queryNum != regCount1 )
      {
         println( "regex query 1 = " + regCount1 + ", and regex query 2 = "
                  + regCount2 + " not equal " + queryNum ) ;
         throw "ErrQueryNumber" ;
      }
      println( "regex query suceessful, records = " + queryNum ) ;
   }
   catch( e )
   {
      throw e ;
   }
}

// Run Main
try
{
   commDropCL( db, COMMCSNAME, COMMCLNAME, true, true,
               "clean collecition in the beginning" ) ;
   main( db ) ;
   commDropCL( db, COMMCSNAME, COMMCLNAME, false, false,
               "clear collection in the end" ) ;
   db.close() ;
}
catch( e )
{
   commDropCL( db, COMMCSNAME, COMMCLNAME, false, false,
               "clear collection in the end" ) ;
   db.close() ;
   throw e ;
}
