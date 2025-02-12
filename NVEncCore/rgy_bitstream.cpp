﻿// -----------------------------------------------------------------------------------------
// QSVEnc/NVEnc by rigaya
// -----------------------------------------------------------------------------------------
// The MIT License
//
// Copyright (c) 2011-2016 rigaya
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
// --------------------------------------------------------------------------------------------

#include <regex>
#include "rgy_util.h"
#include "rgy_bitstream.h"

std::vector<uint8_t> unnal(const uint8_t *ptr, size_t len) {
    std::vector<uint8_t> data;
    data.reserve(len);
    data.push_back(ptr[0]);
    data.push_back(ptr[1]);
    for (size_t i = 2; i < len; i++) {
        if (ptr[i-2] == 0x00 && ptr[i-1] == 0x00 && ptr[i] == 0x03) {
            //skip
        } else {
            data.push_back(ptr[i]);
        }
    }
    return data;
}

static void to_nal(std::vector<uint8_t>& data) {
    for (auto it = data.begin(); it < data.end() - 2; it++) {
        if (*it == 0
            && *(it + 1) == 0
            && (*(it + 2) & (~(0x03))) == 0) {
            it = data.insert(it + 2, 0x03);
        }
    }
}

static void add_u16(std::vector<uint8_t>& data, uint16_t u16) {
    data.push_back((uint8_t)((u16 & 0xff00) >> 8));
    data.push_back((uint8_t)(u16 & 0x00ff));
}

static void add_u32(std::vector<uint8_t>& data, uint32_t u32) {
    data.push_back((uint8_t)((u32 & 0xff000000) >> 24));
    data.push_back((uint8_t)((u32 & 0x00ff0000) >> 16));
    data.push_back((uint8_t)((u32 & 0x0000ff00) >>  8));
    data.push_back((uint8_t)((u32 & 0x000000ff) >>  0));
}

HEVCHDRSeiPrm::HEVCHDRSeiPrm() : maxcll(-1), maxfall(-1), contentlight_set(false), masterdisplay(), masterdisplay_set(false), atcSei(RGY_TRANSFER_UNKNOWN) {
    memset(&masterdisplay, 0, sizeof(masterdisplay));
}

HEVCHDRSei::HEVCHDRSei() : prm() {
}

void HEVCHDRSei::set_maxcll(int maxcll, int maxfall) {
    prm.maxcll = maxcll;
    prm.maxfall = maxfall;
    prm.contentlight_set = true;
}

int HEVCHDRSei::parse_maxcll(std::string str_maxcll) {
    if (str_maxcll.length()) {
        std::regex re_maxcll(R"((\d+),(\d+))");
        std::smatch match_maxcll;
        if (!regex_search(str_maxcll, match_maxcll, re_maxcll) || match_maxcll.size() != 3) {
            return 1;
        }

        try {
            prm.maxcll = std::stoi(match_maxcll[1]);
            prm.maxfall = std::stoi(match_maxcll[2]);
            prm.contentlight_set = true;
        } catch (...) {
            return 1;
        }
    }
    return 0;
}

void HEVCHDRSei::set_masterdisplay(const int masterdisplay[10]) {
    for (int i = 0; i < 10; i++) {
        prm.masterdisplay[i] = masterdisplay[i];
    }
    prm.masterdisplay_set = true;
}

int HEVCHDRSei::parse_masterdisplay(std::string str_masterdisplay) {
    if (str_masterdisplay.length()) {
        std::regex re_masterdisplay(R"(G\((\d+),(\d+)\)B\((\d+),(\d+)\)R\((\d+),(\d+)\)WP\((\d+),(\d+)\)L\((\d+),(\d+)\))");
        std::smatch match_masterdisplay;
        if (!regex_search(str_masterdisplay, match_masterdisplay, re_masterdisplay) || match_masterdisplay.size() != 11) {
            return 1;
        }

        try {
            for (int i = 0; i < 10; i++) {
                prm.masterdisplay[i] = std::stoi(match_masterdisplay[i + 1]);
            }
            prm.masterdisplay_set = true;
        } catch (...) {
            return 1;
        }
    }
    return 0;
}

void HEVCHDRSei::set_atcsei(CspTransfer atcSei) {
    prm.atcSei = atcSei;
}

HEVCHDRSeiPrm HEVCHDRSei::getprm() const {
    return prm;
}
std::string HEVCHDRSei::print_masterdisplay() const {
    std::string str;
    if (prm.masterdisplay_set) {
        str += strsprintf("G(%f %f) B(%f %f) R(%f %f) WP(%f %f) L(%f %f)",
            (float)prm.masterdisplay[0] * (1.0f / 50000.0f),
            (float)prm.masterdisplay[1] * (1.0f / 50000.0f),
            (float)prm.masterdisplay[2] * (1.0f / 50000.0f),
            (float)prm.masterdisplay[3] * (1.0f / 50000.0f),
            (float)prm.masterdisplay[4] * (1.0f / 50000.0f),
            (float)prm.masterdisplay[5] * (1.0f / 50000.0f),
            (float)prm.masterdisplay[6] * (1.0f / 50000.0f),
            (float)prm.masterdisplay[7] * (1.0f / 50000.0f),
            (float)prm.masterdisplay[8] * (1.0f / 10000.0f),
            (float)prm.masterdisplay[9] * (1.0f / 10000.0f));
    }
    return str;
}

std::string HEVCHDRSei::print_maxcll() const {
    std::string str;
    if (prm.contentlight_set && prm.maxcll >= 0 && prm.maxfall >= 0) {
        str += strsprintf("%d/%d", prm.maxcll, prm.maxfall);
    }
    return str;
}

std::string HEVCHDRSei::print_atcsei() const {
    std::string str;
    if (prm.atcSei != RGY_TRANSFER_UNKNOWN) {
        str += tchar_to_string(get_cx_desc(list_transfer, prm.atcSei));
    }
    return str;
}

std::string HEVCHDRSei::print() const {
    std::string str = print_masterdisplay();
    std::string str1 = print_maxcll();
    std::string str2 = print_atcsei();
    if (str.length() > 0) {
        str = "Mastering Display: " + str + "\n";
    }
    if (str1.length() > 0) {
        str += "MaxCLL/MaxFALL: " + str1 + "\n";
    }
    if (str2.length() > 0) {
        str += "AtcSei: " + str2 + "\n";
    }
    return str;
}

std::vector<uint8_t> HEVCHDRSei::gen_nal(HEVCHDRSeiPrm prm_set) {
    prm = prm_set;
    return gen_nal();
}

std::vector<uint8_t> HEVCHDRSei::gen_nal() const {
    std::vector<uint8_t> data;
    data.reserve(128);

    auto data_maxcll = sei_maxcll();
    auto data_masterdisplay = sei_masterdisplay();
    auto data_atcsei = sei_atcsei();
    if (data_maxcll.size() == 0
        && data_masterdisplay.size() == 0
        && data_atcsei.size() == 0) {
        return data;
    }
    std::vector<uint8_t> header = { 0x00, 0x00, 0x00, 0x01 };
#if 0
    header.reserve(128);

    uint16_t u16 = 0x00;
    u16 |= (39 << 9) | 1;
    add_u16(data, u16);

    vector_cat(data, data_maxcll);
    vector_cat(data, data_masterdisplay);
    to_nal(data);

    vector_cat(header, data);
    header.push_back(0x80);
    return header;
#else
    if (data_maxcll.size() > 0) {
        std::vector<uint8_t> buf;
        uint16_t u16 = 0x00;
        u16 |= (NALU_HEVC_PREFIX_SEI << 9) | 1;
        add_u16(buf, u16);
        vector_cat(buf, data_maxcll);
        to_nal(buf);

        std::vector<uint8_t> nal_maxcll;
        nal_maxcll.reserve(128);
        vector_cat(nal_maxcll, header);
        vector_cat(nal_maxcll, buf);
        nal_maxcll.push_back(0x80);

        vector_cat(data, nal_maxcll);
    }

    if (data_masterdisplay.size() > 0) {
        std::vector<uint8_t> buf;
        uint16_t u16 = 0x00;
        u16 |= (NALU_HEVC_PREFIX_SEI << 9) | 1;
        add_u16(buf, u16);
        vector_cat(buf, data_masterdisplay);
        to_nal(buf);

        std::vector<uint8_t> nal_masterdisplay;
        nal_masterdisplay.reserve(128);
        vector_cat(nal_masterdisplay, header);
        vector_cat(nal_masterdisplay, buf);
        nal_masterdisplay.push_back(0x80);

        vector_cat(data, nal_masterdisplay);
    }

    if (data_atcsei.size() > 0) {
        std::vector<uint8_t> buf;
        uint16_t u16 = 0x00;
        u16 |= (NALU_HEVC_PREFIX_SEI << 9) | 1;
        add_u16(buf, u16);
        vector_cat(buf, data_atcsei);
        to_nal(buf);

        std::vector<uint8_t> nal_atcsei;
        nal_atcsei.reserve(128);
        vector_cat(nal_atcsei, header);
        vector_cat(nal_atcsei, buf);
        nal_atcsei.push_back(0x80);

        vector_cat(data, nal_atcsei);
    }
#endif
    return data;
}


std::vector<uint8_t> HEVCHDRSei::sei_maxcll() const {
    std::vector<uint8_t> data;
    data.reserve(256);
    if (prm.contentlight_set && prm.maxcll >= 0 && prm.maxfall >= 0) {
        data.push_back(CONTENT_LIGHT_LEVEL_INFO);
        data.push_back(4);
        add_u16(data, (uint16_t)prm.maxcll);
        add_u16(data, (uint16_t)prm.maxfall);
    }
    return data;
}

std::vector<uint8_t> HEVCHDRSei::sei_masterdisplay() const {
    std::vector<uint8_t> data;
    data.reserve(256);
    if (prm.masterdisplay_set) {
        data.push_back(MASTERING_DISPLAY_COLOUR_VOLUME);
        data.push_back(24);
        for (int i = 0; i < 8; i++) {
            add_u16(data, (uint16_t)prm.masterdisplay[i]);
        }
        add_u32(data, (uint32_t)prm.masterdisplay[8]);
        add_u32(data, (uint32_t)prm.masterdisplay[9]);
    }
    return data;
}

std::vector<uint8_t> HEVCHDRSei::sei_atcsei() const {
    std::vector<uint8_t> data;
    data.reserve(8);
    if (prm.atcSei != RGY_TRANSFER_UNKNOWN) {
        data.push_back(ALTERNATIVE_TRANSFER_CHARACTERISTICS);
        data.push_back(1);
        data.push_back((uint8_t)prm.atcSei);
    }
    return data;
}

DOVIRpu::DOVIRpu() : m_find_header(get_find_header_func()), m_filepath(), m_fp(nullptr, fclose), m_buffer(), m_datasize(0), m_dataoffset(0), m_count(0), m_rpus() {};
DOVIRpu::~DOVIRpu() { m_fp.reset(); };

const uint8_t DOVIRpu::rpu_header[4] = { 0, 0, 0, 1 };

const tstring& DOVIRpu::get_filepath() const {
    return m_filepath;
}

int DOVIRpu::init(const TCHAR *rpu_file) {
    m_filepath.clear();
    FILE *fp = NULL;
    if (_tfopen_s(&fp, rpu_file, _T("rb")) != 0) {
        return 1;
    }
    m_fp.reset(fp);
    m_filepath = rpu_file;

    m_buffer.resize(256 * 1024);
    return 0;
}

int DOVIRpu::fillBuffer() {
    int64_t bufRemain = m_buffer.size() - (m_dataoffset + m_datasize);
    if (bufRemain < 4) {
        if (m_dataoffset > 4) {
            memmove(m_buffer.data(), m_buffer.data() + m_dataoffset, m_datasize);
            m_dataoffset = 0;
        } else {
            m_buffer.resize(m_buffer.size() * 2);
        }
        bufRemain = m_buffer.size() - (m_dataoffset + m_datasize);
    }
    const auto bytes_read = (int)fread(m_buffer.data() + m_dataoffset + m_datasize, sizeof(uint8_t), bufRemain, m_fp.get());
    m_datasize += bytes_read;
    return bytes_read;
}

int DOVIRpu::get_next_rpu(std::vector<uint8_t>& bytes) {
    if (m_datasize <= 4) {
        if (fillBuffer() == 0) {
            return 1; //EOF
        }
    }
    if (memcmp(m_buffer.data() + m_dataoffset, &DOVIRpu::rpu_header, sizeof(DOVIRpu::rpu_header)) != 0) {
        return 1;
    }
    m_dataoffset += sizeof(DOVIRpu::rpu_header);
    m_datasize -= sizeof(DOVIRpu::rpu_header);

    int64_t next_size = 0;
    for (;;) {
        auto dataptr = m_buffer.data() + m_dataoffset;
        const auto pos = m_find_header(dataptr, m_datasize);
        if (pos >= 0) {
            const auto next_header = dataptr + pos;
            next_size = next_header - dataptr;
            break;
        }
        if (fillBuffer() == 0) { // EOF
            next_size = m_datasize;
            break;
        }
    }
    if (next_size <= 0) {
        return 1;
    }

    bytes.resize(next_size);
    const auto dataptr = m_buffer.data() + m_dataoffset;
    memcpy(bytes.data(), dataptr, next_size);
    m_dataoffset += next_size;
    m_datasize -= next_size;
    return 0;
}

int DOVIRpu::get_next_rpu(std::vector<uint8_t>& bytes, const int64_t id) {
    bytes.clear();
    for (; m_count <= id; m_count++) {
        std::vector<uint8_t> rpu;
        if (int ret = get_next_rpu(rpu); ret != 0) {
            return ret;
        }
        m_rpus[m_count] = rpu;
    }
    if (auto it = m_rpus.find(id); it != m_rpus.end()) {
        bytes = std::move(it->second);
        m_rpus.erase(it);
    } else {
        return 1;
    }
    return 0;
}

int DOVIRpu::get_next_rpu_nal(std::vector<uint8_t>& bytes, const int64_t id) {
    std::vector<uint8_t> rpu;
    if (int ret = get_next_rpu(rpu, id); ret != 0) {
        return ret;
    }
    //to_nal(rpu); // NALU_HEVC_UNSPECIFIEDの場合は不要
    if (rpu.back() == 0x00) { // 最後が0x00の場合
        rpu.push_back(0x03);
    }

    bytes.resize(sizeof(DOVIRpu::rpu_header));
    memcpy(bytes.data(), &DOVIRpu::rpu_header, sizeof(DOVIRpu::rpu_header));

    uint16_t u16 = 0x00;
    u16 |= (NALU_HEVC_UNSPECIFIED << 9) | 1;
    add_u16(bytes, u16);
    vector_cat(bytes, rpu);
    return 0;
}

static inline int64_t memmem_c(const void *data_, const int64_t data_size, const void *target_, const int64_t target_size) {
    const uint8_t *data = (const uint8_t *)data_;
    for (int64_t i = 0; i <= data_size - target_size; i++) {
        if (memcmp(data + i, target_, target_size) == 0) {
            return i;
        }
    }
    return -1;
}

const DOVIProfile *getDOVIProfile(const int id) {
    static const std::array<DOVIProfile, 4> DOVI_PROFILES = {
        DOVIProfile{ 50, true, true, true, VideoVUIInfo(1, RGY_PRIM_UNSPECIFIED, RGY_MATRIX_UNSPECIFIED, RGY_TRANSFER_UNSPECIFIED, 5, RGY_COLORRANGE_FULL,    RGY_CHROMALOC_UNSPECIFIED) },
        DOVIProfile{ 81, true, true, true, VideoVUIInfo(1, RGY_PRIM_BT2020,      RGY_MATRIX_BT2020_NCL,  RGY_TRANSFER_ST2084,      5, RGY_COLORRANGE_LIMITED, RGY_CHROMALOC_UNSPECIFIED) },
        DOVIProfile{ 82, true, true, true, VideoVUIInfo(1, RGY_PRIM_BT709,       RGY_MATRIX_BT709,       RGY_TRANSFER_BT709,       5, RGY_COLORRANGE_LIMITED, RGY_CHROMALOC_UNSPECIFIED) },
        DOVIProfile{ 84, true, true, true, VideoVUIInfo(1, RGY_PRIM_BT2020,      RGY_MATRIX_BT2020_NCL,  RGY_TRANSFER_ARIB_B67,    5, RGY_COLORRANGE_LIMITED, RGY_CHROMALOC_UNSPECIFIED) }
    };
    for (const auto& profile : DOVI_PROFILES) {
        if (profile.profile == id) {
            return &profile;
        }
    }
    return nullptr;
}

std::vector<nal_info> parse_nal_unit_h264_c(const uint8_t *data, size_t size) {
    std::vector<nal_info> nal_list;
    if (size >= 3) {
        static const uint8_t header[3] = { 0, 0, 1 };
        nal_info nal_start = { nullptr, 0, 0 };
        int64_t i = 0;
        for (;;) {
            const int64_t next = memmem_c((const void *)(data + i), size - i, (const void *)header, sizeof(header));
            if (next < 0) break;

            i += next;
            if (nal_start.ptr) {
                nal_list.push_back(nal_start);
            }
            nal_start.ptr = data + i - (i > 0 && data[i-1] == 0);
            nal_start.type = data[i + 3] & 0x1f;
            nal_start.size = data + size - nal_start.ptr;
            if (nal_list.size()) {
                auto prev = nal_list.end() - 1;
                prev->size = nal_start.ptr - prev->ptr;
            }
            i += 3;
        }
        if (nal_start.ptr) {
            nal_list.push_back(nal_start);
        }
    }
    return nal_list;
}

std::vector<nal_info> parse_nal_unit_hevc_c(const uint8_t *data, size_t size) {
    std::vector<nal_info> nal_list;
    if (size >= 3) {
        static const uint8_t header[3] = { 0, 0, 1 };
        nal_info nal_start = { nullptr, 0, 0 };
        int64_t i = 0;
        for (;;) {
            const int64_t next = memmem_c((const void *)(data + i), size - i, (const void *)header, sizeof(header));
            if (next < 0) break;

            i += next;
            if (nal_start.ptr) {
                nal_list.push_back(nal_start);
            }
            nal_start.ptr = data + i - (i > 0 && data[i - 1] == 0);
            nal_start.type = (data[i + 3] & 0x7f) >> 1;
            nal_start.size = data + size - nal_start.ptr;
            if (nal_list.size()) {
                auto prev = nal_list.end() - 1;
                prev->size = nal_start.ptr - prev->ptr;
            }
            i += 3;
        }
        if (nal_start.ptr) {
            nal_list.push_back(nal_start);
        }
    }
    return nal_list;
}

int64_t find_header_c(const uint8_t *data, size_t size) {
    return memmem_c(data, size, DOVIRpu::rpu_header, sizeof(DOVIRpu::rpu_header));
}

#include "rgy_simd.h"

decltype(parse_nal_unit_h264_c)* get_parse_nal_unit_h264_func() {
#if defined(_M_IX86) || defined(_M_X64) || defined(__x86_64)
    const auto simd = get_availableSIMD();
#if defined(_M_X64) || defined(__x86_64)
    if ((simd & RGY_SIMD::AVX512BW) == RGY_SIMD::AVX512BW) return parse_nal_unit_h264_avx512bw;
#endif
    if ((simd & RGY_SIMD::AVX2) == RGY_SIMD::AVX2) return parse_nal_unit_h264_avx2;
#endif
    return parse_nal_unit_h264_c;
}
decltype(parse_nal_unit_hevc_c)* get_parse_nal_unit_hevc_func() {
#if defined(_M_IX86) || defined(_M_X64) || defined(__x86_64)
    const auto simd = get_availableSIMD();
#if defined(_M_X64) || defined(__x86_64)
    if ((simd & RGY_SIMD::AVX512BW) == RGY_SIMD::AVX512BW) return parse_nal_unit_hevc_avx512bw;
#endif
    if ((simd & RGY_SIMD::AVX2) == RGY_SIMD::AVX2) return parse_nal_unit_hevc_avx2;
#endif
    return parse_nal_unit_hevc_c;
}

decltype(find_header_c)* get_find_header_func() {
#if defined(_M_IX86) || defined(_M_X64) || defined(__x86_64)
    const auto simd = get_availableSIMD();
#if defined(_M_X64) || defined(__x86_64)
    if ((simd & RGY_SIMD::AVX512BW) == RGY_SIMD::AVX512BW) return find_header_avx512bw;
#endif
    if ((simd & RGY_SIMD::AVX2) == RGY_SIMD::AVX2) return find_header_avx2;
#endif
    return find_header_c;
}
