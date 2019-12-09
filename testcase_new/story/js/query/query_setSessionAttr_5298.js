/************************************
*@Description: setSessionAttr，字段值超过边界值_ST.basicOperate.setSessionAttr.002
*@author:      wangkexin
*@createDate:  2019.6.5
*@testlinkCase: seqDB-5298
**************************************/
main();
function main ()
{
   if( true == commIsStandalone( db ) )
   {
      println( "run mode is standalone" );
      return;
   }

   //PreferedInstance字段值分别取0、256、y
   checkInvalidArg( 0 );
   checkInvalidArg( 256 );
   checkInvalidArg( "y" );
}

function checkInvalidArg ( argument )
{
   try
   {
      db.setSessionAttr( { "PreferedInstance": argument } );
      throw "expect failure but succeed.";
   }
   catch( e )
   {
      if( e !== -6 )
      {
         throw buildException( "checkInvalidArg()", e, "argument is :" + argument, -6, e );
      }
   }
}