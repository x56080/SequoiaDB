1.download from git

2.compile and package
mvn clean package assembly:single
or
mvn clean package -Dmaven.test.skip=true assembly:single

3.run

case 1: init job file
java -jar scm_meta-1.0-SNAPSHOT-jar-with-dependencies.jar --host 192.168.17.37:50000 -u "" -w "" -a initjob -f ./job.list -c main2020.main,main2021.main,test.test

case 2: repair data
java -jar scm_meta-1.0-SNAPSHOT-jar-with-dependencies.jar --host 192.168.17.37:50000 -u "" -w "" -a repair -f ./job.list -j 54 -b 10000