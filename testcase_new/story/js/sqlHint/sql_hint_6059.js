/************************************************************************
*@Description:   seqDB-6059:select指定某个集合使用索引扫描，集合不存在_st.sql.hint.011
                 seqDB-6060:hint格式错误_st.sql.hint.012
*@Author:  2016/7/13  huangxiaoni
************************************************************************/
main();

function main ()
{
   try
   {
      var csName = COMMCSNAME;
      var clName = COMMCLNAME + "_6059";

      dropCL( csName, clName, true, "Failed to drop cl in the begin." );
      createCL( csName, clName, true, true, "Failed to create cl." );

      //verifyParam1( csName );
      verifyParam2( csName, clName );
      verifyParam3( csName, clName );

      dropCL( csName, clName, false, "Failed to drop cl in the end." );
   }
   catch( e )
   {
      throw e;
   }
}

function verifyParam1 ( csName )
{
   println( "\n---Begin to verify parameter[cl is not exist]." );

   try
   {
      db.exec( "select * from 123.atest /*+use_index(idx)*/" ); //"+ csName +"
   }
   catch( e )
   {
      //check result
      var expectE = -23;
      if( e !== expectE )
      {
         throw buildException( "checkResult", e, "cl is not exist",
            "[e:" + expectE + "]", "[e:" + e + "]" );
      }
   }
}

function verifyParam2 ( csName, clName )
{
   println( "\n---Begin to verify parameter[invalid param]." );

   try
   {
      db.exec( "select * from " + csName + "." + clName + " /+use_index(idx)*/" );
   }
   catch( e )
   {
      //check result
      var expectE = -195;
      if( e !== expectE )
      {
         throw buildException( "checkResult", e, "format is invalid",
            "[e:" + expectE + "]", "[e:" + e + "]" );
      }
   }
}

function verifyParam3 ( csName, clName )
{
   println( "\n---Begin to verify parameter[use_index()]." );

   try
   {
      db.exec( "select * from " + csName + "." + clName + " /*+use_index()*/" );
   }
   catch( e )
   {
      throw buildException( "checkResult", e, "use_index()",
         "exec successful", "exec failed" );
   }
}