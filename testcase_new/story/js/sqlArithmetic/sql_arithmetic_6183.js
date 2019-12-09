/************************************************************************
*@Description:    字段值为非数值型，做单列的算术运算_st.sql.arithExpre.006
*@Author:  2016/7/12  huangxiaoni
************************************************************************/
main();

function main ()
{
   try
   {
      var csName = COMMCSNAME;
      var clName = COMMCLNAME + "_6183";

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

   var cl = db.getCS( csName ).getCL( clName );
   cl.insert( { a: "1", b: "", c: "test", d: { $regex: { a: "^9" } }, e: "test" } );
}

function selectRecs ( csName, clName )
{
   println( "\n---Begin to select records." );

   var rc = db.exec( "select a+2,b-2,c*2,d/2,e%2 from " + csName + "." + clName );
   return rc;
}

function checkResult ( rc )
{
   println( "\n---Begin to check result." );

   //compare the records for rc[0]
   var expA = null;
   var actA = rc.current().toObj().a;
   if( expA !== actA )
   {
      throw buildException( "checkResult", null, "[ compare results ]",
         "[a:" + expA + "]",
         "[a:" + actA + "]" );
   }
}