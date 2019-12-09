/******************************************************************************
@Description : 1. sort: index sort
@Modify list :
               2015-01-15 pusheng Ding  Init
******************************************************************************/
main();
function main ()
{
   var csName = COMMCSNAME;
   var clName = "cl13737";
   var indexName1 = "index13737_1";
   var indexName2 = "index13737_2";
   var rownums = 10000;

   commDropCL( db, csName, clName, true, true, "drop cl in the beginning" );

   //create index
   var varCS = commCreateCS( db, csName, true, "create CS in the beginning" );
   var varCL = varCS.createCL( clName );
   varCL.createIndex( indexName1, { a: 1 } );
   varCL.createIndex( indexName2, { b: -1 } );
   println( "create indexes finished!" );

   //insert data
   var records = [];
   for( var i = 0; i < rownums; i++ )
   {
      records.push( { a: i, b: i, c: i + "abcdefghijklmnopqrstuvwxyz" } );
   }
   varCL.insert( records );
   println( "insert data finished!" );

   //query1
   //select a,b,c from foo.bar order by a
   var sel = varCL.find().sort( { a: 1 } ).hint( { "": indexName1 } );
   checkRec( sel, records );
   println( "'select a,b,c from foo.bar order by a' with index1 finished!" );

   //query2
   //select b,a from foo.bar order by b desc
   var sel = varCL.find().sort( { b: -1 } ).hint( { "": indexName2 } );
   //expected result {a:9999,...} {a:9998,...} ... {a:0,...}
   records.reverse();
   checkRec( sel, records );
   println( "'select b,a from foo.bar order by b desc' with index2 finished!" );
   commDropCL( db, csName, clName, false, false, "drop cl in the end" );
}
