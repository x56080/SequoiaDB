/******************************************************************************
*@Description : seqDB-6710:����binary����$type�Ƿ�                  
*@Author      : 2019-5-29  wuyan modify
******************************************************************************/
main();
function main ()
{
   var clName = "insert6710";
   var cl = readyCL( clName );

   //test $type is 256
   insertWithTypeErrorA( cl );
   //test $type is -1  
   insertWithTypeErrorB( cl );

   cleanCL( clName );
}

function insertWithTypeErrorA ( cl )
{
   println( "---begin to insert binary with $type:256." );
   try
   {
      var binary = { "$binary": "aGVsbG8gd29ybGQ=", "$type": "256" };
      cl.insert( { binary: binary } );
      throw "insert should be fail!";
   }
   catch( e )   
   {
      if( -6 !== e )
      {
         throw buildException( "insertRecords", e );
      }
   }

   var expCount = 0;
   var count = cl.count();
   if( Number( expCount ) !== Number( count ) )
   {
      throw buildException( "insertRecords", "count value error", "count()", expCount, count );
   }
}

function insertWithTypeErrorB ( cl )
{
   println( "---begin to insert binary with $type:-1." );
   var binary = { "$binary": "aGVsbG8gd29ybGQ=", "$type": "-1" };
   cl.insert( { binary: binary } );

   //С����Сֵʱ���Զ�����Ϊ���ֵ255
   var expBinaryValue = { "$binary": "aGVsbG8gd29ybGQ=", "$type": "255" };
   var expRecords = [];
   expRecords.push( { binary: expBinaryValue } );

   var actRecords = cl.find( {}, { "_id": { "$include": 0 } } );
   checkRec( actRecords, expRecords );
}






