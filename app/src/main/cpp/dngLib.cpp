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
        JNIEnv* env, jobject thiz, jstring _src, jstring _dst, jstring _jpeg
){
    const char* src = env->GetStringUTFChars(_src, 0);
    const char* dst = env->GetStringUTFChars(_dst, 0);
    const char* jpegpath = env->GetStringUTFChars(_jpeg, 0);
    __android_log_print(ANDROID_LOG_DEBUG, TAG, "src:%s dst:%s _jpeg:%s", src, dst, jpegpath);

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

    // ↑ここまででまずsrcからdstへ丸っと情報をコピー
    // コピーしたらsrcは不要？なので破棄



    //-----------------------------------------
    // JPEGからの情報抜き出し
    std::ifstream jpgifs(jpegpath, std::ios::binary);
    jpgifs.seekg(0, std::ios::end);
    std::streamsize jpgsize = jpgifs.tellg();
    jpgifs.seekg(0, std::ios_base::beg);

    char *jpeg_bytes;
    jpeg_bytes = new char[jpgsize];
    if (jpgifs.read(jpeg_bytes, jpgsize)) {
        __android_log_print(ANDROID_LOG_DEBUG, TAG, "[HDR-DNG-META] succeed at read jpeg file, size = %lu", jpgsize);

    }
    else {
        __android_log_print(ANDROID_LOG_DEBUG, TAG, "[HDR-DNG-META] error caused during open jpeg file!!");
    }

    //JPEGのヘッダ情報 debug用
    __android_log_print(ANDROID_LOG_DEBUG, TAG, "[HDR-DNG-META] bite = %x %x %x %x %x %x", jpeg_bytes[0], jpeg_bytes[1], jpeg_bytes[2], jpeg_bytes[3], jpeg_bytes[4], jpeg_bytes[5]);
    __android_log_print(ANDROID_LOG_DEBUG, TAG, "[HDR-DNG-META] bite = %x %x %x %x %x %x", jpeg_bytes[6], jpeg_bytes[7], jpeg_bytes[8], jpeg_bytes[9], jpeg_bytes[10], jpeg_bytes[11]);
    __android_log_print(ANDROID_LOG_DEBUG, TAG, "[HDR-DNG-META] bite = %x %x %x %x", jpeg_bytes[12], jpeg_bytes[13], jpeg_bytes[14], jpeg_bytes[15]);
    __android_log_print(ANDROID_LOG_DEBUG, TAG, "[HDR-DNG-META] bite = %x %x %x %x", jpeg_bytes[16], jpeg_bytes[17], jpeg_bytes[18], jpeg_bytes[19]);

    __android_log_print(ANDROID_LOG_DEBUG, TAG, "[HDR-DNG-META] タグ数 = %x%x = %d タグ = %x%x", jpeg_bytes[20], jpeg_bytes[21],(jpeg_bytes[20] << 2) + jpeg_bytes[21], jpeg_bytes[22],jpeg_bytes[23]);

    //-----------------------------------------


    //--------------------------------------
    //IFD address
    //readIFDstart(ifs);      //スタート位置の取得。

    int ADR_IFDSTART = 12;  //ouffset
    int ADR_OFFSET = 4;

    bool enableexif = false;
    bool enablegpg = false;
    bool enablemaker = false;
    bool enablereport = false;

    char ver[4];
    jpgifs.seekg(ADR_IFDSTART+ADR_OFFSET, std::ios_base::beg);  //offset:ADR_IFDSTART = 4　JPEGは20番目からスタート
    jpgifs.read(ver, 4);
    int adr_ifd_s = b2i(ver, sizeof(ver)) + ADR_IFDSTART;
    __android_log_print(ANDROID_LOG_DEBUG, TAG, "[HDR-DNG-META] adr_ifd_start = %d", adr_ifd_s);


    dngLib _readdngLib;
    dngLib _readexitdngLib;
    dngLib _readgpsdngLib;
    dngLib _readmakerdngLib;
    dngLib _readrecepdngLib;


    _readdngLib.e_lists.clear();
    //number of tag count
    int ent_cnt = _readdngLib.readEntryCnt(jpgifs, adr_ifd_s);

    __android_log_print(ANDROID_LOG_DEBUG, TAG, "[HDR-DNG-META] EntryCount = %d", ent_cnt);

    //read entries
    int adr_ent = adr_ifd_s + 2;
    for (int i = 0; i < ent_cnt; i++) {
        dngEntry * e = new dngEntry();
        //__android_log_print(ANDROID_LOG_DEBUG, TAG, "[HDR-DNG-META] adr_ent = %d", adr_ent);

        _readdngLib.readEntryEach(jpgifs, adr_ent, *e);
        __android_log_print(ANDROID_LOG_DEBUG, TAG, "[HDR-DNG-META] tag=%d type=%d size=%d num=%d data=%d", b2i(e->tag, sizeof(e->tag)), b2i(e->typ, sizeof(e->typ)), e->length, b2i(e->num, sizeof(e->num)), b2i(e->dat, sizeof(e->dat)));

        _readdngLib.e_lists.push_back(e);
        adr_ent += 12; //next
    }

    // ----- exif ifd read ----------------------------

    //----------------
    // read exif ifd
    //----------------
    int searchExif = _readdngLib.searchEntry(0x8769);
    if(searchExif == -1){
        __android_log_print(ANDROID_LOG_DEBUG, TAG, "[HDR-DNG-META] no search for exif tag in jpeg data");
    }else{
        enableexif = true;
        dngEntry *e_exif = _readdngLib.e_lists[searchExif];
        long eofs = b2i(e_exif->edata, 4);
        __android_log_print(ANDROID_LOG_DEBUG, TAG, "[HDR-DNG-META] EXIF point = %d", eofs);


        //tag count
        adr_ifd_s = eofs + ADR_IFDSTART;   //ADR_IFDSTART
        ent_cnt = _readdngLib.readEntryCnt(jpgifs, adr_ifd_s);

        __android_log_print(ANDROID_LOG_DEBUG, TAG, "[HDR-DNG-META] EXIF EntryCount = %d", ent_cnt);

        //search entries in IFD
        adr_ent = adr_ifd_s + 2;

        for (int i = 0; i < ent_cnt; i++) {
            dngEntry * e = new dngEntry();
            _readexitdngLib.readEntryEach(jpgifs, adr_ent, *e);

            //__android_log_print(ANDROID_LOG_DEBUG, TAG, "[HDR-DNG-META] exifdngLib tag=%d type=%d size=%d num=%d data=%d ", b2i(e->tag, sizeof(e->tag)), b2i(e->typ, sizeof(e->typ)), e->length, b2i(e->num, sizeof(e->num)), b2i(e->dat, sizeof(e->dat)));
            _readexitdngLib.e_lists.push_back(e);

            /*if (b2i(e->tag, sizeof(e->tag)) == 0x927c) {      //store for RICOH makernote
                //m_maker_adr = b2i(e->data, sizeof(e->data));
                __android_log_print(ANDROID_LOG_DEBUG, TAG, "[HDR-DNG-META] maker_adr = %d i = %d", b2i(e->dat, sizeof(e->dat)), i);
            }*/
            adr_ent += 12; //next
        }
        __android_log_print(ANDROID_LOG_DEBUG, TAG, "[HDR-DNG-META] Exif End");
    }

    //----------------
    // read gps ifd
    //----------------
    searchExif = _readdngLib.searchEntry(0x8825);
    if(searchExif == -1) {
        __android_log_print(ANDROID_LOG_DEBUG, TAG, "[HDR-DNG-META] no search for gps tag in jpeg data");
    }else{
        enablegpg = true;
        dngEntry *e_exif = _readdngLib.e_lists[searchExif];
        long eofs = b2i(e_exif->edata, 4);

        //tag count
        adr_ifd_s = eofs + ADR_IFDSTART;
        ent_cnt = _readgpsdngLib.readEntryCnt(jpgifs, adr_ifd_s);

        //std::cout << "GPS EntryCount = " << int2str(ent_cnt) << std::endl;
        __android_log_print(ANDROID_LOG_DEBUG, TAG, "[HDR-DNG-META] GPS EntryCount = %d", ent_cnt);


        //search entries in IFD
        adr_ent = adr_ifd_s + 2;

        for (int i = 0; i < ent_cnt; i++) {
            dngEntry * e = new dngEntry();
            _readgpsdngLib.readEntryEach(jpgifs, adr_ent, *e);
            //__android_log_print(ANDROID_LOG_DEBUG, TAG, "[HDR-DNG-META] tag=%d type=%d size=%d num=%d data=%d", b2i(e->tag, sizeof(e->tag)), b2i(e->typ, sizeof(e->typ)), e->length, b2i(e->num, sizeof(e->num)), b2i(e->dat, sizeof(e->dat)));
            _readgpsdngLib.e_lists.push_back(e);
            adr_ent += 12; //next
        }

        __android_log_print(ANDROID_LOG_DEBUG, TAG, "[HDR-DNG-META] GPS End");
    }


    //----------------
    // read makernote
    //----------------
    searchExif = enableexif == true ? _readexitdngLib.searchEntry(0x927c) : -1;

    if(searchExif == -1) {
        __android_log_print(ANDROID_LOG_DEBUG, TAG, "[HDR-DNG-META] no search for makernote tag in jpeg data");
    }else{
        enablemaker = true;
        __android_log_print(ANDROID_LOG_DEBUG, TAG, "[HDR-DNG-META] makernote searchExif = %d", searchExif);
        dngEntry *e_exif = _readexitdngLib.e_lists[searchExif];
        long eofs = b2i(e_exif->dat, 4);
        __android_log_print(ANDROID_LOG_DEBUG, TAG, "[HDR-DNG-META] makernote point = %d", eofs);

        //tag count
        // MakerNote先頭８byteは"Ricoh000"
        adr_ifd_s = eofs + ADR_IFDSTART + 8;
        ent_cnt = _readmakerdngLib.readEntryCnt(jpgifs, adr_ifd_s);
        __android_log_print(ANDROID_LOG_DEBUG, TAG, "[HDR-DNG-META] MakerNote header bite = %x %x %x %x %x %x %x %x", jpeg_bytes[eofs + ADR_IFDSTART], jpeg_bytes[eofs + ADR_IFDSTART + 1], jpeg_bytes[eofs + ADR_IFDSTART + 2], jpeg_bytes[eofs + ADR_IFDSTART + 3], jpeg_bytes[eofs + ADR_IFDSTART + 4], jpeg_bytes[eofs + ADR_IFDSTART + 5], jpeg_bytes[eofs + ADR_IFDSTART + 6], jpeg_bytes[eofs + ADR_IFDSTART + 7]);
        __android_log_print(ANDROID_LOG_DEBUG, TAG, "[HDR-DNG-META] MakerNote entries bite = %x %x", jpeg_bytes[eofs + ADR_IFDSTART + 8], jpeg_bytes[eofs + ADR_IFDSTART + 9]);

        __android_log_print(ANDROID_LOG_DEBUG, TAG, "[HDR-DNG-META] MakerNote EntryCount = %d", ent_cnt);

        //search entries in IFD
        adr_ent = adr_ifd_s + 2;

        for (int i = 0; i < ent_cnt; i++) {
            dngEntry * e = new dngEntry();
            _readmakerdngLib.readEntryEach(jpgifs, adr_ent, *e);
            __android_log_print(ANDROID_LOG_DEBUG, TAG, "[HDR-DNG-META] tag=%d type=%d size=%d num=%d data=%d", b2i(e->tag, sizeof(e->tag)), b2i(e->typ, sizeof(e->typ)), e->length, b2i(e->num, sizeof(e->num)), b2i(e->dat, sizeof(e->dat)));
            _readmakerdngLib.e_lists.push_back(e);

            if (b2i(e->tag, sizeof(e->tag)) == 0x4001) { //store for ReceptorIFD
                __android_log_print(ANDROID_LOG_DEBUG, TAG, "[HDR-DNG-META] ReceptorIFD = %d i = %d", b2i(e->dat, sizeof(e->dat)), i);
            }
            __android_log_print(ANDROID_LOG_DEBUG, TAG, "[HDR-DNG-META] MakerNote entries bite = %x %x", jpeg_bytes[adr_ent], jpeg_bytes[adr_ent + 1]);
            __android_log_print(ANDROID_LOG_DEBUG, TAG, "[HDR-DNG-META] MakerNote entries bite = %x %x", jpeg_bytes[adr_ent + 2], jpeg_bytes[adr_ent + 3]);
            __android_log_print(ANDROID_LOG_DEBUG, TAG, "[HDR-DNG-META] MakerNote entries bite = %x %x %x %x", jpeg_bytes[adr_ent + 4], jpeg_bytes[adr_ent + 5], jpeg_bytes[adr_ent+ 6], jpeg_bytes[adr_ent + 7]);
            __android_log_print(ANDROID_LOG_DEBUG, TAG, "[HDR-DNG-META] MakerNote entries bite = %x %x %x %x", jpeg_bytes[adr_ent + 8], jpeg_bytes[adr_ent + 9], jpeg_bytes[adr_ent+ 10], jpeg_bytes[adr_ent + 11]);
            adr_ent += 12; //next
        }
        __android_log_print(ANDROID_LOG_DEBUG, TAG, "[HDR-DNG-META] MakerNote End");
    }
    __android_log_print(ANDROID_LOG_DEBUG, TAG, "[HDR-DNG-META] Receptor search start");


    //----------------
    // read Receptor
    //----------------
    searchExif = enablemaker == true ? _readmakerdngLib.searchEntry(0x4001) : -1;

    if(searchExif == -1) {
        __android_log_print(ANDROID_LOG_DEBUG, TAG, "[HDR-DNG-META] no search for Receptor tag in jpeg data");
    }else{
        enablereport = true;
        __android_log_print(ANDROID_LOG_DEBUG, TAG, "[HDR-DNG-META] Receptor searchExif = %d", searchExif);
        dngEntry *e_exif = _readmakerdngLib.e_lists[searchExif];
        long eofs = b2i(e_exif->dat, 4);
        __android_log_print(ANDROID_LOG_DEBUG, TAG, "[HDR-DNG-META] Receptor point = %d", eofs);

        //tag count
        adr_ifd_s = eofs + ADR_IFDSTART;
        ent_cnt = _readrecepdngLib.readEntryCnt(jpgifs, adr_ifd_s);

        __android_log_print(ANDROID_LOG_DEBUG, TAG, "[HDR-DNG-META] Receptor EntryCount = %d", ent_cnt);

        //search entries in IFD
        adr_ent = adr_ifd_s + 2;

        for (int i = 0; i < ent_cnt; i++) {
            dngEntry * e = new dngEntry();
            _readrecepdngLib.readEntryEach(jpgifs, adr_ent, *e);
            __android_log_print(ANDROID_LOG_DEBUG, TAG, "[HDR-DNG-META] tag=%d type=%d size=%d num=%d data=%d", b2i(e->tag, sizeof(e->tag)), b2i(e->typ, sizeof(e->typ)), e->length, b2i(e->num, sizeof(e->num)), b2i(e->dat, sizeof(e->dat)));
            _readrecepdngLib.e_lists.push_back(e);
            adr_ent += 12; //next
        }

        __android_log_print(ANDROID_LOG_DEBUG, TAG, "[HDR-DNG-META] Receptor End");
    }

    //--------------------------------------


    //write
    dngLib _dngLib;
    dngLib _exifdngLib;
    dngLib _makerdngLib;
    dngLib _recepdngLib;
    dngLib _gpsdngLib;

    int searchTag;

    jpgifs.close();
    delete jpeg_bytes;

    // ↓ここからメタデータを付与
    //dngLib _dngLib;

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
    unsigned char d_0x0102[6] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x10 }; //16  cnt=3
    _dngLib.AddEntry(0x0102, SHORT, d_0x0102, 6);

    //0x0103 Compression
    unsigned char d_0x0103[2] = { 0x00, 0x01 }; //1:Uncompressed
    _dngLib.AddEntry(0x0103, SHORT, d_0x0103, 2);

    //0x0106 PhotometricInterpretat
    unsigned char d_0x0106[2] = { 0x80, 0x23 }; //32803:CFA
    _dngLib.AddEntry(0x0106, SHORT, d_0x0106, 2);

    //0x010E ImageDescription
    unsigned char d_0x010e[65] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                   0x00, 0x00, 0x00, 0x20, 0x00 }; //0x20 (64Byte) + 0x00
    _dngLib.AddEntry(0x010E, ASCII, d_0x010e, 65);

    //0x010F Make
    unsigned char d_0x010f[6] = { 0x52, 0x49, 0x43, 0x4F, 0x48, 0x00 };  //"RICOH"
    _dngLib.AddEntry(0x010F, ASCII, d_0x010f, 6);

    //0x0110 Model
    unsigned char d_0x0110[14] = { 0x52, 0x49, 0x43, 0x4F, 0x48, 0x20, 0x54, 0x48, 0x45, 0x54, 0x41, 0x20, 0x58, 0x00 };  //"RICOH THETA X"
    _dngLib.AddEntry(0x0110, ASCII, d_0x0110, 14);

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
    unsigned char d_0x011a[8] = { 0x00, 0x00, 0x01, 0x2C, 0x00, 0x00, 0x00, 0x01 };	//300/1
    _dngLib.AddEntry(0x011A, RATIONAL, d_0x011a, 8);

    //0x011B Yresolution
    unsigned char d_0x011b[8] = { 0x00, 0x00, 0x01, 0x2C, 0x00, 0x00, 0x00, 0x01 };	//300/1
    _dngLib.AddEntry(0x011B, RATIONAL, d_0x011b, 8);

    //0x011C PlanarConfiguration
    unsigned char d_0x011c[2] = { 0x00, 0x01 };
    _dngLib.AddEntry(0x011c, SHORT, d_0x011c, 2);

    //0x0128 ResolutionUnit
    unsigned char d_0x0128[2] = { 0x00, 0x02 };
    _dngLib.AddEntry(0x0128, SHORT, d_0x0128, 2);

    //0x0131 Software
    searchTag = _readdngLib.searchEntry(0x0131);
    if(searchTag != -1) {
        _dngLib.copyEntry(_readdngLib.e_lists[searchTag]);
    }

    //0x0132 DateTime
    searchTag = _readdngLib.searchEntry(0x0132);
    if(searchTag != -1) {
        _dngLib.copyEntry(_readdngLib.e_lists[searchTag]);
    }

    //0x0153 SampleFormat
    unsigned char d_0x0153[2] = { 0x00, 0x03 };	//float
    _dngLib.AddEntry(0x0153, SHORT, d_0x0153, 2);

    //0x0213 YCbCrPositioning
    //0x8298 Copyright
    unsigned char d_0x8298[25] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                   0x00, 0x00, 0x00, 0x00, 0x00 }; //0x00
    _dngLib.AddEntry(0x8298, ASCII, d_0x8298, 25);

    //0x828D CFARepeatPatternDim
    unsigned char d_0x828D[4] = { 0x00, 0x02, 0x00, 0x02 };
    _dngLib.AddEntry(0x828D, SHORT, d_0x828D, 4);

    //0x828E CFAPattern
    unsigned char d_0x828E[4] = { 0x00, 0x01, 0x01, 0x02 };
    _dngLib.AddEntry(0x828E, BYTE, d_0x828E, 4);

    //0x8769 ExifIFDPointer  //正規の値は最後にセット
    unsigned char d_0x8769[4] = { 0x00, 0x00, 0x00, 0x00 };
    _dngLib.AddEntry(0x8769, LONG, d_0x8769, 4);

    //0x8825 GPSIFDPointer  //正規の値は最後にセット
    unsigned char d_0x8825[4] = { 0x00, 0x00, 0x00, 0x00 };
    _dngLib.AddEntry(0x8825, LONG, d_0x8825, 4);

    //0xC612 DNGVersion
    unsigned char d_0xc612[4] = { 0x01, 0x04, 0x00, 0x00 }; //1 4 0 0
    _dngLib.AddEntry(0xc612, BYTE, d_0xc612, 4);

    //0xC613 DNGBackwardVersion
    unsigned char d_0xc613[4] = { 0x01, 0x04, 0x00, 0x00 }; //1 4 0 0
    _dngLib.AddEntry(0xc613, BYTE, d_0xc613, 4);

    //0xC614 UniqueCameraModel
    unsigned char d_0xc614[14] = { 0x52, 0x49, 0x43, 0x4F, 0x48, 0x20, 0x54, 0x48, 0x45, 0x54, 0x41, 0x20, 0x58, 0x00 };  //"RICOH THETA X"
    _dngLib.AddEntry(0xC614, ASCII, d_0xc614, 14);

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
    unsigned char d_0xc62b[8] = { 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01 };
    _dngLib.AddEntry(0xC62B, RATIONAL, d_0xc62b, 8);

    //0xC62C BaselineSharpness
    unsigned char d_0xc62c[8] = { 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01 };
    _dngLib.AddEntry(0xC62C, RATIONAL, d_0xc62c, 8);

    //0xC62D BayerGreenSplit
    unsigned char d_0xc62d[4] = { 0x00, 0x00, 0x00, 0x00 };
    _dngLib.AddEntry(0xc62d, LONG, d_0xc62d, 4);

    //0xC62E LinearResponseLimit
    unsigned char d_0xc62e[8] = { 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01 };
    _dngLib.AddEntry(0xc62e, RATIONAL, d_0xc62e, 8);

    //0xC630 LensInfo
    unsigned char d_0xc630[8 * 4] = {
            0x00, 0x00, 0x00, 0xD2, 0x00, 0x00, 0x00, 0x64,      // 210/100
            0x00, 0x00, 0x00, 0xD2, 0x00, 0x00, 0x00, 0x64,      // 210/100
            0x00, 0x00, 0x00, 0x15, 0x00, 0x00, 0x00, 0x0A,      // 21/10
            0x00, 0x00, 0x00, 0x15, 0x00, 0x00, 0x00, 0x0A };    // 21/10
    _dngLib.AddEntry(0xc630, RATIONAL, d_0xc630, 8 * 4);

    //0xC632 AntiAliasStrength
    unsigned char d_0xc632[8] = { 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01 };
    _dngLib.AddEntry(0xc632, RATIONAL, d_0xc632, 8);

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
    unsigned char d_0xc71a[4] = { 0x00, 0x00, 0x00, 0x02 };
    _dngLib.AddEntry(0xc71a, LONG, d_0xc71a, 4);


    // exif ifd
    //dngLib _exifdngLib;

    //0x829A ExposureTime
    searchTag = (enableexif == true) ? _readexitdngLib.searchEntry(0x829A) : -1;
    if(searchTag != -1) {
        _exifdngLib.copyEntry(_readexitdngLib.e_lists[searchTag]);
    }

    //0x829D Fnumber
    searchTag = (enableexif == true) ? _readexitdngLib.searchEntry(0x829D) : -1;
    if(searchTag != -1) {
        _exifdngLib.copyEntry(_readexitdngLib.e_lists[searchTag]);
    }

    //0x8822 ExposureProgram
    searchTag = (enableexif == true) ? _readexitdngLib.searchEntry(0x8822) : -1;
    if(searchTag != -1) {
        _exifdngLib.copyEntry(_readexitdngLib.e_lists[searchTag]);
    }

    //0x8827 PhotographicSensitivity
    searchTag = (enableexif == true) ? _readexitdngLib.searchEntry(0x8827) : -1;
    if(searchTag != -1) {
        _exifdngLib.copyEntry(_readexitdngLib.e_lists[searchTag]);
    }

    //0x8830 SensitivityType
    searchTag = (enableexif == true) ? _readexitdngLib.searchEntry(0x8830) : -1;
    if(searchTag != -1) {
        _exifdngLib.copyEntry(_readexitdngLib.e_lists[searchTag]);
    }

    //0x9000 ExifVersion
    searchTag = (enableexif == true) ? _readexitdngLib.searchEntry(0x9000) : -1;
    if(searchTag != -1) {
        _exifdngLib.copyEntry(_readexitdngLib.e_lists[searchTag]);
    }

    //0x9010 OffsetTime
    searchTag = (enableexif == true) ? _readexitdngLib.searchEntry(0x9010) : -1;
    if(searchTag != -1) {
        _exifdngLib.copyEntry(_readexitdngLib.e_lists[searchTag]);
    }

    //0x9011 OffsetTimeOriginal
    searchTag = (enableexif == true) ? _readexitdngLib.searchEntry(0x9011) : -1;
    if(searchTag != -1) {
        _exifdngLib.copyEntry(_readexitdngLib.e_lists[searchTag]);
    }

    //0x9012 OffsetTimeDigitized
    searchTag = (enableexif == true) ? _readexitdngLib.searchEntry(0x9012) : -1;
    if(searchTag != -1) {
        _exifdngLib.copyEntry(_readexitdngLib.e_lists[searchTag]);
    }

    //0x9003 DateTimeOriginal
    searchTag = (enableexif == true) ? _readexitdngLib.searchEntry(0x9003) : -1;
    if(searchTag != -1) {
        _exifdngLib.copyEntry(_readexitdngLib.e_lists[searchTag]);
    }

    //0x9004 DateTimeDigitized
    searchTag = (enableexif == true) ? _readexitdngLib.searchEntry(0x9004) : -1;
    if(searchTag != -1) {
        _exifdngLib.copyEntry(_readexitdngLib.e_lists[searchTag]);
    }

    //0x9101 ComponentConfiguration
    unsigned char d_0x9101[4] = { 0x01, 0x02, 0x03, 0x00 };
    _exifdngLib.AddEntry(0x9101, UNDEFINED, d_0x9101, 4);

    //0x9204 ExposureBiasValue
    searchTag = (enableexif == true) ? _readexitdngLib.searchEntry(0x9204) : -1;
    if(searchTag != -1) {
        _exifdngLib.copyEntry(_readexitdngLib.e_lists[searchTag]);
    }

    //0x9205 MaxApertureRatioValue
    searchTag = (enableexif == true) ? _readexitdngLib.searchEntry(0x9205) : -1;
    if(searchTag != -1) {
        _exifdngLib.copyEntry(_readexitdngLib.e_lists[searchTag]);
    }

    //0x9207 MeteringMode
    searchTag = (enableexif == true) ? _readexitdngLib.searchEntry(0x9207) : -1;
    if(searchTag != -1) {
        _exifdngLib.copyEntry(_readexitdngLib.e_lists[searchTag]);
    }

    //0x9208 LightSource
    searchTag = (enableexif == true) ? _readexitdngLib.searchEntry(0x9208) : -1;
    if(searchTag != -1) {
        _exifdngLib.copyEntry(_readexitdngLib.e_lists[searchTag]);
    }

    //0x9209 Flash
    searchTag = (enableexif == true) ? _readexitdngLib.searchEntry(0x9209) : -1;
    if(searchTag != -1) {
        _exifdngLib.copyEntry(_readexitdngLib.e_lists[searchTag]);
    }

    //0x920A FocalLength
    searchTag = (enableexif == true) ? _readexitdngLib.searchEntry(0x920A) : -1;
    if(searchTag != -1) {
        _exifdngLib.copyEntry(_readexitdngLib.e_lists[searchTag]);
    }

    //0x927C MakerNote     // 正規の値は後でセットする
    unsigned char d_0x927c[4] = { 0x00, 0x00, 0x00, 0x00 };
    _exifdngLib.AddEntry(0x927c, UNDEFINED, d_0x927c, 4);

    //0xA300 FileSource
    searchTag = (enableexif == true) ? _readexitdngLib.searchEntry(0xA300) : -1;
    if(searchTag != -1) {
        _exifdngLib.copyEntry(_readexitdngLib.e_lists[searchTag]);
    }

    //0xA401 CustomRendered
    searchTag = (enableexif == true) ? _readexitdngLib.searchEntry(0xA401) : -1;
    if(searchTag != -1) {
        _exifdngLib.copyEntry(_readexitdngLib.e_lists[searchTag]);
    }

    //0xA402 ExposureMode
    searchTag = (enableexif == true) ? _readexitdngLib.searchEntry(0xA402) : -1;
    if(searchTag != -1) {
        _exifdngLib.copyEntry(_readexitdngLib.e_lists[searchTag]);
    }

    //0xA403 WhiteBalance
    searchTag = (enableexif == true) ? _readexitdngLib.searchEntry(0xA403) : -1;
    if(searchTag != -1) {
        _exifdngLib.copyEntry(_readexitdngLib.e_lists[searchTag]);
    }

    //0xA406 SceneCaptureType
    searchTag = (enableexif == true) ? _readexitdngLib.searchEntry(0xA406) : -1;
    if(searchTag != -1) {
        _exifdngLib.copyEntry(_readexitdngLib.e_lists[searchTag]);
    }

    //0xA40A Sharpness
    searchTag = (enableexif == true) ? _readexitdngLib.searchEntry(0xA40A) : -1;
    if(searchTag != -1) {
        _exifdngLib.copyEntry(_readexitdngLib.e_lists[searchTag]);
    }


    // GPS ifd
    //dngLib _gpsdngLib;

    //0x0000 GPS Version
    searchTag = (enablegpg == true) ? _readgpsdngLib.searchEntry(0x0000) : -1;
    if(searchTag != -1) {
        _gpsdngLib.copyEntry(_readgpsdngLib.e_lists[searchTag]);
    }

    //0x0001 GPSLatitudeRef
    searchTag = (enablegpg == true) ? _readgpsdngLib.searchEntry(0x0001) : -1;
    if(searchTag != -1) {
        _gpsdngLib.copyEntry(_readgpsdngLib.e_lists[searchTag]);
    }

    //0x0002 GPSLatitude
    searchTag = (enablegpg == true) ? _readgpsdngLib.searchEntry(0x0002) : -1;
    if(searchTag != -1) {
        _gpsdngLib.copyEntry(_readgpsdngLib.e_lists[searchTag]);
    }

    //0x0003 GPSLongitudeRef
    searchTag = (enablegpg == true) ? _readgpsdngLib.searchEntry(0x0003) : -1;
    if(searchTag != -1) {
        _gpsdngLib.copyEntry(_readgpsdngLib.e_lists[searchTag]);
    }

    //0x0004 GPSLongitude
    searchTag = (enablegpg == true) ? _readgpsdngLib.searchEntry(0x0004) : -1;
    if(searchTag != -1) {
        _gpsdngLib.copyEntry(_readgpsdngLib.e_lists[searchTag]);
    }

    //0x0005 GPSAltitudeRef
    searchTag = (enablegpg == true) ? _readgpsdngLib.searchEntry(0x0005) : -1;
    if(searchTag != -1) {
        _gpsdngLib.copyEntry(_readgpsdngLib.e_lists[searchTag]);
    }

    //0x0006 GPSAltitude
    searchTag = (enablegpg == true) ? _readgpsdngLib.searchEntry(0x0006) : -1;
    if(searchTag != -1) {
        _gpsdngLib.copyEntry(_readgpsdngLib.e_lists[searchTag]);
    }

    //0x0007 GPSTimeStamp
    searchTag = (enablegpg == true) ? _readgpsdngLib.searchEntry(0x0007) : -1;
    if(searchTag != -1) {
        _gpsdngLib.copyEntry(_readgpsdngLib.e_lists[searchTag]);
    }

    //0x0008 GPSSatellites
    searchTag = (enablegpg == true) ? _readgpsdngLib.searchEntry(0x0008) : -1;
    if(searchTag != -1) {
        _gpsdngLib.copyEntry(_readgpsdngLib.e_lists[searchTag]);
    }

    //0x0009 GPSStatus
    searchTag = (enablegpg == true) ? _readgpsdngLib.searchEntry(0x0009) : -1;
    if(searchTag != -1) {
        _gpsdngLib.copyEntry(_readgpsdngLib.e_lists[searchTag]);
    }

    //0x000A GPSMeasureMode
    searchTag = (enablegpg == true) ? _readgpsdngLib.searchEntry(0x000A) : -1;
    if(searchTag != -1) {
        _gpsdngLib.copyEntry(_readgpsdngLib.e_lists[searchTag]);
    }

    //0x000B GPSDOP
    searchTag = (enablegpg == true) ? _readgpsdngLib.searchEntry(0x000B) : -1;
    if(searchTag != -1) {
        _gpsdngLib.copyEntry(_readgpsdngLib.e_lists[searchTag]);
    }

    //0x000C GPSSpeedRef
    searchTag = (enablegpg == true) ? _readgpsdngLib.searchEntry(0x000C) : -1;
    if(searchTag != -1) {
        _gpsdngLib.copyEntry(_readgpsdngLib.e_lists[searchTag]);
    }

    //0x000D GPSSpeed
    searchTag = (enablegpg == true) ? _readgpsdngLib.searchEntry(0x000D) : -1;
    if(searchTag != -1) {
        _gpsdngLib.copyEntry(_readgpsdngLib.e_lists[searchTag]);
    }

    //0x000E GPSTrackRef
    searchTag = (enablegpg == true) ? _readgpsdngLib.searchEntry(0x000E) : -1;
    if(searchTag != -1) {
        _gpsdngLib.copyEntry(_readgpsdngLib.e_lists[searchTag]);
    }

    //0x000F GPSTrack
    searchTag = (enablegpg == true) ? _readgpsdngLib.searchEntry(0x000F) : -1;
    if(searchTag != -1) {
        _gpsdngLib.copyEntry(_readgpsdngLib.e_lists[searchTag]);
    }

    //0x0010 GPSImgDirectionRef
    searchTag = (enablegpg == true) ? _readgpsdngLib.searchEntry(0x0010) : -1;
    if(searchTag != -1) {
        _gpsdngLib.copyEntry(_readgpsdngLib.e_lists[searchTag]);
    }

    //0x0011 GPSImgDirection
    searchTag = (enablegpg == true) ? _readgpsdngLib.searchEntry(0x0011) : -1;
    if(searchTag != -1) {
        _gpsdngLib.copyEntry(_readgpsdngLib.e_lists[searchTag]);
    }

    //0x0012 GPSMapDatum
    searchTag = (enablegpg == true) ? _readgpsdngLib.searchEntry(0x0012) : -1;
    if(searchTag != -1) {
        _gpsdngLib.copyEntry(_readgpsdngLib.e_lists[searchTag]);
    }

    //0x001B GPSProcessingMethod
    searchTag = (enablegpg == true) ? _readgpsdngLib.searchEntry(0x001B) : -1;
    if(searchTag != -1) {
        _gpsdngLib.copyEntry(_readgpsdngLib.e_lists[searchTag]);
    }

    //0x001D GPSDateStamp
    searchTag = (enablegpg == true) ? _readgpsdngLib.searchEntry(0x001D) : -1;
    if(searchTag != -1) {
        _gpsdngLib.copyEntry(_readgpsdngLib.e_lists[searchTag]);
    }



    // MakerNote ifd -------------
    //dngLib _mndngLib;

    //IDないヘッダー値は後ほど
    //MakerNoteHeader
    //MakerNoteRicoh entries  <- タグ数

    //0x0001 Rdc   //JPEGからコピー箇所　動作確認のため仮値をセット
    searchTag = (enablemaker == true) ? _readmakerdngLib.searchEntry(0x0001) : -1;
    if(searchTag != -1) {
        _makerdngLib.copyEntry(_readmakerdngLib.e_lists[searchTag]);
    }

    //0x0002 Cpuver
    searchTag = (enablemaker == true) ? _readmakerdngLib.searchEntry(0x0002) : -1;
    if(searchTag != -1) {
        _makerdngLib.copyEntry(_readmakerdngLib.e_lists[searchTag]);
    }

    //0x0003 Cammodel
    searchTag = (enablemaker == true) ? _readmakerdngLib.searchEntry(0x0003) : -1;
    if(searchTag != -1) {
        _makerdngLib.copyEntry(_readmakerdngLib.e_lists[searchTag]);
    }

    //0x0005 Camserial
    searchTag = (enablemaker == true) ? _readmakerdngLib.searchEntry(0x0005) : -1;
    if(searchTag != -1) {
        _makerdngLib.copyEntry(_readmakerdngLib.e_lists[searchTag]);
    }

    //0x1000 ImageQuality
    searchTag = (enablemaker == true) ? _readmakerdngLib.searchEntry(0x1000) : -1;
    if(searchTag != -1) {
        _makerdngLib.copyEntry(_readmakerdngLib.e_lists[searchTag]);
    }

    //0x1003 wbMode
    searchTag = (enablemaker == true) ? _readmakerdngLib.searchEntry(0x1003) : -1;
    if(searchTag != -1) {
        _makerdngLib.copyEntry(_readmakerdngLib.e_lists[searchTag]);
    }

    //0x1307 wbColorTemp
    searchTag = (enablemaker == true) ? _readmakerdngLib.searchEntry(0x1307) : -1;
    if(searchTag != -1) {
        _makerdngLib.copyEntry(_readmakerdngLib.e_lists[searchTag]);
    }

    //0x4001 ReceptorIFD     //正規の値は後でセット
    unsigned char d_m0x4001[4] = { 0x00, 0x00, 0x00, 0x00 };
    _makerdngLib.AddEntry(0x4001, ASCII, d_m0x4001, 4);


    // Receptor ifd ------------
    //dngLib _recepdngLib;

    //ReceptofIFDEntries  <- タグ数

    //0x0001 SphereType
    unsigned char d_r0x0001[2] = { 0x00, 0x03 };
    _recepdngLib.AddEntry(0x0001, SHORT, d_r0x0001, 2);

    //0x0002 SphereHDR
    searchTag = (enablereport == true) ? _readrecepdngLib.searchEntry(0x0002) : -1;
    if(searchTag != -1) {
        _recepdngLib.copyEntry(_readrecepdngLib.e_lists[searchTag]);
    }

    //0x0003 SpherePitchRoll
    unsigned char d_r0x0003[8*2] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
    _recepdngLib.AddEntry(0x0003, SRATIONAL, d_r0x0003, 8*2);

    //0x0004 SphereYaw
    searchTag = (enablereport == true) ? _readrecepdngLib.searchEntry(0x0004) : -1;
    if(searchTag != -1) {
        _recepdngLib.copyEntry(_readrecepdngLib.e_lists[searchTag]);
    }

    //0x0006 SphereNoiseReduction
    searchTag = (enablereport == true) ? _readrecepdngLib.searchEntry(0x0006) : -1;
    if(searchTag != -1) {
        _recepdngLib.copyEntry(_readrecepdngLib.e_lists[searchTag]);
    }

    //0x0007 SphereAdjZenith
    unsigned char d_r0x0007[2] = { 0x00, 0x00 };
    _recepdngLib.AddEntry(0x0007, SHORT, d_r0x0007, 2);

    //0x000E SphereHhHDR
    searchTag = (enablereport == true) ? _readrecepdngLib.searchEntry(0x000E) : -1;
    if(searchTag != -1) {
        _recepdngLib.copyEntry(_readrecepdngLib.e_lists[searchTag]);
    }

    //0x0013 SphereAdjRollingShutter
    searchTag = (enablereport == true) ? _readrecepdngLib.searchEntry(0x0013) : -1;
    if(searchTag != -1) {
        _recepdngLib.copyEntry(_readrecepdngLib.e_lists[searchTag]);
    }

    //0x0015 CenterOfExposureTimeStamp
    searchTag = (enablereport == true) ? _readrecepdngLib.searchEntry(0x0015) : -1;
    if(searchTag != -1) {
        _recepdngLib.copyEntry(_readrecepdngLib.e_lists[searchTag]);
    }

    //0x0016 DynamicStitchingResult
    searchTag = (enablereport == true) ? _readrecepdngLib.searchEntry(0x0016) : -1;
    if(searchTag != -1) {
        _recepdngLib.copyEntry(_readrecepdngLib.e_lists[searchTag]);
    }

    //0x0101 SphereISOFilmSpeed
    searchTag = (enablereport == true) ? _readrecepdngLib.searchEntry(0x0101) : -1;
    if(searchTag != -1) {
        _recepdngLib.copyEntry(_readrecepdngLib.e_lists[searchTag]);
    }

    //0x0102 SphereFNumber
    searchTag = (enablereport == true) ? _readrecepdngLib.searchEntry(0x0102) : -1;
    if(searchTag != -1) {
        _recepdngLib.copyEntry(_readrecepdngLib.e_lists[searchTag]);
    }

    //0x0103 SphereExposureTime
    searchTag = (enablereport == true) ? _readrecepdngLib.searchEntry(0x0103) : -1;
    if(searchTag != -1) {
        _recepdngLib.copyEntry(_readrecepdngLib.e_lists[searchTag]);
    }

    //0x0106 SphereShootingMode
    searchTag = (enablereport == true) ? _readrecepdngLib.searchEntry(0x0106) : -1;
    if(searchTag != -1) {
        _recepdngLib.copyEntry(_readrecepdngLib.e_lists[searchTag]);
    }

    //0x0107 ProcStitching
    unsigned char d_r0x0107[2] = { 0x00, 0x01 };
    _recepdngLib.AddEntry(0x0107, SHORT, d_r0x0107, 2);

    //0x0108 ProcZenithCorrection
    unsigned char d_r0x0108[2] = { 0x00, 0x01 };
    _recepdngLib.AddEntry(0x0108, SHORT, d_r0x0108, 2);

    //0x0109 ZenithDirection
    searchTag = (enablereport == true) ? _readrecepdngLib.searchEntry(0x0109) : -1;
    if(searchTag != -1) {
        _recepdngLib.copyEntry(_readrecepdngLib.e_lists[searchTag]);
    }

    //0x010A StitchingDownside
    searchTag = (enablereport == true) ? _readrecepdngLib.searchEntry(0x010A) : -1;
    if(searchTag != -1) {
        _recepdngLib.copyEntry(_readrecepdngLib.e_lists[searchTag]);
    }

    //0x010B ExposureDifferenceGain
    searchTag = (enablereport == true) ? _readrecepdngLib.searchEntry(0x010B) : -1;
    if(searchTag != -1) {
        _recepdngLib.copyEntry(_readrecepdngLib.e_lists[searchTag]);
    }

    //0x010C DRCompGain
    searchTag = (enablereport == true) ? _readrecepdngLib.searchEntry(0x010C) : -1;
    if(searchTag != -1) {
        _recepdngLib.copyEntry(_readrecepdngLib.e_lists[searchTag]);
    }

    //0x010D waterHousing
    searchTag = (enablereport == true) ? _readrecepdngLib.searchEntry(0x010D) : -1;
    if(searchTag != -1) {
        _recepdngLib.copyEntry(_readrecepdngLib.e_lists[searchTag]);
    }

    //0x0200 StitchingLensInfo
    searchTag = (enablereport == true) ? _readrecepdngLib.searchEntry(0x0200) : -1;
    if(searchTag != -1) {
        _recepdngLib.copyEntry(_readrecepdngLib.e_lists[searchTag]);
    }

    //0x0300 adjAccOffset
    searchTag = (enablereport == true) ? _readrecepdngLib.searchEntry(0x0300) : -1;
    if(searchTag != -1) {
        _recepdngLib.copyEntry(_readrecepdngLib.e_lists[searchTag]);
    }

    //0x0301 adjAccGain
    searchTag = (enablereport == true) ? _readrecepdngLib.searchEntry(0x0301) : -1;
    if(searchTag != -1) {
        _recepdngLib.copyEntry(_readrecepdngLib.e_lists[searchTag]);
    }

    //0x0302 adjGyroOffset
    searchTag = (enablereport == true) ? _readrecepdngLib.searchEntry(0x0302) : -1;
    if(searchTag != -1) {
        _recepdngLib.copyEntry(_readrecepdngLib.e_lists[searchTag]);
    }

    //0x0303 adjGyroGain
    searchTag = (enablereport == true) ? _readrecepdngLib.searchEntry(0x0303) : -1;
    if(searchTag != -1) {
        _recepdngLib.copyEntry(_readrecepdngLib.e_lists[searchTag]);
    }

    //0x0310 statusAcc
    searchTag = (enablereport == true) ? _readrecepdngLib.searchEntry(0x0310) : -1;
    if(searchTag != -1) {
        _recepdngLib.copyEntry(_readrecepdngLib.e_lists[searchTag]);
    }

    //0x0311 statusGyro
    searchTag = (enablereport == true) ? _readrecepdngLib.searchEntry(0x0311) : -1;
    if(searchTag != -1) {
        _recepdngLib.copyEntry(_readrecepdngLib.e_lists[searchTag]);
    }

    //0x0312 statusCompass
    searchTag = (enablereport == true) ? _readrecepdngLib.searchEntry(0x0312) : -1;
    if(searchTag != -1) {
        _recepdngLib.copyEntry(_readrecepdngLib.e_lists[searchTag]);
    }

    //0x0313 statusGPS
    searchTag = (enablereport == true) ? _readrecepdngLib.searchEntry(0x0313) : -1;
    if(searchTag != -1) {
        _recepdngLib.copyEntry(_readrecepdngLib.e_lists[searchTag]);
    }

    //0x1001 FrontTempStart
    searchTag = (enablereport == true) ? _readrecepdngLib.searchEntry(0x1001) : -1;
    if(searchTag != -1) {
        _recepdngLib.copyEntry(_readrecepdngLib.e_lists[searchTag]);
    }

    //0x1002 FrontTempEnd
    searchTag = (enablereport == true) ? _readrecepdngLib.searchEntry(0x1002) : -1;
    if(searchTag != -1) {
        _recepdngLib.copyEntry(_readrecepdngLib.e_lists[searchTag]);
    }

    //0x1003 RearTempStart
    searchTag = (enablereport == true) ? _readrecepdngLib.searchEntry(0x1003) : -1;
    if(searchTag != -1) {
        _recepdngLib.copyEntry(_readrecepdngLib.e_lists[searchTag]);
    }

    //0x1004 RearTempEnd
    searchTag = (enablereport == true) ? _readrecepdngLib.searchEntry(0x1004) : -1;
    if(searchTag != -1) {
        _recepdngLib.copyEntry(_readrecepdngLib.e_lists[searchTag]);
    }

    //0x1005 SphereWhiteBalanceRGain1
    searchTag = (enablereport == true) ? _readrecepdngLib.searchEntry(0x1005) : -1;
    if(searchTag != -1) {
        _recepdngLib.copyEntry(_readrecepdngLib.e_lists[searchTag]);
    }

    //0x1006 SphereWhiteBalanceBGain1
    searchTag = (enablereport == true) ? _readrecepdngLib.searchEntry(0x1006) : -1;
    if(searchTag != -1) {
        _recepdngLib.copyEntry(_readrecepdngLib.e_lists[searchTag]);
    }

    //0x1007 SphereWhiteBalanceRGain2
    searchTag = (enablereport == true) ? _readrecepdngLib.searchEntry(0x1007) : -1;
    if(searchTag != -1) {
        _recepdngLib.copyEntry(_readrecepdngLib.e_lists[searchTag]);
    }

    //0x1008 SphereWhiteBalanceBGain2
    searchTag = (enablereport == true) ? _readrecepdngLib.searchEntry(0x1008) : -1;
    if(searchTag != -1) {
        _recepdngLib.copyEntry(_readrecepdngLib.e_lists[searchTag]);
    }

    //0x1009 OB1
    searchTag = (enablereport == true) ? _readrecepdngLib.searchEntry(0x1009) : -1;
    if(searchTag != -1) {
        _recepdngLib.copyEntry(_readrecepdngLib.e_lists[searchTag]);
    }

    //0x100A OB2
    searchTag = (enablereport == true) ? _readrecepdngLib.searchEntry(0x100A) : -1;
    if(searchTag != -1) {
        _recepdngLib.copyEntry(_readrecepdngLib.e_lists[searchTag]);
    }

    //0x100B AnalogGain1
    searchTag = (enablereport == true) ? _readrecepdngLib.searchEntry(0x100B) : -1;
    if(searchTag != -1) {
        _recepdngLib.copyEntry(_readrecepdngLib.e_lists[searchTag]);
    }

    //0x100C AnalogGain2
    searchTag = (enablereport == true) ? _readrecepdngLib.searchEntry(0x100C) : -1;
    if(searchTag != -1) {
        _recepdngLib.copyEntry(_readrecepdngLib.e_lists[searchTag]);
    }

    //0x100D DiginalGain1
    searchTag = (enablereport == true) ? _readrecepdngLib.searchEntry(0x100D) : -1;
    if(searchTag != -1) {
        _recepdngLib.copyEntry(_readrecepdngLib.e_lists[searchTag]);
    }

    //0x100E DiginalGain2
    searchTag = (enablereport == true) ? _readrecepdngLib.searchEntry(0x100E) : -1;
    if(searchTag != -1) {
        _recepdngLib.copyEntry(_readrecepdngLib.e_lists[searchTag]);
    }

    //0x100F AEadjustValue
    searchTag = (enablereport == true) ? _readrecepdngLib.searchEntry(0x100F) : -1;
    if(searchTag != -1) {
        _recepdngLib.copyEntry(_readrecepdngLib.e_lists[searchTag]);
    }

    //0x1010 AWBadjustValue
    searchTag = (enablereport == true) ? _readrecepdngLib.searchEntry(0x1010) : -1;
    if(searchTag != -1) {
        _recepdngLib.copyEntry(_readrecepdngLib.e_lists[searchTag]);
    }

    //0x1015 ADB status
    searchTag = (enablereport == true) ? _readrecepdngLib.searchEntry(0x1015) : -1;
    if(searchTag != -1) {
        _recepdngLib.copyEntry(_readrecepdngLib.e_lists[searchTag]);
    }

    //0x1101 IMUTempStart
    searchTag = (enablereport == true) ? _readrecepdngLib.searchEntry(0x1101) : -1;
    if(searchTag != -1) {
        _recepdngLib.copyEntry(_readrecepdngLib.e_lists[searchTag]);
    }

    //0x1102 IMUTempEnd
    searchTag = (enablereport == true) ? _readrecepdngLib.searchEntry(0x1102) : -1;
    if(searchTag != -1) {
        _recepdngLib.copyEntry(_readrecepdngLib.e_lists[searchTag]);
    }


    //Write 0th IFD
    int wt = _dngLib.e_lists.size();

    int exf_wt = _exifdngLib.e_lists.size();
    int gps_wt = _gpsdngLib.e_lists.size();
    int mn_wt = _makerdngLib.e_lists.size();
    int rec_wt = _recepdngLib.e_lists.size();

    int raw_size = (int)size; //5508 * 5508 * 2;

    unsigned char data8769[4];
    unsigned char data8825[4];
    unsigned char data927C[4];
    unsigned char data4001[4];

    int start_exf = 8 + raw_size + 2 + _dngLib.e_lists.size() * 12 + 4;
    std::cout << start_exf << std::endl;

    int start_gps = start_exf + 2 + _exifdngLib.e_lists.size() * 12 + 4;
    int start_mn = start_gps + 2 + _gpsdngLib.e_lists.size() * 12 + 4;
    int start_rec = start_mn + 2 + _makerdngLib.e_lists.size() * 12 + 4 + 8; //MakerNoteのhead分

    __android_log_print(ANDROID_LOG_DEBUG, TAG, "[HDR-DNG-META] data size=%d exif size=%d gps size=%d mn size=%d", _dngLib.e_lists.size(), _exifdngLib.e_lists.size(), _gpsdngLib.e_lists.size(), _makerdngLib.e_lists.size());

    for (int i = 0; i < 4; i++) {
        data8769[3 - i] = start_exf & 0xff;
        data8825[3 - i] = start_gps & 0xff;
        data927C[3 - i] = start_mn & 0xff;
        data4001[3 - i] = start_rec & 0xff;

        //std::cout << (unsigned int)((unsigned char)data8769[3 - i]) << std::endl;
        start_exf = start_exf >> 8;
        start_gps = start_gps >> 8;
        start_mn = start_mn >> 8;
        start_rec = start_rec >> 8;
    }

    __android_log_print(ANDROID_LOG_DEBUG, TAG,"[HDR-DNG-META] start_exf data8769=%x%x%x%x", data8769[0], data8769[1], data8769[2], data8769[3]);
    __android_log_print(ANDROID_LOG_DEBUG, TAG,"[HDR-DNG-META] start_gps data8825=%x%x%x%x", data8825[0], data8825[1], data8825[2], data8825[3]);
    __android_log_print(ANDROID_LOG_DEBUG, TAG,"[HDR-DNG-META] start_mn  data927C=%x%x%x%x", data927C[0], data927C[1], data927C[2], data927C[3]);
    __android_log_print(ANDROID_LOG_DEBUG, TAG,"[HDR-DNG-META] start_rec data4001=%x%x%x%x", data4001[0], data4001[1], data4001[2], data4001[3]);


    //offset
    // 0th IFD : 0x8769 ExifIFDPointer
    _dngLib.DeleteEntry(0x8769);
    _dngLib.AddEntry(0x8769, LONG, data8769, 4);

    // 0th IFD : 0x8825 GPSIFDPointer
    _dngLib.DeleteEntry(0x8825);
    _dngLib.AddEntry(0x8825, LONG, data8825, 4);

    // Exif IFD : 0x927C MakerNote
    _exifdngLib.DeleteEntry(0x927C);
    _exifdngLib.AddEntry(0x927C, UNDEFINED, data927C, 8 + 2 + (12 * mn_wt));
    __android_log_print(ANDROID_LOG_DEBUG, TAG,"[HDR-DNG-META] data927C size=%x", 8 + 2 + (12 * mn_wt));

    // MakerNote : 0x4001 ReceptorIFD
    _makerdngLib.DeleteEntry(0x4001);
    _makerdngLib.AddEntry(0x4001, LONG, data4001, 4);


    char wtotal[2] = { (char)(wt / 256), (char)(wt % 256) };
    //dng main part size
    ofs.write(wtotal, 2);
    //2(count) + count * 12 + 4(NextIFD=0)
    __android_log_print(ANDROID_LOG_DEBUG, TAG,"[HDR-DNG-META] wtotal=%x %x", wtotal[0], wtotal[1]);

    //int offset = (int)(ofs.tellp()) + (12 * wt) + 4y;
    //int offset = (int)(ofs.tellp()) + (12 * wt) + 4 + 2 + (12 * exf_wt) + 4 + 2 + (12 * gps_wt) + 4;
    //int offset = (int)(ofs.tellp()) + (12 * wt) + 4 + 2 + (12 * exf_wt) + 4 + 2 + (12 * gps_wt) + 4 + 8 + 2 + (12 * mn_wt) + 4;
    int offset = (int)(ofs.tellp()) + (12 * wt) + 4 + 2 + (12 * exf_wt) + 4 + 2 + (12 * gps_wt) + 4 + 8 + 2 + (12 * mn_wt) + 4 + 2 + (12 * rec_wt) + 4;

    __android_log_print(ANDROID_LOG_DEBUG, TAG, "[HDR-DNG-META] offset=%x ofs.tellp=%x", offset, (int)(ofs.tellp()));

    for (int i = 0; i < _dngLib.e_lists.size(); i++) {
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
             __android_log_print(ANDROID_LOG_DEBUG, TAG, "[HDR-DNG-META] dng tag=%d type=%d size=%d num=%d data=%d", b2i(e->tag, sizeof(e->tag)), b2i(e->typ, sizeof(e->typ)), e->length, b2i(e->num, sizeof(e->num)),  b2i(e->edata, bnum) );
            ofs.write(e->edata, bnum);
            ofs.seekp(cadr + 12, std::ios_base::beg);

        }
        else {
            __android_log_print(ANDROID_LOG_DEBUG, TAG, "[HDR-DNG-META] △dng tag=%d type=%d size=%d num=%d data=%d", b2i(e->tag, sizeof(e->tag)), b2i(e->typ, sizeof(e->typ)), e->length, b2i(e->num, sizeof(e->num)),  b2i(e->edata, bnum) );
            char pos[4];
            i2b(offset, pos);
            __android_log_print(ANDROID_LOG_DEBUG, TAG, "[HDR-DNG-META] △dng tag=%x point=%x ", b2i(e->tag, sizeof(e->tag)), b2i(pos, sizeof(pos)));
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


    // -----------------------

    __android_log_print(ANDROID_LOG_DEBUG, TAG, "[HDR-DNG-META] exif start point=%x ", (int)ofs.tellp());
    //additional data(exifIFD, gpsIFD, extra, extra-exif, extra-gps)
    //exif
    wtotal[0] = (char)(exf_wt / 256);
    wtotal[1] = (char)(exf_wt % 256);
    ofs.write(wtotal, 2);
    __android_log_print(ANDROID_LOG_DEBUG, TAG,"[HDR-DNG-META] wtotal=%x %x", wtotal[0], wtotal[1]);
    //int offset = (int)(ofs.tellp()) + (12 * wt) + 4;
    for (int i = 0; i < _exifdngLib.e_lists.size(); i++) {
        dngEntry *e = _exifdngLib.e_lists[i];
        //edataの長さとdlengthからoffsetに書くか、そのまま書くか
        int dl = e->length;
        int dn = b2i(e->num, 4);
        int bnum = dl * dn;

        int cadr = ofs.tellp();
        ofs.write(e->tag, 2);
        ofs.write(e->typ, 2);
        ofs.write(e->num, 4);
        if (bnum <= 4 || b2i(e->tag,sizeof(e->tag)) == 0x927C) {
            //std::cout << "exif tag= " << b2i(e->tag, sizeof(e->tag)) << " type= " << b2i(e->typ, sizeof(e->typ)) << " size= " << e->length << " num= " << b2i(e->num, sizeof(e->num)) << " data= " << b2i(e->dat, sizeof(e->dat)) << std::endl;
            __android_log_print(ANDROID_LOG_DEBUG, TAG, "[HDR-DNG-META] exif tag=%d type=%d size=%d num=%d data=%d", b2i(e->tag, sizeof(e->tag)), b2i(e->typ, sizeof(e->typ)), e->length, b2i(e->num, sizeof(e->num)),  b2i(e->edata, bnum) );
            __android_log_print(ANDROID_LOG_DEBUG, TAG, "[HDR-DNG-META] exif tag=%x write point=%x ", b2i(e->tag, sizeof(e->tag)), cadr);
            ofs.write(e->edata, bnum);
            ofs.seekp(cadr + 12, std::ios_base::beg);
        }
        else {
            //std::cout << "exif tag= " << b2i(e->tag, sizeof(e->tag)) << " type= " << b2i(e->typ, sizeof(e->typ)) << " size= " << e->length << " num= " << b2i(e->num, sizeof(e->num)) << " data(offset)= " << offset << std::endl;
            __android_log_print(ANDROID_LOG_DEBUG, TAG, "[HDR-DNG-META] △exif tag=%d type=%d size=%d num=%d data=%d", b2i(e->tag, sizeof(e->tag)), b2i(e->typ, sizeof(e->typ)), e->length, b2i(e->num, sizeof(e->num)),  b2i(e->edata, sizeof(e->edata)) );
            __android_log_print(ANDROID_LOG_DEBUG, TAG, "[HDR-DNG-META] △exif tag=%x write point=%x ", b2i(e->tag, sizeof(e->tag)), cadr);
            char pos[4];
            i2b(offset, pos);
            __android_log_print(ANDROID_LOG_DEBUG, TAG, "[HDR-DNG-META] △exif tag=%x offset point=%x ", b2i(e->tag, sizeof(e->tag)), b2i(pos, sizeof(pos)));
            ofs.write(pos, 4);
            offset += bnum;
            if (bnum % 2 == 1) {
                offset += 1;
            }
        }
    }
    ofs.write(z, 4);

    //gps
    __android_log_print(ANDROID_LOG_DEBUG, TAG, "[HDR-DNG-META] gps start point=%x ", (int) ofs.tellp());
    wtotal[0] = (char) (gps_wt / 256);
    wtotal[1] = (char) (gps_wt % 256);
    ofs.write(wtotal, 2);
    __android_log_print(ANDROID_LOG_DEBUG, TAG, "[HDR-DNG-META] wtotal=%x %x", wtotal[0],wtotal[1]);
    //int offset = (int)(ofs.tellp()) + (12 * wt) + 4;
    if(enablegpg) {
        for (int i = 0; i < gps_wt; i++) {
            dngEntry *e = _gpsdngLib.e_lists[i];
            //edataの長さとdlengthからoffsetに書くか、そのまま書くか
            int dl = e->length;
            int dn = b2i(e->num, 4);
            int bnum = dl * dn;

            int cadr = ofs.tellp();
            ofs.write(e->tag, 2);
            ofs.write(e->typ, 2);
            ofs.write(e->num, 4);
            if (bnum <= 4) {
                __android_log_print(ANDROID_LOG_DEBUG, TAG,
                                    "[HDR-DNG-META] gps tag=%d type=%d size=%d num=%d data=%d",
                                    b2i(e->tag, sizeof(e->tag)), b2i(e->typ, sizeof(e->typ)),
                                    e->length, b2i(e->num, sizeof(e->num)),
                                    b2i(e->edata, sizeof(e->edata)));
                ofs.write(e->edata, bnum);
                ofs.seekp(cadr + 12, std::ios_base::beg);

            } else {
                __android_log_print(ANDROID_LOG_DEBUG, TAG,
                                    "[HDR-DNG-META] △gps tag=%d type=%d size=%d num=%d data=%d",
                                    b2i(e->tag, sizeof(e->tag)), b2i(e->typ, sizeof(e->typ)),
                                    e->length, b2i(e->num, sizeof(e->num)),
                                    b2i(e->edata, sizeof(e->edata)));
                char pos[4];
                i2b(offset, pos);
                __android_log_print(ANDROID_LOG_DEBUG, TAG, "[HDR-DNG-META] △gps tag=%x point=%x ",
                                    b2i(e->tag, sizeof(e->tag)), b2i(pos, sizeof(pos)));
                ofs.write(pos, 4);
                offset += bnum;
                if (bnum % 2 == 1) {
                    offset += 1;
                }
            }
        }
    }
    ofs.write(z, 4);

    //makernote
    __android_log_print(ANDROID_LOG_DEBUG, TAG, "[HDR-DNG-META] makerNote start point=%x ", (int)ofs.tellp());
    char mnhead[8] = { 0x52,0x69,0x63,0x6f,0x68,0x00,0x00,0x00 };  //Ricoh000
    ofs.write(mnhead, 8);

    wtotal[0] = (char)(mn_wt / 256);
    wtotal[1] = (char)(mn_wt % 256);
    ofs.write(wtotal, 2);
    __android_log_print(ANDROID_LOG_DEBUG, TAG,"[HDR-DNG-META] wtotal=%x %x", wtotal[0], wtotal[1]);
    //int offset = (int)(ofs.tellp()) + (12 * wt) + 4;
    for (int i = 0; i < mn_wt; i++) {
        dngEntry *e = _makerdngLib.e_lists[i];
        //edataの長さとdlengthからoffsetに書くか、そのまま書くか
        int dl = e->length;
        int dn = b2i(e->num, 4);
        int bnum = dl * dn;

        int cadr = ofs.tellp();
        __android_log_print(ANDROID_LOG_DEBUG, TAG, "[HDR-DNG-META] makerNote point=%x ", (int)ofs.tellp());
        ofs.write(e->tag, 2);
        ofs.write(e->typ, 2);
        ofs.write(e->num, 4);
        if (bnum <= 4) {
            __android_log_print(ANDROID_LOG_DEBUG, TAG, "[HDR-DNG-META] maker tag=%d type=%d size=%d num=%d data=%d", b2i(e->tag, sizeof(e->tag)), b2i(e->typ, sizeof(e->typ)), e->length, b2i(e->num, sizeof(e->num)),  b2i(e->edata, sizeof(e->edata)) );
            __android_log_print(ANDROID_LOG_DEBUG, TAG, "[HDR-DNG-META] maker data=%x%x%x%x", e->edata[0],e->edata[1],e->edata[2],e->edata[3]);
            ofs.write(e->edata, bnum);
            ofs.seekp(cadr + 12, std::ios_base::beg);
        }
        else {
            __android_log_print(ANDROID_LOG_DEBUG, TAG, "[HDR-DNG-META] △maker tag=%d type=%d size=%d num=%d data=%d", b2i(e->tag, sizeof(e->tag)), b2i(e->typ, sizeof(e->typ)), e->length, b2i(e->num, sizeof(e->num)),  b2i(e->edata, sizeof(e->edata)) );
            char pos[4];
            i2b(offset, pos);
            __android_log_print(ANDROID_LOG_DEBUG, TAG, "[HDR-DNG-META] △maker tag=%x point=%x ", b2i(e->tag, sizeof(e->tag)), b2i(pos, sizeof(pos)));
            ofs.write(pos, 4);
            offset += bnum;
            if (bnum % 2 == 1) {
                offset += 1;
            }
        }
    }
    ofs.write(z, 4);


    // Receptor IFD
    __android_log_print(ANDROID_LOG_DEBUG, TAG, "[HDR-DNG-META] Receptor start point=%x ", (int)ofs.tellp());

    wtotal[0] = (char)(rec_wt / 256);
    wtotal[1] = (char)(rec_wt % 256);
    ofs.write(wtotal, 2);
    __android_log_print(ANDROID_LOG_DEBUG, TAG,"[HDR-DNG-META] wtotal=%x %x", wtotal[0], wtotal[1]);
    //int offset = (int)(ofs.tellp()) + (12 * wt) + 4;
    for (int i = 0; i < rec_wt; i++) {
        dngEntry *e = _recepdngLib.e_lists[i];
        //edataの長さとdlengthからoffsetに書くか、そのまま書くか
        int dl = e->length;
        int dn = b2i(e->num, 4);
        int bnum = dl * dn;

        int cadr = ofs.tellp();
        ofs.write(e->tag, 2);
        ofs.write(e->typ, 2);
        ofs.write(e->num, 4);
        if (bnum <= 4) {
            __android_log_print(ANDROID_LOG_DEBUG, TAG, "[HDR-DNG-META] Receptor tag=%d type=%d size=%d num=%d data=%d", b2i(e->tag, sizeof(e->tag)), b2i(e->typ, sizeof(e->typ)), e->length, b2i(e->num, sizeof(e->num)),  b2i(e->edata, sizeof(e->edata)) );
            __android_log_print(ANDROID_LOG_DEBUG, TAG, "[HDR-DNG-META] Receptor data=%x%x%x%x", e->edata[0],e->edata[1],e->edata[2],e->edata[3]);
            ofs.write(e->edata, bnum);
            ofs.seekp(cadr + 12, std::ios_base::beg);

        }
        else {
            __android_log_print(ANDROID_LOG_DEBUG, TAG, "[HDR-DNG-META] △Receptor tag=%d type=%d size=%d num=%d data=%d", b2i(e->tag, sizeof(e->tag)), b2i(e->typ, sizeof(e->typ)), e->length, b2i(e->num, sizeof(e->num)),  b2i(e->edata, sizeof(e->edata)) );
            char pos[4];
            i2b(offset, pos);
            __android_log_print(ANDROID_LOG_DEBUG, TAG, "[HDR-DNG-META] △Receptor tag=%x point=%x ", b2i(e->tag, sizeof(e->tag)), b2i(pos, sizeof(pos)));
            ofs.write(pos, 4);
            offset += bnum;
            if (bnum % 2 == 1) {
                offset += 1;
            }
        }
    }
    ofs.write(z, 4);




    //------offsetの値を書き込み------------

    __android_log_print(ANDROID_LOG_DEBUG, TAG, "[HDR-DNG-META] extra start point=%x ", (int)ofs.tellp());
    //extra
    for (int i = 0; i < _dngLib.e_lists.size(); i++) {
        dngEntry *e = _dngLib.e_lists[i];
        int dl = e->length;
        int dn = b2i(e->num, 4);
        int bnum = dl * dn;

        if (bnum <= 4) {
            ; //no extra
        }
        else {
            __android_log_print(ANDROID_LOG_DEBUG, TAG, "[HDR-DNG-META] extra tag=%x point=%x ", b2i(e->tag, sizeof(e->tag)), (int)ofs.tellp());
            ofs.write(e->edata, bnum);
            char z[1] = { 0 };
            if (bnum % 2 != 0) ofs.write(z, 1);//fill
        }
    }

    __android_log_print(ANDROID_LOG_DEBUG, TAG, "[HDR-DNG-META] extra exif start point=%x ", (int)ofs.tellp());
    //仮
    //exif extra
    for (int i = 0; i < _exifdngLib.e_lists.size(); i++) {
        dngEntry *e = _exifdngLib.e_lists[i];
        int dl = e->length;
        int dn = b2i(e->num, 4);
        int bnum = dl * dn;

        if (bnum <= 4 || b2i(e->tag,sizeof(e->tag)) == 0x927C) {
            ; //no extra
        }
        else {
            __android_log_print(ANDROID_LOG_DEBUG, TAG, "[HDR-DNG-META] extra exif tag=%x point=%x bnum=%d", b2i(e->tag, sizeof(e->tag)), (int)ofs.tellp(), bnum);

            ofs.write(e->edata, bnum);
            char z[1] = { 0 };
            if (bnum % 2 != 0) ofs.write(z, 1);//fill
        }
    }

    if(enablegpg) {
        __android_log_print(ANDROID_LOG_DEBUG, TAG, "[HDR-DNG-META] extra gps start point=%x ", (int) ofs.tellp());
        //gps extra
        for (int i = 0; i < gps_wt; i++) {
            dngEntry *e = _gpsdngLib.e_lists[i];
            int dl = e->length;
            int dn = b2i(e->num, 4);
            int bnum = dl * dn;

            if (bnum <= 4) { ; //no extra
            } else {
                __android_log_print(ANDROID_LOG_DEBUG, TAG, "[HDR-DNG-META] extra gps tag=%x point=%x ",
                                    b2i(e->tag, sizeof(e->tag)), (int) ofs.tellp());
                ofs.write(e->edata, bnum);
                char z[1] = {0};
                if (bnum % 2 != 0) ofs.write(z, 1);//fill
            }
        }
    }

    //MakerNote extra
    for (int i = 0; i < mn_wt; i++) {
        dngEntry *e = _makerdngLib.e_lists[i];
        int dl = e->length;
        int dn = b2i(e->num, 4);
        int bnum = dl * dn;

        if (bnum <= 4) {
            ; //no extra
        }
        else {
            __android_log_print(ANDROID_LOG_DEBUG, TAG, "[HDR-DNG-META] extra MakerNote tag=%x point=%x bnum=%d", b2i(e->tag, sizeof(e->tag)), (int)ofs.tellp(), bnum);
            ofs.write(e->edata, bnum);
            char z[1] = { 0 };
            if (bnum % 2 != 0) ofs.write(z, 1);//fill
        }
    }

    //Receptor extra
    for (int i = 0; i < rec_wt; i++) {
        dngEntry *e = _recepdngLib.e_lists[i];
        int dl = e->length;
        int dn = b2i(e->num, 4);
        int bnum = dl * dn;

        if (bnum <= 4) {
            ; //no extra
        }
        else {
            __android_log_print(ANDROID_LOG_DEBUG, TAG, "[HDR-DNG-META] extra Receptor tag=%x point=%x bnum=%d", b2i(e->tag, sizeof(e->tag)), (int)ofs.tellp(), bnum);
            ofs.write(e->edata, bnum);
            char z[1] = { 0 };
            if (bnum % 2 != 0) ofs.write(z, 1);//fill
        }
    }
    // -----------------------

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

int dngLib::searchEntry(int tag) {
    for (int i = 0; i < (int)e_lists.size(); ++i) {
        int t = b2i(e_lists[i]->tag, 2);
        if (t == tag) {
            return i;
        }
    }
    return -1;
}

void dngLib::DeleteEntry(int tag) {
    int ei = searchEntry(tag);
    if (ei >= 0) {
        e_lists.erase(e_lists.begin() + ei);
    }
    //std::wcout << "delete entry " << tag << std::endl;
}

void dngLib::readEntryEach(std::ifstream& ifs, int adr, dngEntry& e) {
    ifs.seekg(adr, std::ios_base::beg);
    ifs.read(e.tag, 2);
    ifs.read(e.typ, 2);
    ifs.read(e.num, 4);
    ifs.read(e.dat, 4);

    int dlength = dtype2bytenum(b2i(e.typ, sizeof(e.typ)));
    e.length = dlength;
    int dnum = b2i(e.num, sizeof(e.num));
    int rbytenum = dlength * dnum;
    if (rbytenum <= 4) {
        e.new_edata(4);
        e.edata[0] = e.dat[0];
        e.edata[1] = e.dat[1];
        e.edata[2] = e.dat[2];
        e.edata[3] = e.dat[3];
    }
    else {
        //e.edata.resize(rbytenum);
        e.new_edata(rbytenum);
        //long c_adr = ifs.tellg();
        long t_adr = b2i(e.dat, sizeof(e.dat));
        t_adr = t_adr + 12;
        ifs.seekg(t_adr, std::ios_base::beg);
        ifs.read(e.edata, rbytenum);
        //ifs.seekg(c_adr+4, std::ios_base::beg);
    }
}

int dngLib::readEntryCnt(std::ifstream& ifs, int adr) {
    ifs.seekg(adr, std::ios_base::beg);
    char ver[2];
    ifs.read(ver, 2);
    return b2i(ver, sizeof(ver));
}

void dngLib::AddEntry(int _tag, int _typ, unsigned char * _dat, int _num)
{
    dngEntry *e = new dngEntry();

    (e->tag)[1] = 0xff & _tag;
    (e->tag)[0] = 0xff & (_tag >> 8);

    (e->typ)[1] = 0xff & _typ;
    (e->typ)[0] = 0xff & (_typ >> 8);

    int _length = dtype2bytenum(b2i(e->typ, sizeof(e->typ)));
    if(_tag == 0x8769) __android_log_print(ANDROID_LOG_DEBUG, TAG, "[HDR-DNG-META] 0x8769 Entry length=%d", _length );
    e->length = _length;

    int __num = _num / e->length;
    (e->num)[3] = 0xff & __num;			//TODO
    (e->num)[2] = 0xff & (_num >> 8);
    (e->num)[1] = 0xff & (_num >> 16);
    (e->num)[0] = 0xff & (_num >> 24);

    if(_tag == 0x927C){
        __android_log_print(ANDROID_LOG_DEBUG, TAG, "[HDR-DNG-META] Entry type=%d", _length );
        int mn_num = 4;
        e->new_edata(mn_num);
        for (int i = 0; i < mn_num; i++) {
            (e->edata)[i] = _dat[i];
        }
    }else{
        e->new_edata(_num);
        for (int i = 0; i < _num; i++) {
            (e->edata)[i] = _dat[i];
        }
    }


    //std::cout << int(e->tag[0]) << std::endl;
    //std::cout << int(e->tag[1]) << std::endl;
    //std::cout << e->typ << std::endl;
    //std::cout << e->num << std::endl;

    e_lists.push_back(e);
}

void dngLib::copyEntry(dngEntry * src)
{
    dngEntry *e = new dngEntry();

    (e->tag)[1] = (src->tag)[1];
    (e->tag)[0] = (src->tag)[0];

    (e->typ)[1] = (src->typ)[1];
    (e->typ)[0] = (src->typ)[0];

    int _length = dtype2bytenum(b2i(e->typ, sizeof(e->typ)));
    e->length = _length;

    for (int i = 0; i < 4; i++) {
        (e->num)[i] = (src->num)[i];
    }

    int dnum = b2i(e->num, sizeof(e->num));
    int rbytenum = _length * dnum;
    if (rbytenum <= 4) {
        e->new_edata(4);
        e->edata[0] = src->dat[0];
        e->edata[1] = src->dat[1];
        e->edata[2] = src->dat[2];
        e->edata[3] = src->dat[3];
    }
    else {
        e->new_edata(rbytenum);
        for (int i = 0; i < rbytenum; i++) {
            //(e->edata)[i] = _dat[i];
            e->edata[i] = src->edata[i];
        }
        __android_log_print(ANDROID_LOG_DEBUG, TAG, "[HDR-DNG-META] copyEntry edata: edata=%d", b2i(e->edata, 4) );
    }
    __android_log_print(ANDROID_LOG_DEBUG, TAG, "[HDR-DNG-META] copyEntry: tag=%d type=%d size=%d num=%d data=%d", b2i(e->tag, sizeof(e->tag)), b2i(e->typ, sizeof(e->typ)), e->length, b2i(e->num, sizeof(e->num)), b2i(e->edata, 4));
    e_lists.push_back(e);
}
