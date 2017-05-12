#!/bin/bash
######################################################################
#@decript:  judge that old version can upgrade to new version or not
#@author:   Ting YU 2016-04-13 
#@input:    $1: old version  
#           $2: old edition
#           $3: new version  
#           $4: new edition
#           $5: version.conf directory and file name
#@return :  false: can not upgrade
#           true:  can upgrade 
#@return code: 0:normal 1:format error 2/3:unexpect error
######################################################################
function checkVsnPara()
{
   local vsn="$1"
   local vsnArr=(`tr "." " " <<< $vsn`) 
   local len=${#vsnArr[@]} 
    
   #can't be null   
   if [ "X$vsn" == "X" ] 
   then 
      echo "error format of version : $vsn (null value)"
      exit 1
   fi
     
   #can't be more than 3, eg 1.12.5.1               
   if [ $len -gt 3 ] 
      then
         echo "error format of version : $vsn"
         exit 1
   fi
   
   #make sure number value
   for v in ${vsnArr[@]}                
   do 
      n=`echo "$v" | grep "[^0-9]" | wc -l`
      if [ $n -gt 0 ]; 
      then 
         echo "error format of version : $vsn"
         exit 1 
      fi
   done        
}

function checkEdtPara()
{
   local edt="$1"
   
   #can't be null  
   if [ "X$edt" == "X" ] 
   then 
      echo "error format of editon : $edt (null value)"
      exit 1 
   fi 
   
   #should be "C" or "E"
   if [ $edt == "C" ]; then return 0; fi
   if [ $edt == "E" ]; then return 0; fi
   
   echo "error format of editon : $edt" 
   exit 1   
}

# add zero to version, eg "1.12" -> "1.12.0"
function formatVsn()
{
   local vsn="$1"
   local vsnArr=(`tr "." " " <<< $vsn`) 
   local len=${#vsnArr[@]} 
    
   if [ $len -eq 1 ]; then vsn="${vsn}.0.0"; echo $vsn; return 0; fi
   if [ $len -eq 2 ]; then vsn="${vsn}.0";   echo $vsn; return 0; fi 
   
   echo $vsn; return 0;     
}

#compare version1 to version2 ,if v1 < v2, echo "less"
function compareTwoVsn()
{
   local vsn1=$1
   local vsn2=$2 
   local vsnArr1=(`tr "." " " <<< $vsn1`)
   local vsnArr2=(`tr "." " " <<< $vsn2`)       
   
   local result="null"
   
   for(( i=0; i<$vsnLen; i++ ))            
   do 
      local v1=${vsnArr1[i]}
      local v2=${vsnArr2[i]}
      
      if [ $v1 -eq $v2 ] 
      then
         result="equal"
         continue
      fi 
      
      if [ $v1 -gt $v2 ] 
      then
         result="greater"
         break
      else
         result="less"
         break   
      fi        
   done
   
   echo $result
}

#match version in file version.conf
#if match, echo "match"; if not, echo "not match"
function vsnMatch()
{
   local vsn=$1
   local vsnLmt=$2 
   local result="not_match"  
    
   if [ $vsnLmt == "*" ]
   then 
      result="match";  
      echo $result  
      return 0; 
   fi

   local vsnArr1=(`tr "." " " <<< $vsn`)
   local vsnArr2=(`tr "." " " <<< $vsnLmt`)  
   local len=${#vsnArr2[@]} 
     
   for(( i=0; i<$len; i++ ))            
   do
      local v1=${vsnArr1[i]}
      local v2=${vsnArr2[i]}

      if [ $v2 == "*" ] 
      then
         result="match"
         continue
      fi 

      if [ $v1 -eq $v2 ] 
      then
         result="match"
         continue
      else
         result="not_match" 
         break                          
      fi 
   done
   
   echo $result                    
} 

function edtMatch()
{
   local edt=$1
   local edtLmt=$2   
   result="not_match"
  
   if [ $edtLmt == "*" ];  then result="match"; fi       
   if [ $edtLmt == $edt ]; then result="match"; fi  
            
   echo $result         
}  

function matchConf()
{
   local oldVsn=$1    
   local oldEdt=$2
   local newVsn=$3
   local newEdt=$4   
   local compatible=1  
   
   while read line
   do
      
      #get every line from version.conf, eg:  1.6.* * *  * 0
      n=`echo "$line" | grep "^#" | wc -l`
      if [ $n != 0 ]; then continue; fi      
      if [ "X$line" == "X" ]; then continue; fi
                 
      set -f paraConf
      paraConf=($line)  
      
      #match old version
      result=`vsnMatch $oldVsn ${paraConf[0]}`
      if [ $result == "not_match" ]; then continue; fi        
              
      #match old edition
      result=`edtMatch $oldEdt ${paraConf[1]}`
      if [ $result == "not_match" ]; then continue; fi
      
      #match new version
      result=`vsnMatch $newVsn ${paraConf[2]}`
      if [ $result == "not_match" ]; then continue; fi
      
      #match new edition
      result=`edtMatch $newEdt ${paraConf[3]}`
      if [ $result == "not_match" ]; then continue; fi
      
      compatible=${paraConf[4]} 
      break  
   done < ${confDirName}
   
   if [ $compatible -eq 1 ]; then echo true;   return 0; fi
   if [ $compatible -eq 0 ]; then echo false;  return 0; fi
   return 1
}    
   
#################### main entry ############################
oldVsn=$1    
oldEdt=$2
newVsn=$3
newEdt=$4
confDirName=$5
   
checkVsnPara $oldVsn
checkEdtPara $oldEdt
checkVsnPara $newVsn
checkEdtPara $newEdt

oldVsn=`formatVsn $oldVsn`
newVsn=`formatVsn $newVsn`

vsnLen=3
result=`compareTwoVsn $oldVsn $newVsn`

if [ $result == "greater" ]; then echo false; exit 0; fi
if [ $result == "equal" ];   then echo true;  exit 0; fi 
if [ $result == "null" ];    then echo false; exit 2; fi
if [ $result == "less" ]
then 
   isCompatible=`matchConf $oldVsn $oldEdt $newVsn $newEdt`
   if [ $? -ne 0 ]; then exit 3; fi
   echo $isCompatible
   exit 0
fi  
