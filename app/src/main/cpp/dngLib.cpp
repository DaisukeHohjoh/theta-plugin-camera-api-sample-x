//
// Created by DaisukeHohjoh on 2024/02/28.
//

#include "dngLib.h"
#include <string.h>
#include <string>
#include <jni.h>
#include <fstream>
#include <iostream>
#include <android/log.h>

const char* TAG = "Camera_API_Sample";

extern "C" JNIEXPORT int JNICALL
Java_com_theta360_sample_camera_MainActivity_rawToDng(
        JNIEnv* env, jobject thiz, jstring _src, jstring _dst
){
    const char* src = env->GetStringUTFChars(_src, 0);
    const char* dst = env->GetStringUTFChars(_dst, 0);
    __android_log_print(ANDROID_LOG_DEBUG, TAG, "src:%s dst:%s", src, dst);

    //TODO
    char filename[256];
    sprintf(filename, "%s", dst);
    std::ofstream ofs(filename, std::ios::binary);

    unsigned char header[8] = {
            0x4D, 0x4D,					//Byte Order     = "MM"
            0x00, 0x2A,					//IFD Identifer  = 0x00_2A
            0x03, 0x9c, 0x80, 0x08 };	//0th IFD Offset = (5504 * 5504 * 2) + 8 = 60,588,040‬‭ = 0x03_9C_80_08‬
    ofs.write(reinterpret_cast<const char *>(header), 8);

    //TODO raw_bytes
    std::ifstream ifs(src, std::ios::binary);
    if (!ifs) {
        __android_log_print(ANDROID_LOG_DEBUG, TAG, "cannot open file!!");
        return 1;
    }
    ifs.seekg(0, std::ios::end);
    std::streamsize size = ifs.tellg();
    ifs.seekg(0, std::ios::beg);

    char *raw_bytes;
    raw_bytes = new char[size];
    if (ifs.read(raw_bytes, size)) {
        __android_log_print(ANDROID_LOG_DEBUG, TAG, "succeed at read file, size = %lu", size);
    }
    else {
        __android_log_print(ANDROID_LOG_DEBUG, TAG, "error caused during open file!!");
    }
    ifs.close();
    ofs.write(raw_bytes, size); //rawdata
    delete raw_bytes;

    dngLib _dngLib;

    //0x00FE NewSubFileType
    unsigned char d_0x00fe[4] = { 0x00, 0x00, 0x00, 0x00 };
    _dngLib.AddEntry(0x00FE, LONG, d_0x00fe, 4);

    //0x0100 ImageWidth
    unsigned char d_0x0100[4] = { 0x00, 0x00, 0x15, 0x80 }; //5504
    _dngLib.AddEntry(0x0100, LONG, d_0x0100, 4);

    //0x0101 ImageHeight
    unsigned char d_0x0101[4] = { 0x00, 0x00, 0x15, 0x80 }; //5504
    _dngLib.AddEntry(0x0101, LONG, d_0x0101, 4);

    //0x0102 BitsPerSample
    unsigned char d_0x0102[2] = { 0x00, 0x10 }; //16bit
    _dngLib.AddEntry(0x0102, SHORT, d_0x0102, 2);

    //0x0103 Compression
    unsigned char d_0x0103[2] = { 0x00, 0x01 }; //1:Uncompressed
    _dngLib.AddEntry(0x0103, SHORT, d_0x0103, 2);

    //0x0106 PhotometricInterpretat
    unsigned char d_0x0106[2] = { 0x80, 0x23 }; //32803:CFA
    _dngLib.AddEntry(0x0106, SHORT, d_0x0106, 2);

    //0x010E ImageDescription
    //0x010F Make
    //0x0110 Model
    //0x0111 StripOffsets
    unsigned char d_0x0111[4] = { 0x00, 0x00, 0x00, 0x08 };
    _dngLib.AddEntry(0x0111, LONG, d_0x0111, 4);

    //0x0112 Orientation
    unsigned char d_0x0112[2] = { 0x00, 0x01 };
    _dngLib.AddEntry(0x0112, SHORT, d_0x0112, 2);

    //0x0115 SamplesPerPixels
    unsigned char d_0x0115[2] = { 0x00, 0x01 };
    _dngLib.AddEntry(0x0115, SHORT, d_0x0115, 2);

    //0x0116 RowsPerStrip
    unsigned char d_0x0116[4] = { 0x00, 0x00, 0x15, 0x80 };	//5504
    _dngLib.AddEntry(0x0116, LONG, d_0x0116, 4);

    //0x0117 StripByteCounts
    unsigned char d_0x0117[4] = { 0x03, 0x9C, 0x80, 0x00 };	//5504*5504*2
    _dngLib.AddEntry(0x0117, LONG, d_0x0117, 4);

    //0x011A Xresolution
    //0x011B Yresolution
    //0x011C PlanarConfiguration
    unsigned char d_0x011c[2] = { 0x00, 0x01 };
    _dngLib.AddEntry(0x011c, SHORT, d_0x011c, 2);

    //0x0128 ResolutionUnit
    unsigned char d_0x0128[2] = { 0x00, 0x02 };
    _dngLib.AddEntry(0x0128, SHORT, d_0x0128, 2);

    //0x0131 Software
    //0x0132 DateTime
    //0x0153 SampleFormat
    unsigned char d_0x0153[2] = { 0x00, 0x03 };	//float
    _dngLib.AddEntry(0x0153, SHORT, d_0x0153, 2);

    //0x0213 YCbCrPositioning
    //0x8298 Copyright
    //0x828D CFARepeatPatternDim
    unsigned char d_0x828D[4] = { 0x00, 0x02, 0x00, 0x02 };
    _dngLib.AddEntry(0x828D, SHORT, d_0x828D, 4);

    //0x828E CFAPattern
    unsigned char d_0x828E[4] = { 0x00, 0x01, 0x01, 0x02 };
    _dngLib.AddEntry(0x828E, BYTE, d_0x828E, 4);

    //0x8769 ExifIFDPointer
    //0x8825 GPSIFDPointer

    //0xC612 DNGVersion
    unsigned char d_0xc612[4] = { 0x01, 0x04, 0x00, 0x00 }; //1 4 0 0
    _dngLib.AddEntry(0xc612, BYTE, d_0xc612, 4);

    //0xC613 DNGBackwardVersion
    unsigned char d_0xc613[4] = { 0x01, 0x04, 0x00, 0x00 }; //1 4 0 0
    _dngLib.AddEntry(0xc613, BYTE, d_0xc613, 4);

    //0xC614 UniqueCameraModel
    //0xC617 CFALayout
    unsigned char d_0xc617[2] = { 0x00, 0x01 };
    _dngLib.AddEntry(0xc617, SHORT, d_0xc617, 2);

    //0xC619 BlackLevelRepeatDim
    unsigned char d_0xc619[4] = { 0x00, 0x02, 0x00, 0x02 };
    _dngLib.AddEntry(0xc619, SHORT, d_0xc619, 4);

    //0xC61A BlackLevel
    unsigned char d_0xc61a[16] = { 0x00, 0x00, 0x00, 0x00,
                                   0x00, 0x00, 0x00, 0x00,
                                   0x00, 0x00, 0x00, 0x00,
                                   0x00, 0x00, 0x00, 0x00 };
    _dngLib.AddEntry(0xc61a, LONG, d_0xc61a, 16);

    //0xC61D WhiteLevel
    unsigned char d_0xc61d[4] = { 0x00, 0x00, 0xFF, 0xFF };
    _dngLib.AddEntry(0xc61d, LONG, d_0xc61d, 4);

    //0xC61E DefaultScale
    unsigned char d_0xc61e[8 * 2] = {
            0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01,
            0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01};
    _dngLib.AddEntry(0xc61e, RATIONAL, d_0xc61e, 8 * 2);

    //0xC61F DefaultCropOrigin
    unsigned char d_0xc61f[4 * 2] = {
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
    _dngLib.AddEntry(0xc61f, LONG, d_0xc61f, 4 * 2);

    //0xC620 DefaultCropSize
    unsigned char d_0xc620[4 * 2] = {
            0x00, 0x00, 0x15, 0x80, 0x00, 0x00, 0x15, 0x80 };
    _dngLib.AddEntry(0xc620, LONG, d_0xc620, 4 * 2);

    //0xC621 ColorMatrix1
    unsigned char d_0xc621[8 * 9] = {
            0x00, 0x01, 0x0D, 0x2E, 0x00, 0x01, 0x00, 0x00,
            0xFF, 0xFF, 0x86, 0xCA, 0x00, 0x01, 0x00, 0x00,
            0xFF, 0xFF, 0xE2, 0x70, 0x00, 0x01, 0x00, 0x00,
            0xFF, 0xFF, 0xAE, 0x8A, 0x00, 0x01, 0x00, 0x00,
            0x00, 0x01, 0x19, 0x55, 0x00, 0x01, 0x00, 0x00,
            0x00, 0x00, 0x40, 0x81, 0x00, 0x01, 0x00, 0x00,
            0xFF, 0xFF, 0xFD, 0x6C, 0x00, 0x01, 0x00, 0x00,
            0x00, 0x00, 0x12, 0x0D, 0x00, 0x01, 0x00, 0x00,
            0x00, 0x00, 0x81, 0x21, 0x00, 0x01, 0x00, 0x00 };
    _dngLib.AddEntry(0xc621, SRATIONAL, d_0xc621, 8 * 9);

    //0xC622 ColorMatrix2
    unsigned char d_0xc622[8 * 9] = {
            0x00, 0x00, 0xD3, 0x97, 0x00, 0x01, 0x00, 0x00,
            0xFF, 0xFF, 0xC7, 0xDF, 0x00, 0x01, 0x00, 0x00,
            0xFF, 0xFF, 0xE1, 0xC6, 0x00, 0x01, 0x00, 0x00,
            0xFF, 0xFF, 0xB9, 0xBB, 0x00, 0x01, 0x00, 0x00,
            0x00, 0x01, 0x32, 0x73, 0x00, 0x01, 0x00, 0x00,
            0x00, 0x00, 0x14, 0xFC, 0x00, 0x01, 0x00, 0x00,
            0xFF, 0xFF, 0xF7, 0xC9, 0x00, 0x01, 0x00, 0x00,
            0x00, 0x00, 0x31, 0x8D, 0x00, 0x01, 0x00, 0x00,
            0x00, 0x00, 0x5F, 0x08, 0x00, 0x01, 0x00, 0x00 };
    _dngLib.AddEntry(0xc622, SRATIONAL, d_0xc622, 8 * 9);

    //0xC627 AnalogBalance
    unsigned char d_0xc627[8 * 3] = {
            0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x04, 0x00,
            0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x04, 0x00,
            0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x04, 0x00 };
    _dngLib.AddEntry(0xc627, RATIONAL, d_0xc627, 8 * 3);

    //0xC628 AsShotNeutral
    unsigned char d_0xc628[8 * 3] = {
            0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x08, 0x4B,
            0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x04, 0x00,
            0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x08, 0xAB };
    _dngLib.AddEntry(0xc628, RATIONAL, d_0xc628, 8 * 3);

    //0xC62A BaselineExposure
    unsigned char d_0xc62a[8] = { 0x00, 0x00, 0x93, 0x63, 0x00, 0x01, 0x00, 0x00 };
    _dngLib.AddEntry(0xc62a, SRATIONAL, d_0xc62a, 8);

    //0xC62B BaselineNoise
    //0xC62C BaselineSharpness
    //0xC62D BayerGreenSplit
    unsigned char d_0xc62d[4] = { 0x00, 0x00, 0x00, 0x00 };
    _dngLib.AddEntry(0xc62d, LONG, d_0xc62d, 4);

    //0xC62E LinearResponseLimit
    unsigned char d_0xc62e[8] = { 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01 };
    _dngLib.AddEntry(0xc62e, RATIONAL, d_0xc62e, 8);

    //0xC630 LensInfo
    //0xC632 AntiAliasStrength
    //0xC65A CalibrationIlluminant1
    unsigned char d_0xc65a[2] = { 0x00, 0x11 };	//17
    _dngLib.AddEntry(0xc65a, SHORT, d_0xc65a, 2);

    //0xC65B CalibrationIlluminant2
    unsigned char d_0xc65b[2] = { 0x00, 0x15 };	//21
    _dngLib.AddEntry(0xc65b, SHORT, d_0xc65b, 2);

    //0xC68D ActiveArea
    unsigned char d_0xc68d[4 * 4] = {
            0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x15, 0x80,
            0x00, 0x00, 0x15, 0x80 };
    _dngLib.AddEntry(0xc68d, LONG, d_0xc68d, 4*4);

    //0xC71A PreviewColorSpace

    //Write 0th IFD
    int wt = _dngLib.e_lists.size();
    int offset = (int)(ofs.tellp()) + (12 * wt) + 4 + 2;
    char wtotal[2] = { (char)(wt / 256), (char)(wt % 256) };
    ofs.write(wtotal, 2);

    for (int i = 0; i < wt; i++) {

        dngEntry *e = _dngLib.e_lists[i];
        int dl = e->length;
        int dn = b2i(e->num, 4);
        int bnum = dl * dn;

        int cadr = ofs.tellp();
        ofs.write(e->tag, 2);
        ofs.write(e->typ, 2);
        ofs.write(e->num, 4);

        //edataの長さとdlengthからoffsetに書くか、そのまま書くか
        if (bnum <= 4) {
            //std::cout << "dng tag= " << b2i(e->tag, sizeof(e->tag)) << " type= " << b2i(e->typ, sizeof(e->typ)) << " size= " << e->length << " num= " << b2i(e->num, sizeof(e->num)) << " data= " << b2i(e->dat, sizeof(e->dat)) << std::endl;
            ofs.write(e->edata, bnum);
            ofs.seekp(cadr + 12, std::ios_base::beg);

        }
        else {
            //std::cout << "dng tag= " << b2i(e->tag, sizeof(e->tag)) << " type= " << b2i(e->typ, sizeof(e->typ)) << " size= " << e->length << " num= " << b2i(e->num, sizeof(e->num)) << " data(offset)= " << offset << std::endl;
            char pos[4];
            i2b(offset, pos);
            ofs.write(pos, 4);
            offset += bnum;
            if (bnum % 2 == 1) {
                offset += 1;
            }
        }
    }
    //no IFD anymore
    char z[4] = { 0,0,0,0 };
    ofs.write(z, 4);

    //extra
    for (int i = 0; i < wt; i++) {
        dngEntry *e = _dngLib.e_lists[i];
        int dl = e->length;
        int dn = b2i(e->num, 4);
        int bnum = dl * dn;

        if (bnum <= 4) {
            ; //no extra
        }
        else {
            ofs.write(e->edata, bnum);
            char z[1] = { 0 };
            if (bnum % 2 != 0) ofs.write(z, 1);//fill
        }
    }

    ofs.close();

    return 0;
}

int b2i(char* buffer, int max) {
    int x = 0;
    int max_i = max - 1;
    for (int i = 0; i < max; i++) {
        x = 256 * x + (unsigned char)buffer[i];
    }
    return x;
}

void i2b(int x, char *b) {
    int xx = x;
    for (int i = 0; i < 4; i++) {
        b[3 - i] = xx & 0xff;
        xx = xx >> 8;
    }
}

int dngLib::dtype2bytenum(int id) {
    int dlength[] = { 0, 1, 1, 2, 4, 8, 1, 1, 2, 4, 8, 4, 8 };
    if (id < 13) {
        return dlength[id];
    }
    else {
        return -1;
    }
}

void dngLib::AddEntry(int _tag, int _typ, unsigned char * _dat, int _num)
{
    dngEntry *e = new dngEntry();

    //std::cout << "_tag:" << _tag << ",_typ:" << _typ << ",_dat:" << _dat << ",num:" << _num << std::endl;

    (e->tag)[1] = 0xff & _tag;
    (e->tag)[0] = 0xff & (_tag >> 8);

    (e->typ)[1] = 0xff & _typ;
    (e->typ)[0] = 0xff & (_typ >> 8);

    int _length = dtype2bytenum(b2i(e->typ, sizeof(e->typ)));
    e->length = _length;

    int __num = _num / e->length;
    (e->num)[3] = 0xff & __num;			//TODO
    (e->num)[2] = 0xff & (_num >> 8);
    (e->num)[1] = 0xff & (_num >> 16);
    (e->num)[0] = 0xff & (_num >> 24);

    e->new_edata(_num);
    for (int i = 0; i < _num; i++) {
        (e->edata)[i] = _dat[i];
    }

    //std::cout << int(e->tag[0]) << std::endl;
    //std::cout << int(e->tag[1]) << std::endl;
    //std::cout << e->typ << std::endl;
    //std::cout << e->num << std::endl;

    e_lists.push_back(e);
}
