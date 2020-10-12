// update record.
// normal case. $set

main( test );
function test ()
{
   commDropCL( db, COMMCSNAME, COMMCLNAME, true, true, "drop cl in the beginning" );

   var varCS = commCreateCS( db, COMMCSNAME, true, "create CS in the beginning" );
   var varCL = varCS.createCL( COMMCLNAME, { ReplSize: 0, Compressed: true } );

   varCL.insert( { a: 1 } );

   varCL.update( { $set: { a: 2 } }, { a: 1 } );

   var rc = varCL.find( { a: 2 } );

   var size = 0;
   while( true )
   {
      var i = rc.next();
      if( !i )
         break;
      else
         size++;
   }

   assert.equal( size, 1 );

   commDropCL( db, COMMCSNAME, COMMCLNAME, true, true );
}