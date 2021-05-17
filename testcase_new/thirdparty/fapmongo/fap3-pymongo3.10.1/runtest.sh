# 本地执行所有用例，本地调测用
for file in $(ls test_*.py)
do 
  echo $file 
  python -m unittest $file
done
