/************************************
*@Description: createDomain，name长度无效_st.verify.domain.004
*@author:      wangkexin
*@createDate:  2019.6.6
*@testlinkCase: seqDB-4487
**************************************/
main();
function main ()
{
   if( true == commIsStandalone( db ) )
   {
      println( "run mode is standalone" );
      return;
   }

   var groupsArray = commGetGroups( db, false, "", true, true, true );
   var groupName = [groupsArray[0][0].GroupName];
   var domainName = "123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890test4487";

   try
   {
      db.createDomain();
      throw "expect failure but succeed.";
   }
   catch( e )
   {
      if( -259 !== e )
      {
         throw buildException( "main()", e, "create domain1 failed", -259, e );
      }
   }

   try
   {
      db.createDomain( "", groupName );
      throw "expect failure but succeed.";
   }
   catch( e )
   {
      if( -6 !== e )
      {
         throw buildException( "main()", e, "create domain2 failed", -6, e );
      }
   }

   try
   {
      db.createDomain( domainName );
      throw "expect failure but succeed.";
   }
   catch( e )
   {
      if( -6 !== e )
      {
         throw buildException( "main()", e, "create domain3 failed", -6, e );
      }
   }
}