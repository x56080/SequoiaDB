// update record.
// update_condition rule.

main( test );
function test ()
{
   commDropCL( db, COMMCSNAME, COMMCLNAME, true, true, "drop cl in the beginning" );

   var varCS = commCreateCS( db, COMMCSNAME, true, "create CS in the beginning" );
   var varCL = varCS.createCL( COMMCLNAME, { ReplSize: 0, Compressed: true } );

   varCL.insert( { a: [1, 2], salary: 100 } );
   varCL.insert( { a: [1, 2], salary: 10, name: "Tom" } );
   varCL.insert( { a: [1, 2], salary: 10, name: "Mike", age: 20 } );

   varCL.update( { $set: { age: 25 } }, { age: { $exists: 1 } } );

   var rc = varCL.find( { age: 25 } );

   var size = 0;
   while( true )
   {
      var i = rc.next();
      if( !i )
         break;
      else
         size++;
      if( !compareObj( i.toObj(), { a: [1, 2], salary: 10, name: "Mike", age: 25 }, false ) )
      {
         throw new Error( "expect: {a:[1,2],salary:10,name:'Mike',age:25}" +
            "real:" + JSON.stringify( i.toObj() ) );
      }
   }
   assert.equal( size, 1 );

   varCL.update( { $set: { age: 30 } }, null );

   var rc1;
   checkResult( varCL, { age: 30 }, [{ a: [1, 2], salary: 100, age: 30 },
   { a: [1, 2], salary: 10, name: "Tom", age: 30 },
   { a: [1, 2], salary: 10, name: "Mike", age: 30 }] );

   commDropCL( db, COMMCSNAME, COMMCLNAME, true, true );
}