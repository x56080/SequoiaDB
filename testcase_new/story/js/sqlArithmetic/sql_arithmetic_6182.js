/************************************************************************
*@Description:    seqDB-6182:做除法/取模运算时被除数为0_st.sql.arithExpre.005
*@Author:  2016/7/12  huangxiaoni
************************************************************************/
main();

function main ()
{
   if( isTransautocommit() ) 
   {
      println( "\nThe node config[ {transautocommit: true} ]." );
      return;
   }

   try
   {
      var csName = COMMCSNAME;
      var clName = COMMCLNAME + "_6182";

      dropCL( csName, clName, true, "Failed to drop cl in the begin." );
      createCL( csName, clName, true, true, "Failed to create cl." );

      insertRecs( csName, clName );
      var rc = selectRecs( csName, clName );
      checkResult( rc );

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

   var rc0 = db.exec( "select a/0 from " + csName + "." + clName );
   var rc1 = db.exec( "select a%0 from " + csName + "." + clName );
   var rc = [rc0, rc1];
   return rc;
}

function checkResult ( rc )
{
   println( "\n---Begin to check result." );

   //compare the records for rc[0]
   var expA = null;
   var actA = rc[0].current().toObj().a;
   if( expA !== actA )
   {
      throw buildException( "checkResult", null, "[ rc0 ]",
         "[a:" + expA + "]",
         "[a:" + actA + "]" );
   }

   //compare the records for rc[1]
   var expA = null;
   var actA = rc[1].current().toObj().a;
   if( expA !== actA )
   {
      throw buildException( "checkResult", null, "[ rc1 ]",
         "[a:" + expA + "]",
         "[a:" + actA + "]" );
   }

}