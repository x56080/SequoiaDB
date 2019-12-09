/******************************************************************************
*@Description : seqDB-12131:��������Ϊ�����ַ�
                seqDB-12132:��������Ϊ���Ƕ�׶���
                seqDB-12137:�����¼ֵΪ�����ַ�                
*@Author      : 2019-5-29  wuyan modify
******************************************************************************/
main();
function main ()
{
   var clName = "insert12131";
   var cl = readyCL( clName );

   var expRecords = insertRecords( cl );
   var actRecords = cl.find().sort( { no: 1 } );
   checkRec( actRecords, expRecords );

   cleanCL( clName );
}

function insertRecords ( cl )
{
   println( "---begin to insert records." );
   //test testcase-12131:��������Ϊ�����ַ�
   var insertObj1 = { "no": 0, "!@#%^&": "&*()?><" };
   //test testcase-12132:��������Ϊ���Ƕ�׶��� 
   var objValue = { "a": { "a": { "a": { "a": { "a": { "a": { "a": { "a": { "a": { "a": 100 } } } } } } } } } };
   //test testcase-12137:��������Ϊ�����ַ�
   var insertObj2 = { "no": 2, name: "��Ǯ�����" };;

   var doc = [];
   doc.push( insertObj1 );
   doc.push( { "no": 1, obj: objValue } );
   doc.push( insertObj2 );
   cl.insert( doc );

   return doc;
}
