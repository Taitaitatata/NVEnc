﻿// -----------------------------------------------------------------------------------------
// NVEnc by rigaya
// -----------------------------------------------------------------------------------------
//
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

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <stdio.h>
#include <shlwapi.h>
#pragma comment(lib, "shlwapi.lib")
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")

#include "output.h"
#include "auo.h"
#include "auo_frm.h"
#include "auo_util.h"
#include "auo_error.h"
#include "auo_version.h"
#include "auo_conf.h"
#include "auo_system.h"

#include "auo_video.h"
#include "auo_audio.h"
#include "auo_faw2aac.h"
#include "auo_mux.h"
#include "auo_encode.h"
#include "auo_runbat.h"

#include "NVEncParam.h"

//---------------------------------------------------------------------
//        出力プラグイン内部変数
//---------------------------------------------------------------------

static CONF_GUIEX g_conf = { 0 };
static SYSTEM_DATA g_sys_dat = { 0 };
static char g_auo_filefilter[1024] = { 0 };


//---------------------------------------------------------------------
//        出力プラグイン構造体定義
//---------------------------------------------------------------------
OUTPUT_PLUGIN_TABLE output_plugin_table = {
    NULL,                         // フラグ
    AUO_FULL_NAME,                // プラグインの名前
    AUO_EXT_FILTER,               // 出力ファイルのフィルタ
    AUO_VERSION_INFO,             // プラグインの情報
    func_init,                    // DLL開始時に呼ばれる関数へのポインタ (NULLなら呼ばれません)
    func_exit,                    // DLL終了時に呼ばれる関数へのポインタ (NULLなら呼ばれません)
    func_output,                  // 出力時に呼ばれる関数へのポインタ
    func_config,                  // 出力設定のダイアログを要求された時に呼ばれる関数へのポインタ (NULLなら呼ばれません)
    func_config_get,              // 出力設定データを取得する時に呼ばれる関数へのポインタ (NULLなら呼ばれません)
    func_config_set,              // 出力設定データを設定する時に呼ばれる関数へのポインタ (NULLなら呼ばれません)
};


//---------------------------------------------------------------------
//        出力プラグイン構造体のポインタを渡す関数
//---------------------------------------------------------------------
EXTERN_C OUTPUT_PLUGIN_TABLE __declspec(dllexport) * __stdcall GetOutputPluginTable( void ) {
    init_SYSTEM_DATA(&g_sys_dat);
    make_file_filter(NULL, 0, g_sys_dat.exstg->s_local.default_output_ext);
    overwrite_aviutl_ini_file_filter(g_sys_dat.exstg->s_local.default_output_ext);
    output_plugin_table.filefilter = g_auo_filefilter;
    return &output_plugin_table;
}


//---------------------------------------------------------------------
//        出力プラグイン出力関数
//---------------------------------------------------------------------
//
    //int        flag;            //    フラグ
    //                        //    OUTPUT_INFO_FLAG_VIDEO    : 画像データあり
    //                        //    OUTPUT_INFO_FLAG_AUDIO    : 音声データあり
    //                        //    OUTPUT_INFO_FLAG_BATCH    : バッチ出力中
    //int        w,h;            //    縦横サイズ
    //int        rate,scale;        //    フレームレート
    //int        n;                //    フレーム数
    //int        size;            //    １フレームのバイト数
    //int        audio_rate;        //    音声サンプリングレート
    //int        audio_ch;        //    音声チャンネル数
    //int        audio_n;        //    音声サンプリング数
    //int        audio_size;        //    音声１サンプルのバイト数
    //LPSTR    savefile;        //    セーブファイル名へのポインタ
    //void    *(*func_get_video)( int frame );
    //                        //    DIB形式(RGB24bit)の画像データへのポインタを取得します。
    //                        //    frame    : フレーム番号
    //                        //    戻り値    : データへのポインタ
    //                        //              画像データポインタの内容は次に外部関数を使うかメインに処理を戻すまで有効
    //void    *(*func_get_audio)( int start,int length,int *readed );
    //                        //    16bitPCM形式の音声データへのポインタを取得します。
    //                        //    start    : 開始サンプル番号
    //                        //    length    : 読み込むサンプル数
    //                        //    readed    : 読み込まれたサンプル数
    //                        //    戻り値    : データへのポインタ
    //                        //              音声データポインタの内容は次に外部関数を使うかメインに処理を戻すまで有効
    //BOOL    (*func_is_abort)( void );
    //                        //    中断するか調べます。
    //                        //    戻り値    : TRUEなら中断
    //BOOL    (*func_rest_time_disp)( int now,int total );
    //                        //    残り時間を表示させます。
    //                        //    now        : 処理しているフレーム番号
    //                        //    total    : 処理する総フレーム数
    //                        //    戻り値    : TRUEなら成功
    //int        (*func_get_flag)( int frame );
    //                        //    フラグを取得します。
    //                        //    frame    : フレーム番号
    //                        //    戻り値    : フラグ
    //                        //  OUTPUT_INFO_FRAME_FLAG_KEYFRAME        : キーフレーム推奨
    //                        //  OUTPUT_INFO_FRAME_FLAG_COPYFRAME    : コピーフレーム推奨
    //BOOL    (*func_update_preview)( void );
    //                        //    プレビュー画面を更新します。
    //                        //    最後にfunc_get_videoで読み込まれたフレームが表示されます。
    //                        //    戻り値    : TRUEなら成功
    //void    *(*func_get_video_ex)( int frame,DWORD format );
    //                        //    DIB形式の画像データを取得します。
    //                        //    frame    : フレーム番号
    //                        //    format    : 画像フォーマット( NULL = RGB24bit / 'Y''U''Y''2' = YUY2 / 'Y''C''4''8' = PIXEL_YC )
    //                        //              ※PIXEL_YC形式 は YUY2フィルタモードでは使用出来ません。
    //                        //    戻り値    : データへのポインタ
    //                        //              画像データポインタの内容は次に外部関数を使うかメインに処理を戻すまで有効

BOOL func_init() {
    return TRUE;
}

BOOL func_exit() {
    delete_SYSTEM_DATA(&g_sys_dat);
    return TRUE;
}

BOOL func_output( OUTPUT_INFO *oip ) {
    AUO_RESULT ret = AUO_RESULT_SUCCESS;
    static const encode_task task[3][2] = { { video_output, audio_output }, { audio_output, video_output }, { audio_output_parallel, video_output }  };
    PRM_ENC pe = { 0 };
    CONF_GUIEX conf_out = { 0 };
    const DWORD tm_start_enc = timeGetTime();
    char default_stg_file[MAX_PATH_LEN] = { 0 };

    //データの初期化
    init_SYSTEM_DATA(&g_sys_dat);
    if (!g_sys_dat.exstg->get_init_success()) return FALSE;

    const bool conf_not_initialized = memcmp(&conf_out, &g_conf, sizeof(g_conf)) == 0;
    if (conf_not_initialized) {
        PathCombine(default_stg_file, g_sys_dat.exstg->s_local.stg_dir, CONF_LAST_OUT);
        if (!PathFileExists(default_stg_file)
            || guiEx_config::load_guiEx_conf(&g_conf, default_stg_file) != CONF_ERROR_NONE) {
            //前回出力した設定ファイルがない場合は、デフォルト設定をロード
            init_CONF_GUIEX(&g_conf, FALSE);
            memset(default_stg_file, 0, sizeof(default_stg_file));
        }
    }
    conf_out = g_conf;

    //ログウィンドウを開く
    open_log_window(oip->savefile, &g_sys_dat, 1, 1);
    if (conf_not_initialized) {
        warning_conf_not_initialized();
    }
    set_prevent_log_close(TRUE); //※1 start

    //各種設定を行う
    set_enc_prm(&conf_out, &pe, oip, &g_sys_dat);
    pe.h_p_aviutl = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, GetCurrentProcessId()); //※2 start

    //チェックを行い、エンコード可能ならエンコードを開始する
    if (check_output(&conf_out, oip, &pe, g_sys_dat.exstg) && setup_afsvideo(oip, &g_sys_dat, &conf_out, &pe)) { //※3 start

        ret |= run_bat_file(&conf_out, oip, &pe, &g_sys_dat, RUN_BAT_BEFORE_PROCESS);

        const auto audio_encode_timing = (conf_out.aud.use_internal) ? 2 : conf_out.aud.ext.audio_encode_timing;
        for (int i = 0; !ret && i < 2; i++)
            ret |= task[audio_encode_timing][i](&conf_out, oip, &pe, &g_sys_dat);

        if (!ret)
            ret |= mux(&conf_out, oip, &pe, &g_sys_dat);

        ret |= move_temporary_files(&conf_out, &pe, &g_sys_dat, oip, ret);

        write_log_auo_enc_time("総エンコード時間  ", timeGetTime() - tm_start_enc);

        close_afsvideo(&pe); //※3 end

    } else {
        ret |= AUO_RESULT_ERROR;
    }

    if (ret & AUO_RESULT_ABORT) info_encoding_aborted();

    CloseHandle(pe.h_p_aviutl); //※2 end
    set_prevent_log_close(FALSE); //※1 end
    auto_save_log(&conf_out, oip, &pe, &g_sys_dat); //※1 end のあとで行うこと

    if (!(ret & (AUO_RESULT_ERROR | AUO_RESULT_ABORT)))
        ret |= run_bat_file(&conf_out, oip, &pe, &g_sys_dat, RUN_BAT_AFTER_PROCESS);

    log_process_events();
    // エラーが発生しなかった場合は設定を保存
    if (ret == AUO_RESULT_SUCCESS) {
        memset(default_stg_file, 0, sizeof(default_stg_file));
        PathCombine(default_stg_file, g_sys_dat.exstg->s_local.stg_dir, CONF_LAST_OUT);
        guiEx_config::save_guiEx_conf(&conf_out, default_stg_file);
    }
    free_enc_prm(&pe);
    return (ret & AUO_RESULT_ERROR) ? FALSE : TRUE;
}

//---------------------------------------------------------------------
//        出力プラグイン設定関数
//---------------------------------------------------------------------
//以下部分的にwarning C4100を黙らせる
//C4100 : 引数は関数の本体部で 1 度も参照されません。
#pragma warning( push )
#pragma warning( disable: 4100 )
BOOL func_config(HWND hwnd, HINSTANCE dll_hinst) {
    init_SYSTEM_DATA(&g_sys_dat);
    overwrite_aviutl_ini_name();
    if (g_sys_dat.exstg->get_init_success())
        ShowfrmConfig(&g_conf, &g_sys_dat);
    return TRUE;
}
#pragma warning( pop )

int func_config_get( void *data, int size ) {
    if (data && size == sizeof(CONF_GUIEX)) {
        memcpy(data, &g_conf, sizeof(g_conf));
    }
    return sizeof(g_conf);
}

int func_config_set( void *data,int size ) {
    init_SYSTEM_DATA(&g_sys_dat);
    if (!g_sys_dat.exstg->get_init_success(TRUE)) {
        return NULL;
    }
    init_CONF_GUIEX(&g_conf, FALSE);
    return (guiEx_config::adjust_conf_size(&g_conf, data, size)) ? size : NULL;
}


//---------------------------------------------------------------------
//        NVEncのその他の関数
//---------------------------------------------------------------------
void init_SYSTEM_DATA(SYSTEM_DATA *_sys_dat) {
    if (_sys_dat->init)
        return;
    get_auo_path(_sys_dat->auo_path, _countof(_sys_dat->auo_path));
    get_aviutl_dir(_sys_dat->aviutl_dir, _countof(_sys_dat->aviutl_dir));
    _sys_dat->exstg = new guiEx_settings();
    //set_ex_stg_ptr(_sys_dat->exstg);
    _sys_dat->init = TRUE;
}
void delete_SYSTEM_DATA(SYSTEM_DATA *_sys_dat) {
    if (_sys_dat->init) {
        delete _sys_dat->exstg;
        _sys_dat->exstg = NULL;
        //set_ex_stg_ptr(_sys_dat->exstg);
    }
    _sys_dat->init = FALSE;
}
#pragma warning( push )
#pragma warning( disable: 4100 )
void init_CONF_GUIEX(CONF_GUIEX *conf, BOOL use_10bit) {
    ZeroMemory(conf, sizeof(CONF_GUIEX));
    guiEx_config::write_conf_header(conf);
    conf->vid.resize_width = 1280;
    conf->vid.resize_height = 720;
    conf->aud.ext.encoder = g_sys_dat.exstg->s_local.default_audio_encoder_ext;
    conf->aud.in.encoder  = g_sys_dat.exstg->s_local.default_audio_encoder_in;
    conf->aud.use_internal = g_sys_dat.exstg->s_local.default_audenc_use_in;
    conf->mux.use_internal = TRUE;
    { const AUDIO_SETTINGS *aud_stg_in = &g_sys_dat.exstg->s_aud_int[conf->aud.in.encoder];
      conf->aud.in.bitrate = aud_stg_in->mode[conf->aud.in.enc_mode].bitrate_default; }
    { const AUDIO_SETTINGS *aud_stg_ext = &g_sys_dat.exstg->s_aud_ext[conf->aud.ext.encoder];
      conf->aud.ext.bitrate = aud_stg_ext->mode[conf->aud.ext.enc_mode].bitrate_default; }
    conf->size_all = CONF_INITIALIZED;
}
void write_log_line_fmt(int log_type_index, const char *format, ...) {
    va_list args;
    int len;
    char *buffer;
    va_start(args, format);
    len = _vscprintf(format, args) + 1; // _vscprintf doesn't count terminating '\0'
    buffer = (char *)malloc(len * sizeof(buffer[0]));
    vsprintf_s(buffer, len, format, args);
    write_log_line(log_type_index, buffer);
    free(buffer);
}
#pragma warning( pop )
void write_log_auo_line_fmt(int log_type_index, const char *format, ... ) {
    va_list args;
    int len;
    char *buffer;
    va_start(args, format);
    len = _vscprintf(format, args) // _vscprintf doesn't count
                              + 1; // terminating '\0'
    buffer = (char *)malloc(len * sizeof(buffer[0]));
    vsprintf_s(buffer, len, format, args);
    write_log_auo_line(log_type_index, buffer);
    free(buffer);
}
//エンコード時間の表示
void write_log_auo_enc_time(const char *mes, DWORD time) {
    time = ((time + 50) / 100) * 100; //四捨五入
    write_log_auo_line_fmt(LOG_INFO, "%s : %d時間%2d分%2d.%1d秒",
        mes,
        time / (60*60*1000),
        (time % (60*60*1000)) / (60*1000),
        (time % (60*1000)) / 1000,
        ((time % 1000)) / 100);
}

void overwrite_aviutl_ini_name() {
    char ini_file[1024];
    get_aviutl_dir(ini_file, _countof(ini_file));
    PathAddBackSlashLong(ini_file);
    strcat_s(ini_file, _countof(ini_file), "aviutl.ini");
    WritePrivateProfileString(AUO_NAME, "name", NULL, ini_file);
    WritePrivateProfileString(AUO_NAME, "name", AUO_FULL_NAME, ini_file);
}

void overwrite_aviutl_ini_file_filter(int idx) {
    char ini_file[1024];
    get_aviutl_dir(ini_file, _countof(ini_file));
    PathAddBackSlashLong(ini_file);
    strcat_s(ini_file, _countof(ini_file), "aviutl.ini");

    char filefilter_ini[1024] = { 0 };
    make_file_filter(filefilter_ini, _countof(filefilter_ini), idx);
    WritePrivateProfileString(AUO_NAME, "filefilter", filefilter_ini, ini_file);
}

void make_file_filter(char *filter, size_t nSize, int default_index) {
    static const char *const TOP = "All Support Formats (*.*)";
    const char separator = (filter) ? '\\' : '\0';
    if (filter == NULL) {
        filter = g_auo_filefilter;
        nSize = _countof(g_auo_filefilter);
    }
    char *ptr = filter;

    auto add_filter = [&](const char *str, char appendix) {
        size_t len = strlen(str);
        if (nSize - (ptr - filter) <= len + 1)
            return;
        memcpy(ptr, (str), sizeof(ptr[0]) * len);
        ptr += len;
        *ptr = appendix;
        ptr++;
    };
    auto add_desc = [&](int idx) {
        size_t len = sprintf_s(ptr, nSize - (ptr - filter), "%s (%s)", OUTPUT_FILE_EXT_DESC[idx], OUTPUT_FILE_EXT_FILTER[idx]);
        ptr += len;
        *ptr = separator;
        ptr++;
        len = strlen(OUTPUT_FILE_EXT_FILTER[idx]);
        if (nSize - (ptr - filter) <= len + 1)
            return;
        memcpy(ptr, OUTPUT_FILE_EXT_FILTER[idx], sizeof(ptr[0]) * len);
        ptr += len;
        *ptr = separator;
        ptr++;
    };
    add_filter(TOP, separator);
    add_filter(OUTPUT_FILE_EXT_FILTER[default_index], ';');
    for (int idx = 0; idx < _countof(OUTPUT_FILE_EXT_FILTER); idx++)
        if (idx != default_index)
            add_filter(OUTPUT_FILE_EXT_FILTER[idx], ';');
    add_filter(OUTPUT_FILE_EXT_FILTER[default_index], separator);
    add_desc(default_index);
    for (int idx = 0; idx < _countof(OUTPUT_FILE_EXT_FILTER); idx++)
        if (idx != default_index)
            add_desc(idx);
    ptr[0] = '\0';
}
