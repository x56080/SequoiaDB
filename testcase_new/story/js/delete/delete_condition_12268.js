// delete record.
// normal case.
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
   var optionObj = { ReplSize: 0, Compressed: true };
   var varCL = commCreateCL( db, COMMCSNAME, COMMCLNAME, optionObj, true,
      false, "create collecton 1 failed" );
}
catch( e )
{
   throw e;
}

try
{
   var docs = [];
   docs.push( { a: 1 } );
   docs.push( { b: [1, 2], salary: 10, name: "Tom" } );
   varCL.insert( docs );
}
catch( e )
{
   println( "failed to insert record, rc= " + e );
   throw e;
}

var rc;
try
{
   rc = varCL.find();
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
println( "size=" + size );
if( 2 != size )
{
   println( " get the nnumber of records is wrong" );
   throw -1;
}

try
{
   varCL.remove( { name: "Mike" } );
}
catch( e )
{
   println( "failed to remove record, rc1= " + e );
   throw e;
}

try
{
   rc1 = varCL.find( { name: "Tom" } );
}
catch( e )
{
   println( "failed to read record, rc1= " + e );
   throw e;
}

size = 0;
while( true )
{
   var i = rc1.next();
   if( !i )
      break;
   else
      size++;
}

if( 1 != size )
{
   println( " get the number of records is wrong, rc1" );
   throw -1;
}
checkResult( varCL, {}, [{ a: 1 }, { b: [1, 2], salary: 10, name: "Tom" }] );
try
{
   varCL.remove( { a: 1 } );
}
catch( e )
{
   println( "failed to remove record, rc= " + e );
   throw e;
}

try
{
   rc = varCL.find( { a: 1 } );
}
catch( e )
{
   println( "failed to read record, rc= " + e );
   throw e;
}

size = 0;
while( true )
{
   var i = rc.next();
   if( !i )
      break;
   else
      size++;
}

if( 0 != size )
{
   println( " get more than zero record" );
   throw -1;
}

checkResult( varCL, {}, [{ b: [1, 2], salary: 10, name: "Tom" }] );
try
{
   commDropCL( db, COMMCSNAME, COMMCLNAME, false, false,
      "drop colleciton in the end" );
}
catch( e )
{
   println( "failed to drop cs, rc= " + e );
   throw e;
}

