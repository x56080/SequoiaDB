// update record.
// normal case. $set
main( test );
function test ()
{
   commDropCL( db, COMMCSNAME, COMMCLNAME, true, true, "drop cl in the beginning" );

   var varCS = commCreateCS( db, COMMCSNAME, true, "create CS in the beginning" );
   var varCL = varCS.createCL( COMMCLNAME, { ReplSize: 0, Compressed: true } );

   var insertCount = 1000;
   var docs = []
   for( i = 0; i < insertCount; i++ ) 
   {
      docs.push( { a: i, b: "fdafdsaf$#@$@%$#%#@!$#@!$", c: null, d: { id: 1.0, name: "qiu" }, e: { "$binary": "aGVsbG8gd29ybGQ=", "$type": "1" } } );
   }
   varCL.insert( docs );

   varCL.update( { "$unset": { noexist: "" } } )

   var rc = varCL.find();

   var recordCount = 0;
   while( true )
   {
      //var record = eval( "("+ rc.current() +")" );
      var record = rc.current().toObj();
      if( !compareObj( docs[recordCount], record, false ) )
      {
         throw new Error( -1 );
      }

      recordCount++;
      if( !rc.next() )
         break;
   }

   assert.equal( insertCount, recordCount );

   commDropCL( db, COMMCSNAME, COMMCLNAME, true, true );
}
