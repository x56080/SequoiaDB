
function loadData ( cl )
{
   try
   {
      cl.insert( { "a": { "$timestamp": "2001-04-03-10.11.12.123456" } } );
      cl.insert( { "a": { "$timestamp": "2001-05-03-10.12.12.123456" } } );
      cl.insert( { "a": { "$timestamp": "2002-04-03-10.12.12.123456" } } );
      cl.insert( { "a": { "$timestamp": "2001-04-04-10.12.12.123456" } } );
      cl.insert( { "a": { "$timestamp": "2001-04-03-09.12.12.123456" } } );
      cl.insert( { "a": { "$timestamp": "2001-04-03-10.13.12.123456" } } );
      cl.insert( { "a": { "$timestamp": "2001-04-03-10.11.13.123456" } } );
      cl.insert( { "a": { "$timestamp": "2001-04-04-00.00.00.000000" } } );
      cl.insert( { a: { "$date": "2001-04-03" } } );
      cl.insert( { a: { "$date": "2001-05-03" } } );
      cl.insert( { a: { "$date": "2002-05-03" } } );
      cl.insert( { a: { "$date": "2001-04-04" } } );
   }
   catch( e )
   {
      throw buildException( "loadData", e );
   }
}

function sortData ( cl )
{
   var resultSet = [];
   var cursor = cl.find( {}, { a: 1 } ).sort( { a: 1 } );
   while( cursor.next() )
   {
      var obj = cursor.current().toObj();
      if( obj["a"]["$date"] != undefined )
      {
         println( typeof ( obj["a"]["$date"] ) );
         resultSet.push( obj["a"]["$date"] );
      }
      else if( obj["a"]["$timestamp"] != undefined )
      {
         println( "#####" + typeof ( obj["a"]["$timestamp"] ) );
         resultSet.push( obj["a"]["$timestamp"] );
      }
   }
   return resultSet;
}

function checkResult ( realResult, expectResult )
{
   for( var i = 0; i < realResult.length; ++i )
   {
      if( realResult[i] !== expectResult[i] )
      {
         var real = JSON.stringify( realResult );
         var expect = JSON.stringify( expectResult );
         throw buildException( "checkResult", -1, "find({}, {a:1}).sort({a:1})",
            expect, real );
      }
   }
}

function main ()
{
   try
   {
      db = new Sdb( COORDHOSTNAME, COORDSVCNAME );
      var clName = COMMCLNAME + "_timetypesort";
      commDropCL( db, COMMCSNAME, clName, true, true,
         "failed to drop cl in the begnning" );

      var cl = commCreateCL( db, COMMCSNAME, COMMCLNAME, 0, true, true, false,
         "failed to create collection in the beginning" );

      cl.createIndex( "indexA", { a: 1 } );
      var expectResult = ["2001-04-03",
         "2001-04-03-09.12.12.123456",
         "2001-04-03-10.11.12.123456",
         "2001-04-03-10.11.13.123456",
         "2001-04-03-10.13.12.123456",
         "2001-04-04-00.00.00.000000",
         "2001-04-04",
         "2001-04-04-10.12.12.123456",
         "2001-05-03",
         "2001-05-03-10.12.12.123456",
         "2002-04-03-10.12.12.123456",
         "2002-05-03"];
      loadData( cl );
      var result = sortData( cl );
      checkResult( result, expectResult );
   }
   catch( e )
   {
      throw e;
   }
   finally
   {
      db.close();
   }
}

main();
