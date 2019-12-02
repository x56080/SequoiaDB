// create cs.
// normal case.

TESTCSNAMGE = CHANGEDPREFIX + "foo"; 

TESTCLNAMGE = CHANGEDPREFIX + "bar"; 
function main( db )
{
   
   
   try
   {
      db.dropCS( TESTCSNAMGE ); 
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
      db.createCS( TESTCSNAMGE ); 
   }
   catch( e )
   {
      println( "failed to create cs, rc= " + e ); 
      throw e; 
   }
   
   try
   {
      var rc = db.getCS( TESTCSNAMGE ); 
   }
   catch( e )
   {
      println( "failed to get cs, rc= " + e ); 
      throw e; 
   }
   
   try
   {
      db.dropCS( TESTCSNAMGE ); 
   }
   catch( e )
   {
      println( "unexpected err happened when clear cs:" + e ); 
      throw e; 
   }
   
}

for( var i = 0; i != 200; ++i )
{
   main( db ); 
}
