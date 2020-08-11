// delete record.
// normal case.

try
{
   main();
}
catch( e )
{
   if( e.constructor === Error )
   {
      println( e.stack );
   }
   throw e;
}

function main ()
{
   var clName = COMMCLNAME + "_12270";
   commDropCL( db, COMMCSNAME, clName );

   var varCL = commCreateCL( db, COMMCSNAME, clName );

   var docs = [];
   docs.push( { a: 1 } );
   docs.push( { b: [1, 2], salary: 10, name: "Tom" } );

   varCL.insert( docs );
   varCL.remove();
   var cursor = varCL.find();
   commCompareResults( cursor, [] );
   commDropCL( db, COMMCSNAME, clName, false, false, "drop colleciton in the end" );
}
