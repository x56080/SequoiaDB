// update record.
// unnormal rule.
try
{
   commDropCL( db, COMMCSNAME, COMMCLNAME, true, true, "drop cl in the beginning" );
}
catch( e )
{
   println( "unexpected err happened when clear cs:" + e );
   throw e;
}

// prepare env
// commCreateCL( db, csName, clName, replSize, compressed, autoCreateCS, ignoreExisted, message )
var varCL = commCreateCL( db, COMMCSNAME, COMMCLNAME, {}, true, true,
   "Ensure env before usecase" );
try
{
   varCL.insert( { a: [1, 2] } );
}
catch( e )
{
   println( "failed to insert record, rc= " + e );
   throw e;
}

try
{
   varCL.update( { $set: { "a.c": 0 } } );
}

catch( e )
{
   println( "Update {$set:{'a.c':0}} failed: " + e );
   throw e;
}

println( "###################" );
checkResult( varCL, {}, [{ a: [1, 2] }] );
try
{
   varCL.insert( { a: { a: 3 } } );
}
catch( e )
{
   println( "failed to insert record, rc= " + e );
   throw e;
}

println( "###################" );
try
{
   varCL.update( { $set: { "a.a": "b" } } );
}

catch( e )
{
   println( "Update {$set:{'a.a':'b'}} failed: " + e );
   throw e;
}

checkResult( varCL, { "a.a": "b" }, [{ "a": { "a": "b" } }] );

try
{
   varCL.insert( { a: 3 } );
}
catch( e )
{
   println( "failed to insert record, rc= " + e );
   throw e;
}

try
{
   varCL.update( { $set: { "a": 0 } } );
}
catch( e )
{
   println( "Update {$set:{'a';:0}} failed: " + e );
   throw e;
}

checkResult( varCL, { a: 0 }, [{ a: 0 }, { a: 0 }, { a: 0 }] )
// clear env
// commDropCL( db, csName, clName, ignoreCSNotExist, ignoreCLNotExist, message )
commDropCL( db, COMMCSNAME, COMMCLNAME, true, true, "Clear env after usecase" );

