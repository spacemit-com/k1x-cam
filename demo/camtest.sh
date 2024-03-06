#!/bin/sh

dumpdir="/data/vendor_de/camera"
resDir="/tmp/cam_jindie/dump"
jsonDir="/tmp/cam_jindie/cfgs"
fPrefix="camtest_"
fSuffix=".json"

rm -rf ${dumpdir} && mkdir -p ${dumpdir}

i=0
until [ ${i} -eq 7 ]
do
    echo "----------------cam-test case${i} start----------------------"
    cd ${jsonDir}/${i}
    files=`ls`
    for file in ${files}
    do
        echo "=====test ${file} start====="
        /tmp/cam_jindie/cam-test ./${file}
        dir=${file#${fPrefix}}
        dir=${dir%${fSuffix}}
        subdir="${resDir}/${i}/${dir}"
        mkdir -p ${subdir} && rm -rf ${subdir}/*
        mv ${dumpdir}/* ${subdir}
        echo "=====test ${file} end====="
        echo
    done
    cd -

    echo "----------------cam-test case${i} end----------------------"
    echo
    echo
    let i++
done
