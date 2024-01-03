#! /usr/bin/env python3
# -*- coding: utf-8 -*-
import sys
import os


def replace_symbol(str_line, write_file):
    write_file.write('@@' + str_line)

    find_name = 'CSpecialEffect'
    key_index = str_line.find(find_name)
    if key_index >= 0:
        sde_setting = 1
    else:
        sde_setting = 0

    find_name = 'CAFFilter'
    key_index = str_line.find(find_name)
    if key_index >= 0:
        caf_setting = 1
    else:
        caf_setting = 0

    return sde_setting, caf_setting


def normal_write_line(line1, line2,line3,sde_setting, caf_setting, write_file,delete_space):
    #skip some symbol, such as {}, space
    skip=0
    if sde_setting == 1:
        if line3.find('m_bManualMode_0') >= 0:
            skip=2
            delete_space = 8
        elif line3.find('m_bManualMode_1') >= 0:
            skip=2
        elif line3.find('m_bManualMode_2') >= 0:
            skip=2
        elif line3.find('m_bManualMode_3') >= 0:
            skip=2
        elif line3.find('m_bManualMode_4') >= 0:
            skip=2
        elif line3.find('m_bManualMode_5') >= 0:
            skip=2
        elif line3.find('m_nLuxLUT') >= 0:
            skip=2
            delete_space = 0

    if caf_setting == 1:
        if line2.find('m_pPDShiftPositionLUT_0_0') >= 0:
            skip=1
            delete_space = 4
        elif line2.find('m_bCAFForce') >= 0:
            skip=1
            delete_space = 0

    if skip==0 and delete_space == 0:
        write_file.write(line1)
    elif skip==0 and delete_space != 0:
        write_file.write(line1[delete_space:])

    return skip,delete_space

def ProcPath(path):
    file_list = os.listdir(path)
    for f in file_list:
        f_lower = f.lower()
        if f_lower.endswith('.h') and ("asr_setting" not in f_lower) and ("asr.h" not in f_lower):
            raw_name = '%s%s' % (path, f)
            ProcSingleFile(raw_name)

def ProcSingleFile(file_name):
    path,tmp_name = os.path.split(file_name)
    tmp_name = tmp_name.split("asr_")[1]

    save_name = path+'/'+tmp_name[:-2] + '.data'

    write_file = open(save_name, 'w')
    skip = 0
    delete_space = 0
    sde_setting = 0
    caf_setting = 0
    linelist = []
    infile = open(file_name, 'r')
    while 1:
        line = infile.readline()
        if not line:
            break
        else:
            linelist.append(line)

    infile.close()
    total_line = len(linelist)
    for i in range(total_line):
        if skip > 1 :
            skip -= 1
            continue
        start_index = linelist[i].find('//')
        if (start_index >= 0):
            line_str = linelist[i][start_index+2:]
            sde_setting, caf_setting = replace_symbol(line_str, write_file)
        elif i>=(total_line-2):
            write_file.write(linelist[i])
        else:
            skip,delete_space = normal_write_line(
                linelist[i],linelist[i+1],linelist[i+2], sde_setting, caf_setting, write_file,delete_space)


    write_file.close()
    print(file_name+' --> ', save_name+' done')


if __name__ == '__main__':
    argu_cnt = len(sys.argv)
    if argu_cnt != 2:
        print("dove_struct_to_settingfile.py [sensor_module_path]")
        sys.exit()


    sensor_module_path = str(sys.argv[1])

    ProcPath(sensor_module_path)

