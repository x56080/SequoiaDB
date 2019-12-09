/******************************************************************************
*@Description : seqDB-11086:_id�� �ֶ����������ʽ����'$'��ͷ������'.';�ֶ���Ϊ��                   
*@Author      : 2019-5-29  wuyan modify
******************************************************************************/
main();
function main ()
{
   var clName = "insert11086";
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
   var illegalFieldName = ["$a", "a.b"];
   for( var i = 0; i < illegalFieldName.length; i++ )
   {
      try
      {
         var fieldName = illegalFieldName[i];
         var obj = {};
         obj[fieldName] = "test" + i;
         cl.insert( { "_id": obj } );
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






