/************************************************************************
*@Description:  seqDB-1964:$project中指定的字段名不存在_SD.aggregate.01.017
*@Author:  2015/10/21  huangxiaoni
************************************************************************/
main();

function main ()
{
   try
   {
      var clName = COMMCLNAME + "_aggre"

      var cl = readyCL();

      insertRecs( cl );
      var rc = aggreOper( cl );
      checkResult( rc );

      cleanCL();
   }
   catch( e )
   {
      throw e;
   }
}

function insertRecs ( cl )
{
   println( "\n---Begin to insert records." );

   cl.insert( { no: 1, score: 80 } );
}

function aggreOper ( cl )
{
   println( "\n---Begin to aggregate records." );

   var rc = cl.aggregate( { $project: { name: 1 } } );

   return rc;
}

function checkResult ( rc )
{
   println( "\n---Begin to check result." );

   //compare the returned records
   var name = rc.current().toObj()["name"];
   var no = rc.current().toObj()["no"];
   //expect results:{name:null}
   var expName = null;
   var expNo = undefined;
   if( name !== expName || no !== expNo )
   {
      throw buildException( "checkResult", null, "[compare the records]",
         "[name:" + expName + ",no:" + expNo + "]",
         "[name:" + name + ",no:" + no + "]" );
   }

   //compare the number of records
   var nextRecs = rc.next();
   if( nextRecs !== undefined )
   {
      throw buildException( "checkResult", null, "[compare the number of records]",
         "[nextRecs:undefined]", "[nextRecs:" + nextRecs + "]" )
   }
}