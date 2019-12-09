/************************************************************************
*@Description:  seqDB-1970:使用$group时指定的_id字段值不存在_SD.aggregate.01.023
*@Author:  2015/10/21  huangxiaoni
************************************************************************/
main();

function main ()
{
   try
   {
      var clName = COMMCLNAME + "_aggre"

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

   cl.insert( { no: 1, name: "Tom" } );
}

function aggreOper ( cl )
{
   println( "\n---Begin to aggregate records." );

   var rc = cl.aggregate( { $group: { _id: "$test" } } );

   return rc;
}

function checkResult ( rc )
{
   println( "\n---Begin to check result." );
   //compare the returned records
   var no = rc.current().toObj()["no"];
   var name = rc.current().toObj()["name"];
   //expect results:{no:1,score:80}
   var expNo = 1;
   var expName = "Tom";
   if( no !== expNo || name !== expName )
   {
      throw buildException( "checkResult", null, "[compare the records]",
         "[no:" + expNo + ",name:" + expName + "]",
         "[no:" + no + ",name:" + name + "]" );
   }

   //compare the number of records
   var nextRecs = rc.next();
   if( nextRecs !== undefined )
   {
      throw buildException( "checkResult", null, "[compare the number of records]",
         "[nextRecs:undefined]", "[nextRecs:" + nextRecs + "]" )
   }
}