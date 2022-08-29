#!/bin/bash

dir=`dirname $0`
echo $dir
cd ${dir}
mkdir -p build
cd build
cmake .. -G Ninja
ninja -j 4

./QtVTKRenderWindows /media/eton/statics/08.dicom-datas/BRAINIX/BRAINIX/MRs/T1-3D-FFE-C-801/

echo "finish."
