// create cs.
// CSname's large is 127.
TESTCSNAMGE = CHANGEDPREFIX + "foo"; 

TESTCLNAMGE = CHANGEDPREFIX + "bar"; 


var _CSPREFIX = TESTCSNAMGE; 
var tmpLen = _CSPREFIX.length; 
for( var i = 0; i <( 128-tmpLen ); ++i )
{
   TESTCSNAMGE = TESTCSNAMGE + "a"; 
}

var res = false; 
try
{
   db.createCS( TESTCSNAMGE ); 
}
catch( e )
{
   
   res = true; 
   
}
if( !res )
{
   throw -1; 
}
try
{
   db.dropCS( TESTCSNAMGE ); 
}
catch( e )
{
   
}

