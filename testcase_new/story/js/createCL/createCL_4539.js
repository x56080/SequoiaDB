/************************************
*@Description: createCL，name长度无效_st.verify.CL.010
*@author:      wangkexin
*@createDate:  2019.6.6
*@testlinkCase: seqDB-4539
**************************************/
main();
function main ()
{
   var clName = "123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890test4539"
   try
   {
      db.getCS( COMMCSNAME ).createCL();
      throw "expect failure but succeed.";
   }
   catch( e )
   {
      if( -259 !== e )
      {
         throw buildException( "main()", e, "create cl1 failed", -259, e );
      }
   }

   try
   {
      db.getCS( COMMCSNAME ).createCL( "" );
      throw "expect failure but succeed.";
   }
   catch( e )
   {
      if( -6 !== e )
      {
         throw buildException( "main()", e, "create cl2 failed", -6, e );
      }
   }

   try
   {
      db.getCS( COMMCSNAME ).createCL( clName );
      throw "expect failure but succeed.";
   }
   catch( e )
   {
      if( -6 !== e )
      {
         throw buildException( "main()", e, "create cl3 failed", -6, e );
      }
   }
}