
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
   if( 10 != varCL.find().limit( 10 ).size() )
      throw -1;
   if( 100 != varCL.find().limit( 10 ).count() )
      throw -1;
   if( 10 != varCL.find().skip( 90 ).size() )
      throw -1;
   if( 100 != varCL.find().skip( 90 ).count() )
      throw -1;
} catch( e )
{
   if( e == -1 )
   {
      println( "find() with limit or skip count/size have error \n" );
      throw -1;
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
