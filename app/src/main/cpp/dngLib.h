//
// Created by DaisukeHohjoh on 2024/02/28.
//

#ifndef THETA_PLUGIN_CAMERA_API_SAMPLE_X_DNGLIB_H
#define THETA_PLUGIN_CAMERA_API_SAMPLE_X_DNGLIB_H

#pragma once

#include <iostream>
#include <vector>

enum DATATYPE {
    BYTE = 1,	//UInt8(符号無し8bit整数)
    ASCII,		//7bitのASCIIコード。最後必ず0x00(NULL)で終わる
    SHORT,		//Uint16(符号無し16bit整数)
    LONG,		//Uint32(符号無し32bit整数)
    RATIONAL,	//分数 Uint32の分子とUint32の分母
    SBYTE,		//SInt8(符号付き8bit)
    UNDEFINED,	//Tagが解釈する。countにはbyte数が入る
    SSHORT,		//SInt16(符号付き16bit整数)
    SLONG,		//SInt32(符号付き32bit整数)
    SRATIONAL,	//分数 SInt32の分子とUInt32の分母
    FLOAT,		//浮動小数点（IEEE 単精度浮動小数点)
    DOUBLE		//浮動小数点（IEEE 倍精度浮動小数点)
};

class dngEntry {
public:
    ~dngEntry() {
        del_edata();
    };
    char tag[2];
    char typ[2];
    char num[4];
    char dat[4];
    char * edata;
    int length;
    void new_edata(int s) {
        edata = new char[s];
    }
    void del_edata() {
        delete[] edata;
    }
};

int b2i(char* buffer, int max);
void i2b(int x, char *b);

class dngLib {

private:
    int dtype2bytenum(int id);

public:
    std::vector <dngEntry*> e_lists;
    void AddEntry(int tag, int typ, unsigned char * dat, int num);
    int searchEntry(int tag);
    void DeleteEntry(int tag);
    void readEntryEach(std::ifstream& ifs, int adr, dngEntry& e);
    int readEntryCnt(std::ifstream& ifs, int adr);
    void copyEntry(dngEntry * src);
};

#endif //THETA_PLUGIN_CAMERA_API_SAMPLE_X_DNGLIB_H
