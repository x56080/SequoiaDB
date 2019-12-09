/******************************************************************************
*@Description : abnormal parameter verification for deleteLob().
*@Modify list :
*               2019-05-29  wuyan  Init
******************************************************************************/

function main ( db )
{
   var clName = "testLob4465";
   commDropCL( db, COMMCSNAME, clName, true, true, "clear collection in the beginning" );

   var cl = commCreateCL( db, COMMCSNAME, COMMCLNAME, 0, true, true, false, "create collection" );

   //test case:4465
   deleteLobWithOidNotExist( cl );
   //test case:4467
   deleteLobWithEmpty( cl );

   commDropCL( db, COMMCSNAME, COMMCLNAME, true, true, "clear collection in the ending" );

}

function deleteLobWithOidNotExist ( cl )
{
   var lobOid = "5ce6016a97216ce21b5c982f";
   try
   {
      cl.deleteLob( lobOid );
      throw "delete Lob with oid not exist must be fail!";
   }
   catch( e )
   {
      //error:File Not Exist -4
      if( -4 !== e )
      {
         throw buildException( "deleteLobWithOidNotExist", e );
      }
   }
}

function deleteLobWithEmpty ( cl )
{
   try
   {
      cl.deleteLob();
      throw "delete Lob with empty must be fail!";
   }
   catch( e )
   {
      //error:Oid must be config -259
      if( -259 !== e )
      {
         throw buildException( "deleteLobWithEmpty", e );
      }
   }
}

