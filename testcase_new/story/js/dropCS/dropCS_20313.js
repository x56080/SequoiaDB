/************************************
 * *@Description: DropCS() can only specify one parameter 
 * *@author:      lena
 * *@createdate:  2019.1.17
 * *@testlinkCase:seqDB-20313
 * **************************************/

try
{
   main();
}
catch( e )
{
   if( e.constructor === Error )
   {
      println( e.stack );
   }
   throw new Error( e );
}

function main ()
{

   var csArray = new Array();

   for( var i = 0; i < 2; i++ )
   {
      csArray[i] = COMMCSNAME + i;
      db.createCS( csArray[i] );
   }
   try
   {
      db.dropCS( csArray[0], csArray[1] );
      throw new Error( "Drop multiple cs incorectlly " );
   }
   catch( e )
   {
      if( e !== -6 )
      {
         throw new Error( e );
      }
   }
   finally
   {
      for( var i = 0; i < csArray.length; i++ )
      {
         db.dropCS( csArray[i] );
      }
   }
}
