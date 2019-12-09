/*******************************************************************************
*@Description : test query by use sql and like. [jira_330].
*@Modify list :
*               2014-11-10  xiaojun Hu  Init
*******************************************************************************/

function main ( db )
{
   var indexName = CHANGEDPREFIX + "idx";
   var cl = commCreateCL( db, COMMCSNAME, COMMCLNAME, 0, true, true, false,
      "failed to create collection in the beignning" );
   // insert data
   var begVal = "A";
   var endVal = "Z";
   for( var i = begVal.charCodeAt( 0 ); i <= endVal.charCodeAt( 0 ); ++i )
   {
      var urlVal = String.fromCharCode( i );
      //println( "char :" + urlVal ) ;
      cl.insert( { "regex": "abcdefg" + urlVal + "test" } );
   }
   var cnt = 0;
   do
   {
      cnt++;
      sleep( 1 );
   } while( 26 != cl.count() || cnt < 1000 );
   if( 26 != cl.count() )
   {
      println( "query record numbers = " + cl.count() );
      throw "ErrInsert";
   }
   println( "insert " + cl.count() + " records successful" );
   // create index
   try
   {
      cl.createIndex( indexName, { "regex": 1 } );
   }
   catch( e )
   {
      println( "failed to create index, rc = " + e );
      throw e;
   }

   // query by using regex , run db.exec( <sql string> ) ;
   try
   {
      var clName = COMMCSNAME + "." + COMMCLNAME;
      db.setSessionAttr( { PreferedInstance: "M" } );
      var idxRead1 = queryGetCurrentSessions( db, clName );
      // cannot query data
      var queryNum = cl.find( {
         "regex": {
            "$regex": "^abcdefg*Q",
            "$options": "i"
         }
      } ).toArray();
      var idxRead2 = queryGetCurrentSessions( db, clName );

      if( 1 != queryNum.length )
      {
         println( "the number of query by using regex : " + queryNum.length );
         throw "ErrQueryNum";
      }
      if( idxRead2[1] != idxRead1[1] && idxRead2[0] <= idxRead1[0] )
      {
         println( "previous index read : " + idxRead1[1] +
            " back index read : " + idxRead2[1] );
         println( "previous data read : " + idxRead1[0] +
            " back data read : " + idxRead2[0] );
         throw "don't query from index, error";
      }
      println( '<cl.find( {"regex":{"$regex":"^abcdefg*Q", "$options":"i"}})>' );
      // query 1 record
      var queryNum1 = cl.find( { "regex": { "$regex": "^abcdefg*G" } } ).toArray();
      var idxRead3 = queryGetCurrentSessions( db, clName );
      if( 1 != queryNum1.length )
      {
         println( "the number of query by using regex : " + queryNum1.length );
         throw "ErrQueryNum1";
      }
      if( ( idxRead3[1] - idxRead2[1] ) != 26 && ( idxRead3[0] - idxRead2[0] ) != 26 )
      {
         println( "previous index read : " + idxRead2[1] +
            " back index read : " + idxRead3[1] );
         println( "previous data read : " + idxRead2[0] +
            " back data read : " + idxRead3[0] );
         throw "don't query from index, error1";
      }
      println( '<cl.find( {"regex":{"$regex":"^abcdefg*G"}})>' );
      // query many record
      var queryNum2 = cl.find( { "regex": { "$regex": "\\AabcdefgG" } } ).toArray();
      var idxRead4 = queryGetCurrentSessions( db, clName );

      if( 1 != queryNum2.length )
      {
         println( "the number of query by using regex : " + queryNum2.length );
         throw "ErrQueryNum2";
      }
      if( idxRead4[1] <= idxRead3[1] && idxRead4[0] <= idxRead3[0] )
      {
         println( "previous index read : " + idxRead3[1] +
            " back index read : " + idxRead4[1] );
         println( "previous data read : " + idxRead3[0] +
            " back data read : " + idxRead4[0] );
         throw "don't query from index, error2";
      }
      println( '<cl.find( {"regex":{"$regex":"\\AabcdefgG"}})>' );
      // query many record and use $or
      var queryNum3 = cl.find( {
         "$or": [{ "regex": { "$gt": "abcdefgG" } },
         { "regex": { "$regex": "\\AabcdefgG" } }]
      } ).toArray();
      var idxRead5 = queryGetCurrentSessions( db, clName );

      if( 20 != queryNum3.length )
      {
         println( "the number of query by using regex : " + queryNum3.length );
         throw "ErrQueryNum3";
      }
      // when use $or, query will not acrocc index
      if( idxRead5[1] != idxRead4[1] && idxRead5[0] <= idxRead4[0] )
      {
         println( "previous index read : " + idxRead4[1] +
            " back index read : " + idxRead5[1] );
         println( "previous data read : " + idxRead4[0] +
            " back data read : " + idxRead5[0] );
         throw "don't query from index, error3";
      }
      println( '<cl.find( {"$or":[{"regex":{"$gt":"abcdefgG"}},' +
         '{"regex":{"$regex":"\\AabcdefgG"}}]})>' );
   }
   catch( e )
   {
      println( "failed to query by using regex, rc = " + e );
      throw e;
   }
}

// Main
try
{
   commDropCL( db, COMMCSNAME, COMMCLNAME, true, true,
      "clear collection in the beginning" );
   main( db );
   commDropCL( db, COMMCSNAME, COMMCLNAME, false, false,
      "clear collection in the end, correct way" );
   db.close();
}
catch( e )
{
   commDropCL( db, COMMCSNAME, COMMCLNAME, false, false,
      "clear collection in the end, wrong way" );
   db.close();
   throw e;
}
