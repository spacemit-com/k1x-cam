#------------------------------------------------------------
# @Copyright 2022-2023 SPACEMIT. All rights reserved.
# @Use of this source code is governed by a BSD-style license
# @that can be found in the LICENSE file.
# @
# @Author: ZRong(zhirong.li@spacemit.com)
# @Date: 2024-01-17 09:26:17
# @LastEditTime: 202i4-02-03 15:49:21
# @Description:
#------------------------------------------------------------

prefix=/usr
exec_prefix=${prefix}
libdir=${prefix}/lib/riscv64-linux-gnu
includedir=${prefix}/include

Name: k1x-cam
Description: Spacemit K1x Cam Process Platform
Requires.private:
Version: 0.1.0
Libs: -L${libdir} -lsdkcam -lisp -lcpp -lvi -lcam_sensors -ltuningtools -lispeagle -lcppfw-2.0
Libs.private:
Cflags: -I${includedir}

