﻿// -----------------------------------------------------------------------------------------
// NVEnc by rigaya
// -----------------------------------------------------------------------------------------
// The MIT License
//
// Copyright (c) 2014-2016 rigaya
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//
// ------------------------------------------------------------------------------------------

#include <string.h>
#include <stdio.h>
#include <stddef.h>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <shlwapi.h>
#pragma comment(lib, "shlwapi.lib")
#include "auo_util.h"
#include "auo_conf.h"

const int guiEx_config::conf_block_data[CONF_BLOCK_COUNT] = {
    sizeof(CONF_GUIEX::nvenc),
    sizeof(CONF_GUIEX::vid),
    sizeof(CONF_GUIEX::aud),
    sizeof(CONF_GUIEX::mux),
    sizeof(CONF_GUIEX::oth)
};

const size_t guiEx_config::conf_block_pointer[CONF_BLOCK_COUNT] = {
    offsetof(CONF_GUIEX, nvenc),
    offsetof(CONF_GUIEX, vid),
    offsetof(CONF_GUIEX, aud),
    offsetof(CONF_GUIEX, mux),
    offsetof(CONF_GUIEX, oth)
};

guiEx_config::guiEx_config() { }

void guiEx_config::write_conf_header(CONF_GUIEX *save_conf) {
    sprintf_s(save_conf->conf_name, sizeof(save_conf->conf_name), CONF_NAME);
    save_conf->size_all = sizeof(CONF_GUIEX);
    save_conf->head_size = CONF_HEAD_SIZE;
    save_conf->block_count = CONF_BLOCK_COUNT;
    for (int i = 0; i < CONF_BLOCK_COUNT; ++i) {
        save_conf->block_size[i] = conf_block_data[i];
        save_conf->block_head_p[i] = conf_block_pointer[i];
    }
}

//設定ファイルサイズを自動拡張する
//拡張できない場合 FALSEを返す
BOOL guiEx_config::adjust_conf_size(CONF_GUIEX *conf_buf, void *old_data, int old_size) {
    BOOL ret = FALSE;
    init_CONF_GUIEX(conf_buf, FALSE);
    CONF_GUIEX *old_conf = (CONF_GUIEX *)old_data;
    if (old_conf->size_all != CONF_INITIALIZED)
        return ret;

    if (0 == strcmp(old_conf->conf_name, CONF_NAME_OLD_1)) {
        convert_nvencstg_to_nvencstgv4(conf_buf, old_conf);
    } else if (0 == strcmp(old_conf->conf_name, CONF_NAME_OLD_2)) {
        convert_nvencstgv2_to_nvencstgv4(conf_buf, old_conf);
    } else if (0 == strcmp(old_conf->conf_name, CONF_NAME_OLD_3)) {
        convert_nvencstgv3_to_nvencstgv4(conf_buf, old_conf);
    } else {
        BYTE *dst = (BYTE *)conf_buf;
        BYTE *block = nullptr;
        dst += CONF_HEAD_SIZE;
        //ブロック部分のコピー
        for (int i = 0; i < old_conf->block_count; ++i) {
            block = (BYTE *)old_conf + old_conf->block_head_p[i];
            dst = (BYTE *)conf_buf + conf_block_pointer[i];
            memcpy(dst, block, std::min(old_conf->block_size[i], conf_block_data[i]));
        }
    }
    ret = TRUE;
    return ret;
}

int guiEx_config::load_guiEx_conf(CONF_GUIEX *conf, const char *stg_file) {
    size_t conf_size = 0;
    BYTE *dst, *filedat;
    //初期化
    ZeroMemory(conf, sizeof(CONF_GUIEX));
    //ファイルからロード
    FILE *fp = NULL;
    if (fopen_s(&fp, stg_file, "rb") != NULL)
        return CONF_ERROR_FILE_OPEN;
    //設定ファイルチェック
    char conf_name[CONF_NAME_BLOCK_LEN + 32];
    fread(&conf_name, sizeof(char), CONF_NAME_BLOCK_LEN, fp);
    if (   strcmp(CONF_NAME,       conf_name)
        && strcmp(CONF_NAME_OLD_1, conf_name)
        && strcmp(CONF_NAME_OLD_2, conf_name)
        && strcmp(CONF_NAME_OLD_3, conf_name)) {
        fclose(fp);
        return CONF_ERROR_FILE_OPEN;
    }
    fread(&conf_size, sizeof(int), 1, fp);
    BYTE *dat = (BYTE*)calloc(conf_size, 1);
    init_CONF_GUIEX(conf, FALSE);
    fseek(fp, 0, SEEK_SET);
    fread(dat, conf_size, 1, fp);
    fclose(fp);

    //ブロックサイズチェック
    if (((CONF_GUIEX *)dat)->block_count > std::max(CONF_BLOCK_COUNT, stgv3_block_size()))
        return CONF_ERROR_BLOCK_SIZE;

    write_conf_header(conf);

    //旧設定ファイルから変換
    if (strcmp(conf_name, CONF_NAME_OLD_1) == 0) {
        convert_nvencstg_to_nvencstgv4(conf, dat);
    } else if (strcmp(conf_name, CONF_NAME_OLD_2) == 0) {
        convert_nvencstgv2_to_nvencstgv4(conf, dat);
    } else if (strcmp(conf_name, CONF_NAME_OLD_3) == 0) {
        convert_nvencstgv3_to_nvencstgv4(conf, dat);
    } else {
        dst = (BYTE *)conf;
        //filedat = (BYTE *)data;
        //memcpy(dst, filedat, data->head_size);
        dst += CONF_HEAD_SIZE;

        //ブロック部分のコピー
        for (int i = 0; i < ((CONF_GUIEX *)dat)->block_count; ++i) {
            filedat = dat + ((CONF_GUIEX *)dat)->block_head_p[i];
            dst = (BYTE *)conf + conf_block_pointer[i];
            memcpy(dst, filedat, std::min(((CONF_GUIEX *)dat)->block_size[i], conf_block_data[i]));
        }
    }

    //初期化するかどうかで使うので。
    conf->size_all = CONF_INITIALIZED;
    free(dat);
    return 0;
}

int guiEx_config::save_guiEx_conf(const CONF_GUIEX *conf, const char *stg_file) {
    CONF_GUIEX save_conf;
    memcpy(&save_conf, conf, sizeof(CONF_GUIEX));
    ZeroMemory(&save_conf.block_count, sizeof(save_conf.block_count));

    //展開したコマンドライン
    //char cmd_all[MAX_CMD_LEN] = { 0 };
    //build_cmd_from_conf(cmd_all, sizeof(cmd_all), &conf->x264, &conf->vid, TRUE);
    //DWORD cmd_all_len = strlen(cmd_all) + 1;

    //設定ファイルのブロックごとの大きさを書き込み
    sprintf_s(save_conf.conf_name, sizeof(save_conf.conf_name), CONF_NAME);
    save_conf.size_all = sizeof(CONF_GUIEX)/* + cmd_all_len*/;
    save_conf.head_size = CONF_HEAD_SIZE;
    save_conf.block_count = CONF_BLOCK_COUNT;
    for (int i = 0; i < CONF_BLOCK_COUNT; ++i) {
        save_conf.block_size[i] = conf_block_data[i];
        save_conf.block_head_p[i] = conf_block_pointer[i];
    }
    //最後に展開したコマンドラインを追加する
    //save_conf.block_size[CONF_BLOCK_COUNT]   = cmd_all_len;
    //save_conf.block_head_p[CONF_BLOCK_COUNT] = sizeof(CONF_GUIEX);

    //ファイルへ書きこみ
    FILE *fp = NULL;
    if (fopen_s(&fp, stg_file, "wb") != NULL)
        return CONF_ERROR_FILE_OPEN;
    fwrite(&save_conf, sizeof(CONF_GUIEX), 1, fp);
    //fwrite(cmd_all,    cmd_all_len,            1, fp);
    fclose(fp);
    return 0;
}