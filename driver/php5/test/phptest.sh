#!/bin/bash
../../../tools/server/php_linux/bin/php -d extension=../build/dd/libsdbphp-5.4.6.so ../../test/php/tools/phpunit.phar --log-junit phptest.error.txt ../../test/php/type/typeTest.php
if [ $? != 0 ] ; then 
   exit 1
fi

../../../tools/server/php_linux/bin/php -d extension=../build/dd/libsdbphp-5.4.6.so ../../test/php/tools/phpunit.phar --log-junit phptest.error.txt ../../test/php/SequoiaDB/sdbTest.php
if [ $? != 0 ] ; then
   exit 1
fi

../../../tools/server/php_linux/bin/php -d extension=../build/dd/libsdbphp-5.4.6.so ../../test/php/tools/phpunit.phar --log-junit phptest.error.txt ../../test/php/SequoiaDB/csTest.php
if [ $? != 0 ] ; then
   exit 1
fi

../../../tools/server/php_linux/bin/php -d extension=../build/dd/libsdbphp-5.4.6.so ../../test/php/tools/phpunit.phar --log-junit phptest.error.txt ../../test/php/SequoiaDB/clTest.php
if [ $? != 0 ] ; then
   exit 1
fi

../../../tools/server/php_linux/bin/php -d extension=../build/dd/libsdbphp-5.4.6.so ../../test/php/tools/phpunit.phar --log-junit phptest.error.txt ../../test/php/SequoiaDB/domainTest.php
if [ $? != 0 ] ; then
   exit 1
fi


../../../tools/server/php_linux/bin/php -d extension=../build/dd/libsdbphp-5.4.6.so ../../test/php/tools/phpunit.phar --log-junit phptest.error.txt ../../test/php/SequoiaDB/groupTest.php
if [ $? != 0 ] ; then
   exit 1
fi

../../../tools/server/php_linux/bin/php -d extension=../build/dd/libsdbphp-5.4.6.so ../../test/php/tools/phpunit.phar --log-junit phptest.error.txt ../../test/php/SequoiaDB/sqlTest.php
if [ $? != 0 ] ; then
   exit 1
fi

../../../tools/server/php_linux/bin/php -d extension=../build/dd/libsdbphp-5.4.6.so ../../test/php/tools/phpunit.phar --log-junit phptest.error.txt ../../test/php/SequoiaDB/configureTest.php
if [ $? != 0 ] ; then
   exit 1
fi

../../../tools/server/php_linux/bin/php -d extension=../build/dd/libsdbphp-5.4.6.so ../../test/php/tools/phpunit.phar --log-junit phptest.error.txt ../../test/php/SequoiaDB/authenticateTest.php
if [ $? != 0 ] ; then
   exit 1
fi

../../../tools/server/php_linux/bin/php -d extension=../build/dd/libsdbphp-5.4.6.so ../../test/php/tools/phpunit.phar --log-junit phptest.error.txt ../../test/php/SequoiaDB/procedureTest.php
if [ $? != 0 ] ; then
   exit 1
fi

../../../tools/server/php_linux/bin/php -d extension=../build/dd/libsdbphp-5.4.6.so ../../test/php/tools/phpunit.phar --log-junit phptest.error.txt ../../test/php/SequoiaDB/transactionTest.php
if [ $? != 0 ] ; then
   exit 1
fi

../../../tools/server/php_linux/bin/php -d extension=../build/dd/libsdbphp-5.4.6.so ../../test/php/tools/phpunit.phar --log-junit phptest.error.txt ../../test/php/SequoiaDB/backupoffline.php
if [ $? != 0 ] ; then
   exit 1
fi

../../../tools/server/php_linux/bin/php -d extension=../build/dd/libsdbphp-5.4.6.so ../../test/php/tools/phpunit.phar --log-junit phptest.error.txt ../../test/php/SequoiaDB/taskTest.php
if [ $? != 0 ] ; then
   exit 1
fi

../../../tools/server/php_linux/bin/php -d extension=../build/dd/libsdbphp-5.4.6.so ../../test/php/tools/phpunit.phar --log-junit phptest.error.txt ../../test/php/SequoiaDB/sessionTest.php
if [ $? != 0 ] ; then
   exit 1
fi

../../../tools/server/php_linux/bin/php -d extension=../build/dd/libsdbphp-5.4.6.so ../../test/php/tools/phpunit.phar --log-junit phptest.error.txt ../../test/php/SequoiaCS/csTest.php
if [ $? != 0 ] ; then
   exit 1
fi

../../../tools/server/php_linux/bin/php -d extension=../build/dd/libsdbphp-5.4.6.so ../../test/php/tools/phpunit.phar --log-junit phptest.error.txt ../../test/php/SequoiaCS/clTest.php
if [ $? != 0 ] ; then
   exit 1
fi

../../../tools/server/php_linux/bin/php -d extension=../build/dd/libsdbphp-5.4.6.so ../../test/php/tools/phpunit.phar --log-junit phptest.error.txt ../../test/php/SequoiaCollection/clTest.php
if [ $? != 0 ] ; then
   exit 1
fi

../../../tools/server/php_linux/bin/php -d extension=../build/dd/libsdbphp-5.4.6.so ../../test/php/tools/phpunit.phar --log-junit phptest.error.txt ../../test/php/SequoiaCollection/indexTest.php
if [ $? != 0 ] ; then
   exit 1
fi

../../../tools/server/php_linux/bin/php -d extension=../build/dd/libsdbphp-5.4.6.so ../../test/php/tools/phpunit.phar --log-junit phptest.error.txt ../../test/php/SequoiaCollection/recordTest.php
if [ $? != 0 ] ; then
   exit 1
fi

../../../tools/server/php_linux/bin/php -d extension=../build/dd/libsdbphp-5.4.6.so ../../test/php/tools/phpunit.phar --log-junit phptest.error.txt ../../test/php/SequoiaCollection/lobTest.php
if [ $? != 0 ] ; then
   exit 1
fi

../../../tools/server/php_linux/bin/php -d extension=../build/dd/libsdbphp-5.4.6.so ../../test/php/tools/phpunit.phar --log-junit phptest.error.txt ../../test/php/SequoiaCursor/cursorTest.php
if [ $? != 0 ] ; then
   exit 1
fi

../../../tools/server/php_linux/bin/php -d extension=../build/dd/libsdbphp-5.4.6.so ../../test/php/tools/phpunit.phar --log-junit phptest.error.txt ../../test/php/SequoiaDomain/domainTest.php
if [ $? != 0 ] ; then
   exit 1
fi

../../../tools/server/php_linux/bin/php -d extension=../build/dd/libsdbphp-5.4.6.so ../../test/php/tools/phpunit.phar --log-junit phptest.error.txt ../../test/php/SequoiaGroup/groupTest.php
if [ $? != 0 ] ; then
   exit 1
fi

../../../tools/server/php_linux/bin/php -d extension=../build/dd/libsdbphp-5.4.6.so ../../test/php/tools/phpunit.phar --log-junit phptest.error.txt ../../test/php/SequoiaGroup/nodeTest.php
if [ $? != 0 ] ; then
   exit 1
fi

../../../tools/server/php_linux/bin/php -d extension=../build/dd/libsdbphp-5.4.6.so ../../test/php/tools/phpunit.phar --log-junit phptest.error.txt ../../test/php/SequoiaNode/nodeTest.php
if [ $? != 0 ] ; then
   exit 1
fi

../../../tools/server/php_linux/bin/php -d extension=../build/dd/libsdbphp-5.4.6.so ../../test/php/tools/phpunit.phar --log-junit phptest.error.txt ../../test/php/SequoiaLob/lobTest.php
if [ $? != 0 ] ; then
   exit 1
fi

echo ------------------ All Successful! ------------------
