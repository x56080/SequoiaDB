// drop cs.
// normal case.

function dropCSAndCheck ( db, csName )
{
   var name = csName;
   try
   {
      db.dropCS( name );
   }
   catch( e )
   {
      println( "failed to drop cs, rc= " + e );
      throw e;
   }

   try
   {
      var cur = db.listCollectionSpaces();
      while( cur.next() )
      {
         if( csName == cur.current().toObj()["Name"] )
         {
            throw "have droped the cs:" + csName + ", but it still exist";
         }
      }
   }
   catch( e )
   {
      throw e;
   }

   /// create a collection in dropped cs.
   /// but have no idea use which interface.
}
csName = CHANGEDPREFIX + "foo";

CSPREFIX_CL = CHANGEDPREFIX + "bar";
//var csName = "testcs"; 


try
{
   try
   {
      db.dropCS( csName );
   }
   catch( e )
   {

   }

   try
   {
      db.createCS( csName );
   }
   catch( e )
   {
      println( "failed to create cs, rc= " + e )
      throw e;
   }
   dropCSAndCheck( db, csName );
}
catch( e )
{
   println( "test case returns error" );
   throw e;
}

