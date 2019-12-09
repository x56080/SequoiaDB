/************************************
*@Description: 设置timeout最小值校验
*@author:      luweikang
*@createdate:  2019.2.24
*@testlinkCase:seqDB-16102
**************************************/
main()
function main ()
{
   var sdb = new Sdb( COORDHOSTNAME, COORDSVCNAME );
   if( true == commIsStandalone( sdb ) )
   {
      println( "run mode is standalone" );
      return;
   }

   var oldSessionAttr = sdb.getSessionAttr().toObj();
   if( oldSessionAttr.Timeout != -1 )
   {
      throw buildException( "checkTimeoutValue()", "", "getSeesionAttr()", -1, oldSessionAttr.Timeout );
   }

   sdb.setSessionAttr( { PreferedInstance: "M", Timeout: 1 } );
   var sessionAttr = sdb.getSessionAttr().toObj();
   if( sessionAttr.Timeout != 1000 )
   {
      throw buildException( "checkTimeoutValue()", "", "getSeesionAttr()", 1000, sessionAttr.Timeout );
   }
}