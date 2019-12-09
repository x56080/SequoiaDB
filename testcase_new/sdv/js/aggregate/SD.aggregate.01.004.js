/************************************************************************
*@Description:  $match+$match组合查询_SD.aggregate.01.004
*@Author:  2015/10/10  huangxiaoni
************************************************************************/
main();

function main ()
{
   try
   {
      var clName = COMMCLNAME + "_aggre";

      var cl = readyCL( clName );

      insertRecs( cl );
      var rc = aggreOper( cl );
      checkResult( rc );

      cleanCL( clName );
   }
   catch( e )
   {
      throw e;
   }
}

function insertRecs ( cl )
{
   println( "\n---Begin to insert records." );

   cl.insert( { no: 2, score: 80, name: "Tom" } );
   cl.insert( { no: 1, score: 80, name: "Json" } );
}

function aggreOper ( cl )
{
   println( "\n---Begin to aggregate records." );

   var rc = cl.aggregate( { $match: { score: 80 } }, { $match: { name: "Tom" } } );

   return rc;
}

function checkResult ( rc )
{
   println( "\n---Begin to check result." );

   //compare the returned records
   var no = rc.current().toObj()["no"];
   var score = rc.current().toObj()["score"];
   var name = rc.current().toObj()["name"];
   //expect results:{no:2,score:80,name:"Tom"}
   var expNo = 2;
   var expScore = 80;
   var expName = "Tom";
   if( no !== expNo || score !== expScore || name !== expName )
   {
      throw buildException( "checkResult", null, "[compare the records]",
         "[no:" + expNo + ",score:" + expScore + ",name:" + expName + "]",
         "[no:" + no + ",score:" + score + ",name:" + name + "]" );
   }

   //compare the number of records
   var nextRecs = rc.next();
   if( nextRecs !== undefined )
   {
      throw buildException( "checkResult", null, "[compare the number of records]",
         "[nextRecs:undefined]", "[nextRecs:" + nextRecs + "]" )
   }
}