#include "gtest/gtest.h"
#include "plusaes/plusaes.hpp"
#include <fstream>

using namespace plusaes::detail;

namespace {

void test_encrypt_decrypt_ecb(const std::string & data, const std::vector<unsigned char> key,
    const unsigned char * ok_encrypted, const bool padding) {

    const long encrypted_size = (padding) ? data.size() + (16 - data.size() % 16) : data.size();
    std::vector<unsigned char> encrypted(encrypted_size);
    plusaes::encrypt_ecb((unsigned char*)data.data(), data.size(), &key[0], (int)key.size(), &encrypted[0], encrypted.size(), padding);
    ASSERT_EQ(memcmp(&encrypted[0], ok_encrypted, encrypted.size()), 0);

    std::vector<unsigned char> decrypted(encrypted.size());
    unsigned long padded = 0;
    if (padding) {
        plusaes::decrypt_ecb(encrypted.data(), encrypted.size(), &key[0], (int)key.size(), &decrypted[0], decrypted.size(), &padded);
    }
    else {
        plusaes::decrypt_ecb(encrypted.data(), encrypted.size(), &key[0], (int)key.size(), &decrypted[0], decrypted.size(), 0);
    }

    const std::string s(decrypted.begin(), decrypted.end() - padded);
    ASSERT_EQ(data, s);
}

void test_encrypt_decrypt_cbc(const std::string & data, const std::vector<unsigned char> key,
    const unsigned char (* iv)[16],
    const unsigned char * ok_encrypted, const bool padding) {

    std::vector<unsigned char> encrypted(data.size() + (16 - data.size() % 16));
    plusaes::encrypt_cbc((unsigned char*)data.data(), data.size(), &key[0], (int)key.size(), iv, &encrypted[0], encrypted.size(), padding);
    ASSERT_EQ(memcmp(&encrypted[0], ok_encrypted, encrypted.size()), 0);

    std::vector<unsigned char> decrypted(encrypted.size());
    unsigned long padded = 0;
    plusaes::decrypt_cbc(&encrypted[0], encrypted.size(), &key[0], (int)key.size(), iv, &decrypted[0], decrypted.size(), &padded);

    const std::string s(decrypted.begin(), decrypted.end() - padded);
    ASSERT_EQ(data, s);
}

} // no namespace

TEST(AES, version) {
    ASSERT_EQ(plusaes::version(), 0x00000100);
}

TEST(AES, sbox) {
    for (int i = 0; i < 256; ++i) {
        const unsigned char v = kSbox[i];
        ASSERT_EQ(i, kInvSbox[v]);
    }
}

TEST(AES, rot_word) {
    ASSERT_EQ(rot_word(0x3c4fcf09), 0x093c4fcf);
    ASSERT_EQ(rot_word(0x05766c2a), 0x2a05766c);
    ASSERT_EQ(rot_word(0x7ff65973), 0x737ff659);
}

TEST(AES, sub_word) {
    ASSERT_EQ(sub_word(0x093c4fcf), 0x01eb848a);
    ASSERT_EQ(sub_word(0x2a05766c), 0xe56b3850);
    ASSERT_EQ(sub_word(0x737ff659), 0x8fd242cb);
}

TEST(AES, inv_sub_word) {
    ASSERT_EQ(inv_sub_word(0x00217f71), 0x527b6b2c);
    ASSERT_EQ(inv_sub_word(0x4a25bbb1), 0x5cc2fe56);
    ASSERT_EQ(inv_sub_word(0xea44ed01), 0xbb865309);
}

TEST(AES, shift_rows) {
    State s = {0x04b7ca63, 0x51d05309, 0xe7e060cd, 0x8ce170ba};
    const State ok_s = {0x8ce05363, 0x04e16009, 0x51b770cd, 0xe7d0caba};

    shift_rows(s);
    ASSERT_EQ(memcmp(s.w, ok_s.w, sizeof(s.w)), 0);
}

TEST(AES, inv_shift_rows) {
    State s = {0x6822d93b, 0x73fb74fc, 0xe0cb6757, 0x2d0e59c0};
    const State ok_s = {0x73cb593b, 0xe00ed9fc, 0x2d227457, 0x68fb67c0};

    inv_shift_rows(s);
    ASSERT_EQ(memcmp(s.w, ok_s.w, sizeof(s.w)), 0);
}

TEST(AES, mix_columns) {
    State s = {0xed08cc68, 0xbcd2bb0a, 0x55f52e64, 0x78e84a24};
    const State ok_s = {0xbd981e7a, 0x14d1b6ac, 0xdd44691a, 0x3e2deb06};

    mix_columns(s);
    ASSERT_EQ(memcmp(s.w, ok_s.w, sizeof(s.w)), 0);
}

TEST(AES, inv_mix_columns) {
    State s = {0x0e93ec88, 0xb6e4e7f5, 0xc9f432cc, 0x1494d206};
    const State ok_s = {0xccaec75c, 0x1972c8e3, 0x83efe54a, 0xc733a909};

    inv_mix_columns(s);
    ASSERT_EQ(memcmp(s.w, ok_s.w, sizeof(s.w)), 0);
}

TEST(AES, expand_key_128) {
    const unsigned char key[] = {0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6, 0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c};
    const unsigned char ok_keys[11][16] = {
        {0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6, 0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c},
        {0xa0, 0xfa, 0xfe, 0x17, 0x88, 0x54, 0x2c, 0xb1, 0x23, 0xa3, 0x39, 0x39, 0x2a, 0x6c, 0x76, 0x05},
        {0xf2, 0xc2, 0x95, 0xf2, 0x7a, 0x96, 0xb9, 0x43, 0x59, 0x35, 0x80, 0x7a, 0x73, 0x59, 0xf6, 0x7f},
        {0x3d, 0x80, 0x47, 0x7d, 0x47, 0x16, 0xfe, 0x3e, 0x1e, 0x23, 0x7e, 0x44, 0x6d, 0x7a, 0x88, 0x3b},
        {0xef, 0x44, 0xa5, 0x41, 0xa8, 0x52, 0x5b, 0x7f, 0xb6, 0x71, 0x25, 0x3b, 0xdb, 0x0b, 0xad, 0x00},
        {0xd4, 0xd1, 0xc6, 0xf8, 0x7c, 0x83, 0x9d, 0x87, 0xca, 0xf2, 0xb8, 0xbc, 0x11, 0xf9, 0x15, 0xbc},
        {0x6d, 0x88, 0xa3, 0x7a, 0x11, 0x0b, 0x3e, 0xfd, 0xdb, 0xf9, 0x86, 0x41, 0xca, 0x00, 0x93, 0xfd},
        {0x4e, 0x54, 0xf7, 0x0e, 0x5f, 0x5f, 0xc9, 0xf3, 0x84, 0xa6, 0x4f, 0xb2, 0x4e, 0xa6, 0xdc, 0x4f},
        {0xea, 0xd2, 0x73, 0x21, 0xb5, 0x8d, 0xba, 0xd2, 0x31, 0x2b, 0xf5, 0x60, 0x7f, 0x8d, 0x29, 0x2f},
        {0xac, 0x77, 0x66, 0xf3, 0x19, 0xfa, 0xdc, 0x21, 0x28, 0xd1, 0x29, 0x41, 0x57, 0x5c, 0x00, 0x6e},
        {0xd0, 0x14, 0xf9, 0xa8, 0xc9, 0xee, 0x25, 0x89, 0xe1, 0x3f, 0x0c, 0xc8, 0xb6, 0x63, 0x0c, 0xa6}
    };

    RoundKeys keys = expand_key(key, sizeof(key));

    ASSERT_EQ(keys.size(), 11);
    for (int i = 0; i < keys.size(); ++i) {
        ASSERT_EQ(memcmp(keys[i].w, ok_keys[i], 16), 0);
    }
}


TEST(AES, expand_key_192) {
    const unsigned char key[] = {0x8e, 0x73, 0xb0, 0xf7, 0xda, 0x0e, 0x64, 0x52, 0xc8, 0x10, 0xf3, 0x2b, 0x80, 0x90, 0x79, 0xe5, 0x62, 0xf8, 0xea, 0xd2, 0x52, 0x2c, 0x6b, 0x7b};
    const unsigned char ok_keys[13][16] = {
        {0x8e, 0x73, 0xb0, 0xf7, 0xda, 0x0e, 0x64, 0x52, 0xc8, 0x10, 0xf3, 0x2b, 0x80, 0x90, 0x79, 0xe5},
        {0x62, 0xf8, 0xea, 0xd2, 0x52, 0x2c, 0x6b, 0x7b, 0xfe, 0x0c, 0x91, 0xf7, 0x24, 0x02, 0xf5, 0xa5},
        {0xec, 0x12, 0x06, 0x8e, 0x6c, 0x82, 0x7f, 0x6b, 0x0e, 0x7a, 0x95, 0xb9, 0x5c, 0x56, 0xfe, 0xc2},
        {0x4d, 0xb7, 0xb4, 0xbd, 0x69, 0xb5, 0x41, 0x18, 0x85, 0xa7, 0x47, 0x96, 0xe9, 0x25, 0x38, 0xfd},
        {0xe7, 0x5f, 0xad, 0x44, 0xbb, 0x09, 0x53, 0x86, 0x48, 0x5a, 0xf0, 0x57, 0x21, 0xef, 0xb1, 0x4f},
        {0xa4, 0x48, 0xf6, 0xd9, 0x4d, 0x6d, 0xce, 0x24, 0xaa, 0x32, 0x63, 0x60, 0x11, 0x3b, 0x30, 0xe6},
        {0xa2, 0x5e, 0x7e, 0xd5, 0x83, 0xb1, 0xcf, 0x9a, 0x27, 0xf9, 0x39, 0x43, 0x6a, 0x94, 0xf7, 0x67},
        {0xc0, 0xa6, 0x94, 0x07, 0xd1, 0x9d, 0xa4, 0xe1, 0xec, 0x17, 0x86, 0xeb, 0x6f, 0xa6, 0x49, 0x71},
        {0x48, 0x5f, 0x70, 0x32, 0x22, 0xcb, 0x87, 0x55, 0xe2, 0x6d, 0x13, 0x52, 0x33, 0xf0, 0xb7, 0xb3},
        {0x40, 0xbe, 0xeb, 0x28, 0x2f, 0x18, 0xa2, 0x59, 0x67, 0x47, 0xd2, 0x6b, 0x45, 0x8c, 0x55, 0x3e},
        {0xa7, 0xe1, 0x46, 0x6c, 0x94, 0x11, 0xf1, 0xdf, 0x82, 0x1f, 0x75, 0x0a, 0xad, 0x07, 0xd7, 0x53},
        {0xca, 0x40, 0x05, 0x38, 0x8f, 0xcc, 0x50, 0x06, 0x28, 0x2d, 0x16, 0x6a, 0xbc, 0x3c, 0xe7, 0xb5},
        {0xe9, 0x8b, 0xa0, 0x6f, 0x44, 0x8c, 0x77, 0x3c, 0x8e, 0xcc, 0x72, 0x04, 0x01, 0x00, 0x22, 0x02}
    };

    RoundKeys keys = expand_key(key, sizeof(key));

    ASSERT_EQ(keys.size(), 13);
    for (int i = 0; i < keys.size(); ++i) {
        ASSERT_EQ(memcmp(keys[i].w, ok_keys[i], 16), 0);
    }
}

TEST(AES, expand_key_256) {
    const unsigned char key[] = {0x60, 0x3d, 0xeb, 0x10, 0x15, 0xca, 0x71, 0xbe, 0x2b, 0x73, 0xae, 0xf0, 0x85, 0x7d, 0x77, 0x81, 0x1f, 0x35, 0x2c, 0x07, 0x3b, 0x61, 0x08, 0xd7, 0x2d, 0x98, 0x10, 0xa3, 0x09, 0x14, 0xdf, 0xf4};
    const unsigned char ok_keys[15][16] = {
        {0x60, 0x3d, 0xeb, 0x10, 0x15, 0xca, 0x71, 0xbe, 0x2b, 0x73, 0xae, 0xf0, 0x85, 0x7d, 0x77, 0x81},
        {0x1f, 0x35, 0x2c, 0x07, 0x3b, 0x61, 0x08, 0xd7, 0x2d, 0x98, 0x10, 0xa3, 0x09, 0x14, 0xdf, 0xf4},
        {0x9b, 0xa3, 0x54, 0x11, 0x8e, 0x69, 0x25, 0xaf, 0xa5, 0x1a, 0x8b, 0x5f, 0x20, 0x67, 0xfc, 0xde},
        {0xa8, 0xb0, 0x9c, 0x1a, 0x93, 0xd1, 0x94, 0xcd, 0xbe, 0x49, 0x84, 0x6e, 0xb7, 0x5d, 0x5b, 0x9a},
        {0xd5, 0x9a, 0xec, 0xb8, 0x5b, 0xf3, 0xc9, 0x17, 0xfe, 0xe9, 0x42, 0x48, 0xde, 0x8e, 0xbe, 0x96},
        {0xb5, 0xa9, 0x32, 0x8a, 0x26, 0x78, 0xa6, 0x47, 0x98, 0x31, 0x22, 0x29, 0x2f, 0x6c, 0x79, 0xb3},
        {0x81, 0x2c, 0x81, 0xad, 0xda, 0xdf, 0x48, 0xba, 0x24, 0x36, 0x0a, 0xf2, 0xfa, 0xb8, 0xb4, 0x64},
        {0x98, 0xc5, 0xbf, 0xc9, 0xbe, 0xbd, 0x19, 0x8e, 0x26, 0x8c, 0x3b, 0xa7, 0x09, 0xe0, 0x42, 0x14},
        {0x68, 0x00, 0x7b, 0xac, 0xb2, 0xdf, 0x33, 0x16, 0x96, 0xe9, 0x39, 0xe4, 0x6c, 0x51, 0x8d, 0x80},
        {0xc8, 0x14, 0xe2, 0x04, 0x76, 0xa9, 0xfb, 0x8a, 0x50, 0x25, 0xc0, 0x2d, 0x59, 0xc5, 0x82, 0x39},
        {0xde, 0x13, 0x69, 0x67, 0x6c, 0xcc, 0x5a, 0x71, 0xfa, 0x25, 0x63, 0x95, 0x96, 0x74, 0xee, 0x15},
        {0x58, 0x86, 0xca, 0x5d, 0x2e, 0x2f, 0x31, 0xd7, 0x7e, 0x0a, 0xf1, 0xfa, 0x27, 0xcf, 0x73, 0xc3},
        {0x74, 0x9c, 0x47, 0xab, 0x18, 0x50, 0x1d, 0xda, 0xe2, 0x75, 0x7e, 0x4f, 0x74, 0x01, 0x90, 0x5a},
        {0xca, 0xfa, 0xaa, 0xe3, 0xe4, 0xd5, 0x9b, 0x34, 0x9a, 0xdf, 0x6a, 0xce, 0xbd, 0x10, 0x19, 0x0d},
        {0xfe, 0x48, 0x90, 0xd1, 0xe6, 0x18, 0x8d, 0x0b, 0x04, 0x6d, 0xf3, 0x44, 0x70, 0x6c, 0x63, 0x1e}
    };

    RoundKeys keys = expand_key(key, sizeof(key));

    ASSERT_EQ(keys.size(), 15);
    for (int i = 0; i < keys.size(); ++i) {
        ASSERT_EQ(memcmp(keys[i].w, ok_keys[i], 16), 0);
    }
}

TEST(AES, key_from_string_128) {
    const char key_str[] = "1234567890123456";
    std::vector<unsigned char> key = plusaes::key_from_string(&key_str);
    ASSERT_EQ(memcmp(&key[0], key_str, 16), 0);
}

TEST(AES, key_from_string_192) {
    const char key_str[] = "123456789012345678901234";
    std::vector<unsigned char> key = plusaes::key_from_string(&key_str);
    ASSERT_EQ(memcmp(&key[0], key_str, 24), 0);
}

TEST(AES, key_from_string_256) {
    const char key_str[] = "abcdefghijklMNOPQRSTUVwxYz789012";
    std::vector<unsigned char> key = plusaes::key_from_string(&key_str);
    ASSERT_EQ(memcmp(&key[0], key_str, 32), 0);
}

TEST(AES, encrypt_decrypt_ecb_128_key_not_mul_16) {
    // from OpenSSL
    const unsigned char ok_encrypted[] = {
        0xAF, 0xA4, 0xFD, 0xE8, 0x0D, 0xFA, 0xE7, 0x37, 0x46, 0x98, 0xED, 0xA4, 0xEE, 0x44, 0x26, 0x8B,
        0x3C, 0x11, 0x32, 0x73, 0xB3, 0xB3, 0xB0, 0x67, 0x55, 0x03, 0x0C, 0x9B, 0xF7, 0xDF, 0x3E, 0x56,
        0xA7, 0x57, 0x8F, 0xFF, 0x5F, 0xF8, 0x5D, 0x49, 0xE1, 0x0D, 0x17, 0xBF, 0x5A, 0x15, 0xDB, 0xE4,
        0x70, 0xA7, 0x1B, 0xB5, 0x41, 0x95, 0xE1, 0x95, 0xBF, 0xDE, 0x0B, 0xBC, 0x90, 0x55, 0x49, 0x7F,
        0x08, 0x81, 0x83, 0x0E, 0x16, 0x9F, 0xC2, 0xDC, 0xAA, 0x73, 0xB5, 0xF0, 0xCF, 0xDE, 0x7B, 0x09,
        0x2F, 0xE0, 0x78, 0x7D, 0xB3, 0x36, 0x74, 0x97, 0x2E, 0x49, 0x2A, 0x00, 0x52, 0x62, 0x73, 0x52,
        0xB2, 0x50, 0xDC, 0xFA, 0xD1, 0xA2, 0x8E, 0x41, 0x72, 0x30, 0xAE, 0x73, 0x55, 0x10, 0x90, 0x87,
        0xDC, 0xF6, 0x9D, 0xCD, 0x9A, 0x20, 0xF5, 0xD5, 0xFE, 0xD4, 0x10, 0x3E, 0x6C, 0xBA, 0x2B, 0x53,
        0x65, 0x0F, 0x25, 0x1B, 0x9E, 0x88, 0x43, 0x77, 0x8A, 0xFF, 0xB9, 0xAF, 0x8E, 0xA5, 0x05, 0xC3,
        0x9D, 0x7B, 0x07, 0xAA, 0x34, 0x05, 0x56, 0x01, 0xE8, 0xF7, 0xCC, 0x24, 0x23, 0x00, 0x7C, 0x32,
        0xF1, 0x8D, 0x8E, 0x2D, 0x5E, 0xF8, 0x63, 0xB0, 0x4B, 0x43, 0x59, 0xFA, 0xCC, 0x33, 0x0C, 0xFC,
        0xEF, 0x20, 0x0B, 0x46, 0x2A, 0x06, 0x15, 0xF5, 0x4A, 0x79, 0xB1, 0x8F, 0x40, 0x76, 0x67, 0x29,
        0x79, 0x96, 0x9A, 0x47, 0x76, 0x03, 0x59, 0x84, 0xB8, 0x34, 0x8C, 0x0E, 0xF5, 0xE0, 0x0F, 0x1F
    };

    const std::vector<unsigned char> key = plusaes::key_from_string(&"1234567890123456");

    const std::string data =
        "1,2,3,4,5,6,7,8,9,10\n"
        "11,12,13,14,15,16,17,18,19,20\n"
        "21,22,23,24,25,26,27,28,29,30\n"
        "abcd, efgh, ijkl, mnop, qrst, uvwx, yz\n"
        "ABCD, EFGH, IJKL, MNOP, QRST, UVWX, YZ\n"
        "!\"#$%&'()0=~|`{}+*_?><./_]:;@[¥^-]`'\"\n";

    test_encrypt_decrypt_ecb(data, key, ok_encrypted, true);
}

TEST(AES, encrypt_decrypt_ecb_192_key_mul_16) {
    const unsigned char ok_encrypted[] = {
        0x5E, 0xF1, 0x10, 0xFC, 0xB6, 0x22, 0xA4, 0xD3, 0x4B, 0xC9, 0x36, 0xCA, 0xA2, 0xE9, 0xF0, 0x55,
        0x9B, 0x28, 0xE8, 0x32, 0x15, 0x3B, 0x2B, 0x3E, 0x91, 0x1F, 0xCC, 0xCB, 0x5B, 0x95, 0xFA, 0x49,
        0x30, 0xE3, 0x1C, 0x65, 0xC9, 0x95, 0x88, 0xFA, 0x00, 0xB8, 0x1A, 0x0E, 0x94, 0x27, 0x51, 0x89
    };

    const std::vector<unsigned char> key = plusaes::key_from_string(&"12345678901234567890ABCD");

    const std::string data =
        "1234567890ABCDEF"
        "!#$%&'()0=~|`{ZZ";

    test_encrypt_decrypt_ecb(data, key, ok_encrypted, true);
    test_encrypt_decrypt_ecb(data, key, ok_encrypted, false);
}

TEST(AES, encrypt_decrypt_ecb_256_key_mul_16) {
    const unsigned char ok_encrypted[] = {
        0xAB, 0x60, 0x61, 0xA8, 0xA5, 0x99, 0x6A, 0x63, 0x48, 0x76, 0x93, 0xE5, 0xA2, 0xF6, 0xB0, 0x8C,
        0x0C, 0x98, 0x36, 0x93, 0x27, 0x8A, 0x3A, 0xD3, 0x51, 0xE5, 0xE1, 0x9E, 0x72, 0x82, 0x40, 0xEC,
        0xB2, 0x02, 0x89, 0x63, 0xF4, 0x23, 0xF6, 0x38, 0x2C, 0xCC, 0x68, 0xC0, 0xB6, 0xCA, 0x4A, 0x6D
    };

    const std::vector<unsigned char> key = plusaes::key_from_string(&"12345678901234567890ABCDEFGHIJKL");

    const std::string data =
        "1234567890ABCDEF"
        "!#$%&'()0=~|`{ZZ";

    test_encrypt_decrypt_ecb(data, key, ok_encrypted, true);
    test_encrypt_decrypt_ecb(data, key, ok_encrypted, false);
}

TEST(AES, encrypt_decrypt_cbc_128_key_not_mul_16_no_iv) {
    // from OpenSSL
    const unsigned char ok_encrypted[] = {
        0xAF, 0xA4, 0xFD, 0xE8, 0x0D, 0xFA, 0xE7, 0x37, 0x46, 0x98, 0xED, 0xA4, 0xEE, 0x44, 0x26, 0x8B,
        0xF7, 0xC4, 0x6A, 0x0F, 0xDD, 0x3C, 0xDF, 0xDE, 0x5D, 0xF4, 0xBA, 0x7F, 0xA8, 0x22, 0x24, 0x45,
        0x73, 0xD4, 0xA3, 0x24, 0x73, 0xF4, 0xD6, 0x90, 0xEE, 0xA2, 0x5E, 0xE6, 0xE0, 0x9F, 0xF5, 0x49,
        0x21, 0x66, 0x87, 0xC4, 0x37, 0xD0, 0xC0, 0x63, 0xF6, 0x1D, 0x88, 0x2B, 0x69, 0x4E, 0x50, 0x99,
        0x03, 0x24, 0x4C, 0x91, 0x75, 0x5B, 0xCF, 0x49, 0xB0, 0x8E, 0x52, 0xA1, 0x57, 0x82, 0xCC, 0xF8,
        0x8B, 0xB9, 0x4B, 0x07, 0x23, 0x0F, 0x33, 0x7C, 0x8A, 0x28, 0x1A, 0xB1, 0x05, 0x9D, 0x0E, 0xF3,
        0x9D, 0x10, 0x92, 0x1F, 0xF5, 0x46, 0x92, 0x59, 0x13, 0xA8, 0x10, 0x21, 0x3B, 0x6B, 0xBF, 0xD1,
        0xC3, 0xF9, 0x38, 0x89, 0x69, 0x9E, 0x96, 0x99, 0x2C, 0x11, 0xF4, 0x1E, 0xBA, 0x01, 0x62, 0xE0,
        0xEA, 0x6E, 0x45, 0xC7, 0x34, 0xAB, 0x8C, 0x3A, 0x51, 0x68, 0x73, 0xF1, 0x9C, 0xB2, 0xDB, 0xA7,
        0x87, 0xEE, 0x37, 0xD1, 0x7D, 0xA0, 0xAD, 0x9D, 0xF6, 0xE2, 0x02, 0x74, 0xBC, 0x96, 0x66, 0xC7,
        0x55, 0x7E, 0x89, 0x21, 0xB7, 0xE0, 0xA4, 0x9B, 0x77, 0x8B, 0x25, 0x67, 0x77, 0x15, 0x8C, 0x37,
        0xA2, 0x6C, 0x58, 0x1B, 0x78, 0x55, 0x04, 0x5E, 0x27, 0x15, 0x2A, 0x65, 0x14, 0x56, 0x12, 0x30,
        0xC3, 0x38, 0xF6, 0x3B, 0x32, 0x29, 0x01, 0x4D, 0x16, 0xEF, 0x27, 0x01, 0x75, 0xD0, 0x17, 0x25
    };

    const std::vector<unsigned char> key = plusaes::key_from_string(&"1234567890123456");

    const std::string data =
    "1,2,3,4,5,6,7,8,9,10\n"
    "11,12,13,14,15,16,17,18,19,20\n"
    "21,22,23,24,25,26,27,28,29,30\n"
    "abcd, efgh, ijkl, mnop, qrst, uvwx, yz\n"
    "ABCD, EFGH, IJKL, MNOP, QRST, UVWX, YZ\n"
    "!\"#$%&'()0=~|`{}+*_?><./_]:;@[¥^-]`'\"\n";

    test_encrypt_decrypt_cbc(data, key, 0, ok_encrypted, true);
}
