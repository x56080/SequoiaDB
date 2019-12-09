// upsert record.
// normal case.

// clear
commDropCL( db, COMMCSNAME, COMMCLNAME, true, true, "drop cl in the beginning" );

// create cs, cl
var varCL = commCreateCL( db, COMMCSNAME, COMMCLNAME, -1, true, true, false,
   "create cs and cl in begin" );

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
   varCL.upsert( { $set: { a: 2 } }, { a: 1 } );
}
catch( e )
{
   println( "failed to update( {$set:{a:2}}, {a:1} ) record, rc= " + e );
   throw e;
}

checkResult( varCL, {}, [{ a: 2 }] );

/*var rc; 
try
{
rc = varCL.find( {a:2} ); 
}
catch( e )
{
println( "failed to read record, rc= " + e ); 
throw e; 
}

var size = rc.size(); 
if( 1 != size )
{
println( " get more than one record after upsert( $set:{a:2}}, {a:1} )" ); 
throw -1; 
}
*/
try
{
   varCL.upsert( { $set: { a: 4 } }, { a: 3 } );
}
catch( e )
{
   println( "failed to insert record, rc= " + e );
   throw e;
}

/*
try
{
rc = varCL.find(); 
}
catch( e )
{
println( "failed to read record, rc= " + e ); 
throw e; 
}

size = 0; 

var bAddNew = false; 
while( rc.next() )
{
var recordObj = rc.current().toObj(); 
var recordStr = rc.current().toJson(); 
if( recordObj["a"] == 4 )
{
bAddNew = true; 
}
size++; 
}

if( size != 2 || !bAddNew )
{
println( "The size is not equal 2 or The new record no be added. size=" + size ); 
println( varCL.find() ); 
throw -1; 
}
*/
checkResult( varCL, {}, [{ a: 2 }, { a: 4 }] );
//zhaoyu add
try
{
   varCL.upsert( { $set: { a: 5 } }, { $or: [{ b: 1 }] } );
}
catch( e )
{
   println( "failed to insert record, rc= " + e );
   throw e;
}
checkResult( varCL, {}, [{ a: 2 }, { a: 4 }, { a: 5, b: 1 }] );
/*
try
{
rc = varCL.find(); 
}
catch( e )
{
println( "failed to read record, rc= " + e ); 
throw e; 
}

size = 0; 

var bAddNew = false; 
while( rc.next() )
{
var recordObj = rc.current().toObj(); 
var recordStr = rc.current().toJson(); 
if( recordObj["a"] == 5 && recordObj["b"] === 1 )
{
bAddNew = true; 
}
size++; 
}

if( size != 3 || !bAddNew )
{
println( "The size is not equal 3 or The new record no be added. size=" + size ); 
println( varCL.find() ); 
throw -1; 
}
*/


// clear end
commDropCL( db, COMMCSNAME, COMMCLNAME, false, false, "drop cl in the end" ); 
