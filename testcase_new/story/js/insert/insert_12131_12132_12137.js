/******************************************************************************
*@Description : seqDB-12131:插入数据为特殊字符
                seqDB-12132:插入数据为多层嵌套对象
                seqDB-12133:插入sequoadb支持所有类型数据
*@Author      : 2019-5-29  wuyan modify
******************************************************************************/
try
{
   main();
}
catch( e )
{
   if( e.constructor === Error )
   {
      println( e.stack );
   }
   throw e;
}

function main ()
{
   var clName = COMMCLNAME + "_12131";
   var cl = readyCL( clName );

   var expRecords = insertRecords( cl );
   var actRecords = cl.find().sort( { no: 1 } );
   commCompareResults( actRecords, expRecords, false );
   commDropCL( db, COMMCSNAME, clName );
}

function insertRecords ( cl )
{
   var insertObj1 = { _id: 1, "no": 0, "!@#%^&": "&*()?><" };
   var objValue = { "a": { "a": { "a": { "a": { "a": { "a": { "a": { "a": { "a": { "a": 100 } } } } } } } } } };
   var insertObj2 = { _id: 2, "no": 2, name: "��Ǯ�����" };;

   var doc = [];
   doc.push( insertObj1 );
   doc.push( { _id: 3, "no": 1, obj: objValue } );
   doc.push( insertObj2 );
   cl.insert( doc );
   return doc;
}
