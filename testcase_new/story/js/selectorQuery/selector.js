// query record.
// normal selector
try
{
   commDropCL( db, COMMCSNAME, COMMCLNAME, true, true, "drop cl in the beginning" );
}
catch( e )
{
   println( "unexpected err happened when clear cs:" + e );
   throw e;
}

//create CS
try
{
   var varCS = commCreateCS( db, COMMCSNAME, true, "create CS in the beginning" );
}
catch( e )
{
   println( "failed to create cs,rc=" + e );
   throw e;
}

try
{
   var varCL = varCS.createCL( COMMCLNAME, { ReplSize: 0, Compressed: true } );
} catch( e )
{
   throw e;
}

try
{
   varCL.insert( { name: "Tom", age: 15 } );
   varCL.insert( { name: "John" } );
   varCL.insert( { name: "tom", address: { street: { street1: "1024 Wall Street", street2: "University Drive" }, zipcode: 100000 } } );
   varCL.insert( { phone: ["123", "456"] } );
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
if( 4 != size )
{
   println( " get the number of records is wrong." );
   throw -1;
}

var rc1;
try
{
   rc1 = varCL.find( null, { name: null } );
}
catch( e )
{
   println( "failed to read record, rc= " + e );
   throw e;
}

var size1 = 0;
while( true )
{
   var i = rc1.next();
   if( !i )
      break;
   else
      size1++;
}
if( 4 != size1 )
{
   println( " get the number of records is wrong.." );
   throw -1;
}


var rc2;
try
{
   rc2 = varCL.find( { name: { $exists: 1 } }, { name: null } );
}
catch( e )
{
   println( "failed to read record, rc2= " + e );
   throw e;
}

var size2 = 0;
while( true )
{
   var i = rc2.next();
   if( !i )
      break;
   else
      size2++;
}
if( 3 != size2 )
{
   println( " get the number of records is wrong..." );
   throw -1;
}

var rc3;
try
{
   rc3 = varCL.find( { "phone.0": { $exists: 1 } }, { "phone.0": "78" } );
}
catch( e )
{
   println( "failed to read record, rc3= " + e );
   throw e;
}

var size3 = 0;
while( true )
{
   var i = rc3.next();
   if( !i )
      break;
   else
      size3++;
}
if( 1 != size3 )
{
   println( " get the number of records is wrong...." );
   throw -1;
}


var rc4;
try
{
   rc4 = varCL.find( { "phone.0": { $exists: 0 } }, { "phone.0": "78" } );
}
catch( e )
{
   println( "failed to read record, rc4= " + e );
   throw e;
}

var size4 = 0;
while( true )
{
   var i = rc4.next();
   if( !i )
      break;
   else
      size4++;
}
if( 3 != size4 )
{
   println( " get the number of records is wrong....." );
   throw -1;
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
