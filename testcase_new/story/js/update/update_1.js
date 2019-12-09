// update record.
// normal case. $set
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
   throw e;
}

try
{
   varCL.insert( { a: 1 } );
}
catch( e )
{
   println( "failed to insert record, rc= " + e );
   throw e;
}

try
{
   varCL.update( { $set: { a: 2 } }, { a: 1 } );
}
catch( e )
{
   println( "failed to update record, rc= " + e );
   throw e;
}

var rc;
try
{
   rc = varCL.find( { a: 2 } );
}
catch( e )
{
   println( "failed to read record, rc= " + e );
   throw e;
}

var size = 0;
while( true )
{
   var i = rc.next();
   if( !i )
      break;
   else
      size++;
}

if( 1 != size )
{
   println( " get more than one record" );
   throw -1;
}

try
{
   commDropCL( db, COMMCSNAME, COMMCLNAME, true, true,
      "drop colleciton in the end" );
}
catch( e )
{
   println( "failed to drop cs, rc= " + e );
   throw e;
}
