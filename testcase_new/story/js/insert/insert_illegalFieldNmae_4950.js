/******************************************************************************
*@Description : seqDB-4950:�ֶ����������ʽ����'$'��ͷ������'.';�ֶ���Ϊ��                   
*@Author      : 2019-5-29  wuyan modify
******************************************************************************/
//http://jira:8080/browse/SEQUOIADBMAINSTREAM-4549
//main();
function main ()
{
   var clName = "insert4950";
   var cl = readyCL( clName );

   insertRecordsWithIllegalFieldName( cl );
   var actRecords = cl.find();
   var expRecords = [];
   checkRec( actRecords, expRecords );

   cleanCL( clName );
}

function insertRecordsWithIllegalFieldName ( cl )
{
   println( "---begin to insert." );
   var illegalFieldName = ["$a", "a.b", ""];
   for( var i = 0; i < illegalFieldName.length; i++ )
   {
      try
      {
         var fieldName = illegalFieldName[i];

         var obj = {};
         obj[fieldName] = "test" + i;
         cl.insert( obj );
         throw "insert should be fail!";
      }
      catch( e )   
      {
         if( -6 !== e )
         {
            throw buildException( "insertRecords", e );
         }
      }

   }

}




