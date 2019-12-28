/*
 * Interface to the EFM32 cryptographic engine
 */
#include <stdio.h>
#include "py/nlr.h"
#include "py/obj.h"
#include "py/runtime.h"
#include "py/binary.h"
#include "py/objarray.h"
#include "em_gpio.h"
#include "em_cmu.h"
#include "em_core.h"

#include "em_crypto.h"

/*
 * Convert an encryption key to a decryption key
 */
static mp_obj_t mp_aes128_decryptkey(mp_obj_t key_in_obj)
{
	mp_buffer_info_t key_in;
	mp_get_buffer_raise(key_in_obj, &key_in, MP_BUFFER_READ);

	if (key_in.len != 16)
		mp_raise_ValueError("key must be 16 bytes");

	uint8_t key_out[16];
	CRYPTO_AES_DecryptKey128(CRYPTO,
		key_out,
		key_in.buf
	);

	return mp_obj_new_bytes(key_out, sizeof(key_out));
}
MP_DEFINE_CONST_FUN_OBJ_1(mp_aes128_decryptkey_obj, mp_aes128_decryptkey);

/*
 * Perform an AES128 ECB encryption operation
 */
static mp_obj_t mp_aes128ecb_encrypt(mp_obj_t key_obj, mp_obj_t in_obj, mp_obj_t out_obj)
{
	mp_buffer_info_t key;
	mp_get_buffer_raise(key_obj, &key, MP_BUFFER_READ);
	if (key.len != 16)
		mp_raise_ValueError("key must be 16 bytes");

	mp_buffer_info_t in;
	mp_get_buffer_raise(in_obj, &in, MP_BUFFER_READ);

	mp_buffer_info_t out;
	mp_get_buffer_raise(out_obj, &out, MP_BUFFER_WRITE);

	if ((in.len & 0xF) != 0)
		mp_raise_ValueError("in buffer must be multiple of 16 bytes");
	if (in.len > out.len)
		mp_raise_ValueError("out buffer must be same size as in");

	CRYPTO_AES_ECB128(
		CRYPTO,
		out.buf, // out
		in.buf, // in
		in.len, // len
		key.buf,
		1 // encrypt
	);

	CRYPTO_InstructionSequenceWait(CRYPTO);

	return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_3(mp_aes128ecb_encrypt_obj, mp_aes128ecb_encrypt);

/*
 * Perform an AES128 ECB decryption operation
 */
static mp_obj_t mp_aes128ecb_decrypt(mp_obj_t key_obj, mp_obj_t in_obj, mp_obj_t out_obj)
{
	mp_buffer_info_t key;
	mp_get_buffer_raise(key_obj, &key, MP_BUFFER_READ);
	if (key.len != 16)
		mp_raise_ValueError("key must be 16 bytes");

	mp_buffer_info_t in;
	mp_get_buffer_raise(in_obj, &in, MP_BUFFER_READ);

	mp_buffer_info_t out;
	mp_get_buffer_raise(out_obj, &out, MP_BUFFER_WRITE);

	if ((in.len & 0xF) != 0)
		mp_raise_ValueError("in buffer must be multiple of 16 bytes");
	if (in.len > out.len)
		mp_raise_ValueError("out buffer must be same size as in");

	CRYPTO_AES_ECB128(
		CRYPTO,
		out.buf, // out
		in.buf, // in
		in.len, // len
		key.buf,
		0 // decrypt
	);

	CRYPTO_InstructionSequenceWait(CRYPTO);

	return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_3(mp_aes128ecb_decrypt_obj, mp_aes128ecb_decrypt);


/*
 * Perform an AES128 counter mode operation
 */
static mp_obj_t mp_aes128_ctr(mp_obj_t key_obj, mp_obj_t buf_obj, mp_obj_t ctr_obj)
{
	mp_buffer_info_t buf;
	mp_get_buffer_raise(buf_obj, &buf, MP_BUFFER_WRITE);

	mp_buffer_info_t key;
	mp_get_buffer_raise(key_obj, &key, MP_BUFFER_READ);

	mp_buffer_info_t ctr;
	mp_get_buffer_raise(ctr_obj, &ctr, MP_BUFFER_WRITE);

	if (key.len != 16)
		mp_raise_ValueError("key must be 16 bytes");
	if (ctr.len != 16)
		mp_raise_ValueError("ctr must be 16 bytes");

	CRYPTO_AES_CTR128(
		CRYPTO,
		buf.buf, // out
		buf.buf, // in
		buf.len, // len
		key.buf,
		ctr.buf,
		NULL // ctrFunc is unused
	);

	return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_3(mp_aes128_ctr_obj, mp_aes128_ctr);


#if 0
/*
 * Perform an AES128 Cipher text chaining mode
 */
static mp_obj_t mp_aes128_cbc(mp_obj_t key_obj, mp_obj_t buf_obj, mp_obj_t iv_obj, mp_obj_t encrypt_obj)
{
	mp_buffer_info_t buf;
	mp_get_buffer_raise(buf_obj, &buf, MP_BUFFER_WRITE);

	mp_buffer_info_t key;
	mp_get_buffer_raise(key_obj, &key, MP_BUFFER_READ);

	mp_buffer_info_t iv;
	mp_get_buffer_raise(iv_obj, &iv, MP_BUFFER_WRITE);

	const int encrypt = mp_obj_get_int(encrypt_obj);

	if (key.len != 16)
		mp_raise_ValueError("key must be 16 bytes");
	if (iv.len != 16)
		mp_raise_ValueError("ctr must be 16 bytes");

	CRYPTO_AES_CBC128(
		CRYPTO,
		buf.buf, // out
		buf.buf, // in
		buf.len, // should be 16
		key.buf,
		iv.buf,
		encrypt
	);

	return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_4(mp_aes128_cbc_obj, mp_aes128_cbc);
#endif

STATIC const mp_map_elem_t mp_crypto_globals_table[] = {
    { MP_OBJ_NEW_QSTR(MP_QSTR___name__), MP_OBJ_NEW_QSTR(MP_QSTR_crypto) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_aes128decryptkey), (mp_obj_t) &mp_aes128_decryptkey_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_aes128ecb_encrypt), (mp_obj_t) &mp_aes128ecb_encrypt_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_aes128ecb_decrypt), (mp_obj_t) &mp_aes128ecb_decrypt_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_aes128ctr), (mp_obj_t) &mp_aes128_ctr_obj },
    //{ MP_OBJ_NEW_QSTR(MP_QSTR_aes128cbc), (mp_obj_t) &mp_aes128_cbc_obj },
};

STATIC MP_DEFINE_CONST_DICT (
    mp_crypto_globals,
    mp_crypto_globals_table
);

const mp_obj_module_t mp_module_crypto = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&mp_crypto_globals,
};
