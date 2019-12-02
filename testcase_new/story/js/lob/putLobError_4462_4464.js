/******************************************************************************
*@Description : abnormal parameter verification for putLob().
*@Modify list :
*               2019-05-29  wuyan  Init
******************************************************************************/

function main( db )
{
   var clName = "testLob4462"; 
   commDropCL( db, COMMCSNAME, clName, true, true, "clear collection in the beginning" ); 
   
   var cl = commCreateCL( db, COMMCSNAME, COMMCLNAME, 0, true, true, false, "create collection" ); 
   
   //test case:4462
   putLobWithFileNotExist( cl ); 
   //test case:4464
   putLobWithEmpty( cl ); 
   
   commDropCL( db, COMMCSNAME, COMMCLNAME, true, true, "clear collection in the ending" ); 
   
}

function putLobWithFileNotExist( cl )
{
   var testLobFile = "/test4462.txt"; 
   try
   {
      cl.putLob( testLobFile ); 
      throw "put Lob with file not exist must be fail!"; 
   }
   catch( e )
   {
      //error:Failed to open file -4
      if( -4 !== e )
      {
         throw buildException( "putLobWithFileNotExist", e ); 
      }
   }
}

function putLobWithEmpty( cl )
{
   try
   {
      cl.putLob(); 
      throw "put Lob with empty  must be fail!"; 
   }
   catch( e )
   {
      //error:Filepath must be config -259
      if( -259 !== e )
      {
         throw buildException( "putLobWithEmpty", e ); 
      }
   }
}

