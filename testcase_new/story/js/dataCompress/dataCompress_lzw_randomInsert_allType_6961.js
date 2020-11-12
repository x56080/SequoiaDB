/************************************************************************
@Description:    seqDB-6961:批量插入覆盖所有支持的数据类型_st.compress.07.004
@input:    
         1 create CL[Compressed:false] ;
           create CL[Compressed: true, CompressionType: "lzw"] ;
         2 get random data types
         3 get random value of mult data types
         4 insert, the records is random data types and values ;
         5 check records, get random records, then compare the records;
           check records for each node in the group;
           check compressed rate. 
@output:   successfull
@Author:   
           2016/3/23   XiaoNi Huang init
************************************************************************/
main( test );

function test ()
{
   var noCSName = COMMCSNAME + "_no";
   var lzwCSName = COMMCSNAME + "_lzw";
   var noCLName = COMMCLNAME + "_no";
   var lzwCLName = COMMCLNAME + "_lzw";
   var rgName = getDataGroupsName()[0];
   var dtNumber = 80000;  //records number of single data type
   var checkRecsNum = 2; //get random 3 records

   commDropCS( db, noCSName, true, "Failed to drop CS[" + noCSName + "]." );
   commDropCS( db, lzwCSName, true, "Failed to drop CS[" + lzwCSName + "]." );

   commCreateCS( db, noCSName, false, "Failed to create CS[" + noCSName + "]." );
   commCreateCS( db, lzwCSName, false, "Failed to create CS[" + lzwCSName + "]." );

   var noCL = createCL( noCSName, noCLName, rgName, false );
   var lzwCL = createCL( lzwCSName, lzwCLName, rgName, true, "lzw" );

   var tmpTypes = ["int", "long", "float", "OID", "bool", "date", "timestamp",
      "binary", "regex", "object", "array", "null", "string"];
   var dataTypes = getRdmType( tmpTypes );    //random data type
   var dataValues = getRdmValue( dataTypes );  //random value
   insertRecs( noCL, noCSName, noCLName, dtNumber, dataTypes, dataValues );
   insertRecs( lzwCL, lzwCSName, lzwCLName, dtNumber, dataTypes, dataValues );

   var totalNum = dtNumber * dataTypes.length;
   checkRecs( lzwCL, dtNumber, checkRecsNum, dataTypes, dataValues );
   checkNodeCnt( lzwCSName, lzwCLName, rgName, totalNum );
   checkCompressedRate( noCSName, lzwCSName );

   clearCS( db, noCSName );
   clearCS( db, lzwCSName );
}

function getRdmType ( tmpTypes )
{

   var dataTypes = new Array;
   var num = 10;
   for( k = 0; k < num; k++ )
   {
      var i = Math.random() * tmpTypes.length;
      i = parseInt( i );
      dataTypes.push( tmpTypes[i] );
   }

   return dataTypes;
}

function getRdmValue ( dataTypes )
{

   var rd = new commDataGenerator();

   var dataValues = new Array;
   for( i = 0; i < dataTypes.length; i++ )
   {
      var tmpValues = rd.getValue( dataTypes[i] );

      dataValues.push( tmpValues );
   }

   return dataValues;
}

function insertRecs ( cl, csName, clName, dtNumber, dataTypes, dataValues )
{

   var i = 0;
   var h = 0;
   while( i < dataValues.length )
   {

      for( k = 0; k < dtNumber; k += 40000 )
      {
         var doc = [];
         for( j = 0 + k; j < 40000 + k; j++ )
         {
            doc.push( { a: j + h, dataType: dataTypes[i], typeValue: dataValues[i] } )
         };
         cl.insert( doc );
      }

      h = h + dtNumber;
      i++;
   }
}

function checkRecs ( cl, dtNumber, checkRecsNum, dataTypes, dataValues )
{

   //get random records, compare the records
   var h = 0;
   for( i = 0; i < dataTypes.length; i++ )
   {

      for( j = 0; j < checkRecsNum; j++ )
      {
         var k = Math.random() * dtNumber + h;
         k = parseInt( k, 10 );  //10: decimal system
         if( dataTypes[i] === "regex" )
         {
            var recsCnt = cl.find( { a: k, dataType: dataTypes[i], typeValue: { $et: dataValues[i] } } ).count();
         }
         else
         {
            var recsCnt = cl.find( { a: k, dataType: dataTypes[i], typeValue: dataValues[i] } ).count();
         }
         var expctCnt = 1;
         assert.equal( recsCnt, expctCnt );
      }

      h = h + dtNumber;
   }
}