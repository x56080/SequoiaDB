/*******************************************************************************
*@Description : test query by use sort and skip function. [jira_503].
*               such as: db.foo.bar.find().sort({a:1}).skip(101)
*@Modify list :
*               2014-11-10  xiaojun Hu  Init
*******************************************************************************/

function main( db )
{
   var indexName = CHANGEDPREFIX + "idx" ;
   var insertNum = 100 ;
   var cl = commCreateCL( db, COMMCSNAME, COMMCLNAME, 0, true, true, false,
                          "failed to create collection in the beignning" ) ;
   // insert data
   idxAutoGenData( cl, insertNum) ;
   println( "success to insert data, number = " + insertNum ) ;
   // query by using sort and skip function
   var sort = new Array( 1, -1 ) ;
   for( var n = 0 ; n < sort.length ; ++n )
   {
      for( var i = 0 ; i < insertNum ; ++i )
      {
         var querySort = cl.find().sort({no: sort[n] }) ;
         var query = querySort.skip(i).toArray();
         var queryNum = query.length ;
         var tempNum = insertNum - i ;
         if( tempNum != queryNum )
         {
            println( "skip number:" + i + ", expect number: " + tempNum +
                     ", actual number: " + queryNum ) ;
            throw "ErrSortSkipNum" ;
         }
      }
   }
   println( "success to query by using sort and skip" +
            " function when don't have index" ) ;

   // create Index
   var idxName = "noIndex" ;
   var indexDef = { "no":1, "no1":1, "no2":-1, "no3":1 } ;
   commCreateIndex( cl, idxName, indexDef, false, false ) ;
   commCheckIndex( cl, idxName, true ) ;
   println( "create index successful" ) ;

   // query by using sort and skip function. and create index
   var sort = new Array( 1, -1 ) ;
   for( var n = 0 ; n < sort.length ; ++n )
   {
      for( var i = 0 ; i < insertNum ; ++i )
      {
         var query = cl.find().sort({no: sort[n]}).skip(i).toArray() ;
         var queryNum = query.length ;
         var tempNum = insertNum - i ;
         if( tempNum != queryNum )
         {
            println( "skip number:" + i + ", expect number: " + tempNum +
                     ", actual number: " + queryNum ) ;
            throw "ErrSortSkipNumIdx" ;
         }
      }
   }
   println( "success to query by using sort and skip" +
            " function while having index" ) ;
}

// Main
try
{
   commDropCL( db, COMMCSNAME, COMMCLNAME, true, true,
               "clear collection in the beginning" ) ;
   main( db ) ;
   commDropCL( db, COMMCSNAME, COMMCLNAME, false, false,
               "clear collection in the end, correct way" ) ;
   db.close() ;
}
catch( e )
{
   commDropCL( db, COMMCSNAME, COMMCLNAME, false, false,
               "clear collection in the end, wrong way" ) ;
   db.close() ;
   throw e ;
}
