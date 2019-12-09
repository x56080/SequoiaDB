/************************************************************************
*@Description:   seqDB-6057:hint指定的索引不存在_st.sql.hint.009
*@Author:  2016/7/13  huangxiaoni
************************************************************************/
main();

function main ()
{
   try
   {
      var csName = COMMCSNAME;
      var clName = COMMCLNAME + "_6057";
      var idxName = CHANGEDPREFIX + "_idx";

      dropCL( csName, clName, true, "Failed to drop cl in the begin." );
      createCL( csName, clName, true, true, "Failed to create cl." );

      insertRecs( csName, clName );
      var preTotalIndexRead = snapshot();
      var rtRecsArray = selectRecs( csName, clName, idxName );
      var aftTotalIndexRead = snapshot();

      checkResult( rtRecsArray, preTotalIndexRead, aftTotalIndexRead );

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

   db.execUpdate( "insert into " + csName + "." + clName + "(a,b,c) values(1,1,1)" );
   db.execUpdate( "insert into " + csName + "." + clName + "(a,b,c) values(2,2,2)" );
}

function selectRecs ( csName, clName, idxName )
{
   println( "\n---Begin to select records." );

   var rc = db.exec( "select t1.c from " + csName + "." + clName + " as t1 where t1.b = 2 /*+use_index(t1, " + idxName + ")*/" );

   var rtRecsArray = [];
   while( tmpRecs = rc.next() )
   {
      rtRecsArray.push( tmpRecs.toObj() );
   }
   return rtRecsArray;
}

function snapshot ()
{
   println( "\n---Begin to exec snapshot(6) to get TotalIndexRead." );

   var TotalIndexRead = db.snapshot( 6 ).current().toObj()["TotalIndexRead"];

   return TotalIndexRead;
}

function checkResult ( rtRecsArray, preTotalIndexRead, aftTotalIndexRead )
{
   println( "\n---Begin to check result." );

   //compare the records
   var expRecsCount = 1;
   var expC = 2;
   var actRecsCount = rtRecsArray.length;
   var actC = rtRecsArray[0]["c"];
   if( expRecsCount !== actRecsCount || expC !== actC )
   {
      throw buildException( "checkResult", null, "[ compare records ]",
         "[recsCount:" + expRecsCount + ", c:" + expC + "]",
         "[recsCount:" + actRecsCount + ", c:" + actC + "]" );
   }

   //judge whether to walk index
   println( "   preTotalIndexRead: " + preTotalIndexRead );
   println( "   aftTotalIndexRead: " + aftTotalIndexRead );
   var times = aftTotalIndexRead - preTotalIndexRead;
   var expWalkIndex = false;
   //init "walkIndex"
   var walkIndex = false;
   if( times === 0 )
   {
      println( "   walkIndex: " + walkIndex );
   }
   else
   {
      walkIndex = true;
      /*
      throw buildException("checkResult", null, "[ compare index ]", 
                       "[walkIndex:"+ expWalkIndex +"]",
                       "[walkIndex:"+ walkIndex +"]");
      */
   }

}