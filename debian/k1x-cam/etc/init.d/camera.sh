#!/bin/sh

###
 # Copyright 2022-2023 SPACEMIT. All rights reserved.
 # Use of this source code is governed by a BSD-style license
 # that can be found in the LICENSE file.
 # 
 # @Author: ZRong(zhirong.li@spacemit.com)
 # @Date: 2023-09-13 14:20:45
 # @LastEditTime: 2023-09-20 14:04:40
 # @Description: 
### 

cam_sensor_nodes=$(find /dev/ -name "cam_sensor*")

for node in $cam_sensor_nodes; do
    chmod 777 $node
done

isp_pipe_nodes=$(find /dev/ -name "mars11isp-pipe*")

for node in $isp_pipe_nodes; do
    chmod 777 $node
done

video_nodes=$(find /dev/ -name "video*")

for node in $video_nodes; do
    chmod 666 $node
done

chmod 666 /dev/dma_heap/system

