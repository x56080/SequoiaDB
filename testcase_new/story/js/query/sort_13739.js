/******************************************************************************
@Description : 1. sort: a[1,2,3] sort
@Modify list :
               2015-01-15 pusheng Ding  Init
******************************************************************************/
main();
function main ()
{
   var indexName = "index13739";
   var rownums = 10000;
   var csName = COMMCSNAME;
   var clName = "cl13739";

   commDropCL( db, csName, clName, true, true, "drop cl in the beginning" );
   var options = { ReplSize: 0 };
   var varCL = commCreateCLByOption( db, csName, clName, options, true, false, "create cl." );

   //insert data
   var records = [];
   for( var i = 0; i < rownums; i++ )
   {
      records.push( { a: { a1: i, a2: rownums - i }, b: [i, i + 1, i + 2] } );
   }
   varCL.insert( records );

   //query1
   //select a,b from foo.bar order by b
   var sel = varCL.find( null, { a: null, b: 'b' } ).sort( { b: 1 } );
   checkRec( sel, records );
   println( "'select a,b from foo.bar order by b' finished!" );

   //create index
   varCL.createIndex( indexName, { b: 1 } );
   println( "create indexes finished!" );

   //query2
   //select a,b from foo.bar order by b
   var sel = varCL.find( null, { a: null, b: 'b' } ).sort( { b: 1 } ).hint( { "": indexName } );
   checkRec( sel, records );
   println( "'select a,b from foo.bar order by b' with index finished!" );
   commDropCL( db, csName, clName, false, false, "drop cl in the end" );
}
