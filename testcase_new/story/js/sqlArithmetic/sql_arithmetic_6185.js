/************************************************************************
*@Description:    seqDB-6185:参数错误_st.sql.arithExpre.008
*@Author:  2016/7/12  huangxiaoni
************************************************************************/
main();

function main ()
{
   try
   {
      var csName = COMMCSNAME;
      var clName = COMMCLNAME + "_6185";

      dropCL( csName, clName, true, "Failed to drop cl in the begin." );
      createCL( csName, clName, true, true, "Failed to create cl." );

      insertRecs( csName, clName );
      selectRecs( csName, clName );

      dropCL( csName, clName, false, "Failed to drop cl in the end." );
   }
   catch( e )
   {
      throw e;
   }
}

function insertRecs ( csName, clName )
{
   println( "\n---Begin to insert records." );

   db.execUpdate( "insert into " + csName + "." + clName + "(a) values(6)" );
}

function selectRecs ( csName, clName )
{
   println( "\n---Begin to select records." );

   try
   {
      db.exec( "select a+'test' from " + csName + "." + clName );
   }
   catch( e )
   {
      //check result  //e:-6
      var expectE = -6;
      if( e !== expectE )
      {
         throw buildException( "checkResult", e, "invalid parameter",
            "[e:" + expectE + "]", "[e:" + e + "]" );
      }
   }

}