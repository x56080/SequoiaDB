
try
{
   commDropCL( db, COMMCSNAME, COMMCLNAME, true, true, "drop cl in the beginning" );
}
catch( e )
{
   println( "unexpected err happened when clear cs:" + e );
   throw e;
}

try
{
   var varCS = commCreateCS( db, COMMCSNAME, true, "create CS in the beginning" );
   var varCL = varCS.createCL( COMMCLNAME, { ReplSize: 0, Compressed: true } );
} catch( e )
{
   println( "createCS or createCL fail" );
   throw e;
}

try
{
   for( var i = 0; i < 100; i++ )
   {
      varCL.insert( { a: i } );
   }
   var curr = varCL.find().skip( 97 ).sort( { a: 1 } );
   if( 97 != curr.current().toObj()["a"] )
      throw -1;

   var curr = varCL.find().skip( 97 ).sort( { a: -1 } );
   if( 2 != curr.current().toObj()["a"] )
      throw -1;

   var curr = varCL.find().skip( 97 ).limit( 2 ).sort( { a: -1 } );
   if( 100 != curr.count() )
      throw -2;
   if( 2 != curr.size() )
      throw -2;
   var curr = varCL.find().skip( 97 ).limit( 2 ).sort( { a: -1 } );
   while( curr.next() ) { var curr_before = curr.current(); }
   if( 1 != curr_before.toObj()["a"] )
      throw -2;

   var curr = varCL.find().skip( 97 ).limit( 2 ).sort( { a: 1 } );
   if( 100 != curr.count() )
      throw -2;
   if( 2 != curr.size() )
      throw -2;
   var curr = varCL.find().skip( 97 ).limit( 2 ).sort( { a: 1 } );
   while( curr.next() ) { var curr_before = curr.current(); }
   if( 98 != curr_before.toObj()["a"] )
      throw -2;


} catch( e )
{
   if( e == -1 )
   {
      println( "find() with (skip~sort[1/-1]) have error \n" );
      throw -1;
   }
   else if( e == -2 )
   {
      println( "find() with (skip~limit~sort[1/-1]) have error \n" );
      throw -2;
   }
   else
   {
      throw e;
   }

}

try
{
   commDropCL( db, COMMCSNAME, COMMCLNAME, false, false, "drop cl in the end" );
}
catch( e )
{
   println( "failed to drop cs, rc= " + e );
   throw e;
}
