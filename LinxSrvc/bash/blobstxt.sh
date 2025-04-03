#!/bin/bash
# 功能：删除包含指定内容的行/插入指定内容到指定行
idx=0
for tmp in $(ls -- *.prototxt)
do
  file=${tmp}
  echo "${tmp}"
  if [ 99 -lt $(< "$file" wc -l) ]; then         #如果文件大于99行
    sed -n "1,/blobs {/p" "${tmp}" > "h_${idx}"  #第一行到含第一个'blobs'的行写入head
    sed "1,/blobs {/d" "${tmp}" > "t_${idx}"     #删除第一行到含第一个'blobs'的行剩余内容写入tail
    tmp=h_${idx}                                 #当处理debug文件时 tmp=head
    echo "[debug: idx=${idx} file=${tmp}]"
  fi
  sed -i "{:loop /dim: 1$/!{N;b loop};s///g}" "${tmp}"   #查找关联'dim: 1'的行，用空格取代这些行
  sed -i "7a#      dim: 1" "${tmp}"                      #第7行插入' dim: 1'变为第8行
  sed -i "8s/^#//" "${tmp}"                              #这两行是为了调整对齐格式
  sed -i "/^ *$/d" "${tmp}"
  if [ 99 -lt $(< "$file" wc -l) ]; then
    #awk 'NR==FNR{a[FNR]=$0;}NR>FNR{print a[FNR]" "$0;}' $tmp t_$idx > $file
    cat "${tmp}" > "${file}"
    cat "t_${idx}" > "${file}"  #拼接
    rm -f "${tmp}" "t_${idx}"   #删除中间文件
  fi
  # echo `grep -C 1 'blobs {' $tmp`;
  let idx=$idx+1
done
#rm -f log_*
