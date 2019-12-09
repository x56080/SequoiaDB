/******************************************************************************
 * @Description : test getSlave operation
 *                seqDB-13792:getSlave参数校验 
 * @auhor       : Liang XueWang
 ******************************************************************************/
var rgName = "testGetSlaveRg13792";

main();

function main ()
{
   if( commIsStandalone( db ) )
   {
      println( "Run mode is standalone" );
      return;
   }

   testIllegalPos();
}

function testIllegalPos ()
{
   var rg = db.createRG( rgName );

   var errorPos = ["a", 0, 8, 1.2, -10];
   try
   {
      for( var i = 0; i < errorPos.length; i++ )
      {
         try
         {
            rg.getSlave( errorPos[i] );
            throw 0;
         }
         catch( e )
         {
            if( e !== -6 )
            {
               throw buildException( "testIllegalPos", e, "test getSlave with " + errorPos[i], -6, e );
            }
         }
      }

      try
      {
         rg.getSlave( 1, 2, 0, 5, 8 );
         throw 0;
      }
      catch( e )
      {
         if( e !== -6 )
         {
            throw buildException( "testIllegalPos", e, "test getSlave with (1, 2, 0, 5, 8)", -6, e );
         }
      }

      try
      {
         rg.getSlave( 1 );
         throw 0;
      }
      catch( e )
      {
         if( e !== -158 )
         {
            throw buildException( "testIllegalPos", e, "test getSlave with empty group", -158, e );
         }
      }
   }
   finally
   {
      db.removeRG( rgName );
   }
}
