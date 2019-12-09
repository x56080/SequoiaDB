/************************************
*@Description: createDomain，options:AutoSplit取值非法_st.verify.domain.009
*@author:      wangkexin
*@createDate:  2019.6.6
*@testlinkCase: seqDB-4492
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
   var domainName = "domain4492";

   checkInvalidAutoSplit( domainName, groupName, "test_4492" );
   checkInvalidAutoSplit( domainName, groupName, "" );
}

function checkInvalidAutoSplit ( domainName, groupName, autosplit )
{
   try
   {
      db.createDomain( domainName, groupName, { AutoSplit: autosplit } );
      throw "expect failure but succeed.";
   }
   catch( e )
   {
      if( -6 !== e )
      {
         throw buildException( "main()", e, "create domain failed, autoSplit is : " + autosplit, -6, e );
      }
   }
}