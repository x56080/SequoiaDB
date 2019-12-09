// create cs.
// CSname's large is 127.
TESTCSNAMGE = CHANGEDPREFIX + "foo";

TESTCLNAMGE = CHANGEDPREFIX + "bar";

var aa = Array( "; ", "\'", "{", "}", "[", "]", ", ", "+", "=", "-", "_", "~", "`", "!", "@", "#", "$", "%", "^", "&", "( ", " )" );


for( var i = 0; i < aa.length; ++i )
{
   try
   {
      var CSname = TESTCSNAMGE + aa[i];
      db.dropCS( CSname );
   }
   catch( e )
   {
   }
}

var j = 0;
var aa = Array( "; ", "\'", "{", "}", "[", "]", ", ", "+", "=", "-", "_", "~", "`", "!", "@", "#", "$", "%", "^", "&", "( ", " )" );

for( var i = 0; i < aa.length; ++i )
{
   try
   {
      var CSname = TESTCSNAMGE + aa[i];
      db.createCS( CSname );
   }
   catch( e )
   {
      println( aa[i] );
      ++j;
   }
}

if( 0 != j )
{
   //       throw -1; 
}

j = 0;

for( var i = 0; i < aa.length; ++i )
{
   try
   {
      var CSname = TESTCSNAMGE + aa[i];
      db.dropCS( CSname );
   }
   catch( e )
   {
      ++j;
   }
}


if( 0 != j )
{
   throw -1;
}
