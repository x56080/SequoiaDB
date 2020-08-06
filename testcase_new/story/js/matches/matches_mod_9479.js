/******************************************************************************
@Description : jira1913 seqDB-9479:取模运算
@author :Shitong Wen
               
******************************************************************************/

function main ()
{
   try
   {
      println( "---drop collection space and collection in the beginning" );
      //create cs cl
      var csName = COMMCSNAME;
      var clName = COMMCLNAME;
      // Drop CS
      commDropCS( db, csName, true,
         "clear collection space in the beginning" );

      println( "---create collection space and collection in the beginning" );
      //create cs and cl      
      commCreateCL( db, csName, clName, {}, true, false, "create cs and cl" );

      println( "---begin to insert long/int/ records" );
      var cl = db.getCS( csName ).getCL( clName );
      cl.remove();
      var recs = [];

      recs.push( { a: -1E+40 } );
      recs.push( { a: { $numberLong: "-9223372036854775808" } } );
      recs.push( { a: -2147483648 } );

      cl.insert( recs );
      checkResult( cl, recs );
      //clean environment
      commDropCS( db, csName, true,
         "clear collection space in the end" );
   }
   catch( e )
   {
      throw e;
   }

}
//check the result;
function checkResult ( cl, recs )
{
   println( "---begin to check result" );
   var findParame = { a: { $mod: [{ $numberLong: "-2147483648" }, 0] } };
   var cursor = cl.find( findParame ).sort( { a: 1 } );
   var i = 0;

   while( cursor.next() )
   {
      if( cursor.current().toObj()["a"]["$numberLong"] == undefined )
      {

         if( cursor.current().toObj()["a"] != recs[i]["a"] )
            throw buildException( "check return code", "",
               'compare  value',
               recs[i]["a"], cursor.current().toObj()["a"] );

      } else
      {

         if( cursor.current().toObj()["a"]["$numberLong"] != recs[i]["a"] )
            throw buildException( "check return code", "",
               'compare numberLong value',
               recs[i]["a"], cursor.current().toObj()["a"]["$numberLong"] );

      }
      i++;
   }

   if( i != 3 )
      throw "expected result set number is not correct! i =" + i;

}

main();
db.close();

