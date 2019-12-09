//create exist collection case3
TESTCSNAME_1 = CHANGEDPREFIX + "foo_1";
TESTCSNAME_2 = CHANGEDPREFIX + "foo_2";
TESTCLNAME = CHANGEDPREFIX + "bar";

try
{
   db.dropCS( TESTCSNAME_1 );
}
catch( e )
{
   if( e != -34 )
   {
      println( "unexpected err happened when clear cs:" + e );
      throw e;
   }
}

try
{
   db.dropCS( TESTCSNAME_2 );
}
catch( e )
{
   if( e != -34 )
   {
      println( "unexpected err happened when clear cs:" + e );
      throw e;
   }
}

try
{
   var varCS = commCreateCS( db, TESTCSNAME_1, false, "failed to create CS" );
}
catch( e )
{
   println( "failed to create cs,rc=" + e );
   throw e;
}

try
{
   var varCS1 = commCreateCS( db, TESTCSNAME_2, false, "failed to create CS" );
}
catch( e )
{
   println( "failed to create cs,rc=" + e );
   throw e;
}

try
{
   varCS.createCL( TESTCLNAME, { Compressed: true } );
}
catch( e )
{
   println( "failed to create cl, rc= " + e );
   throw e;
}

try
{
   varCS1.createCL( TESTCLNAME, { Compressed: true } );
}
catch( e )
{
   println( "failed to create cl, rc= " + e );
   throw e;
}

try
{
   db.dropCS( TESTCSNAME_1 );
}
catch( e )
{
   println( "unexpected err happened when clear cs:" + e );
   throw e;
}

try
{
   db.dropCS( TESTCSNAME_2 );
}
catch( e )
{
   println( "unexpected err happened when clear cs:" + e );
   throw e;
}
