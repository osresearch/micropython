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

#define AES128_BLOCK_LEN	16
#define AES256_BLOCK_LEN	32

/*
 * Convert an AES128/256 encryption key to a decryption key
 */
static mp_obj_t mp_aes_decryptkey(mp_obj_t key_in_obj)
{
	mp_buffer_info_t key_in;
	mp_get_buffer_raise(key_in_obj, &key_in, MP_BUFFER_READ);

	if (key_in.len == AES128_BLOCK_LEN)
	{
		uint8_t key_out[AES128_BLOCK_LEN];
		CRYPTO_AES_DecryptKey128(CRYPTO,
			key_out,
			key_in.buf
		);
		return mp_obj_new_bytes(key_out, sizeof(key_out));
	} else
	if (key_in.len == AES256_BLOCK_LEN)
	{
		uint8_t key_out[AES256_BLOCK_LEN];
		CRYPTO_AES_DecryptKey256(CRYPTO,
			key_out,
			key_in.buf
		);
		return mp_obj_new_bytes(key_out, sizeof(key_out));
	} else
		mp_raise_ValueError("key must be 16 or 32 bytes");

}
MP_DEFINE_CONST_FUN_OBJ_1(mp_aes_decryptkey_obj, mp_aes_decryptkey);

/*
 * Perform an AES128/256 ECB encryption operation
 */
static mp_obj_t _mp_aes_ecb(mp_obj_t key_obj, mp_obj_t in_obj, mp_obj_t out_obj, int encrypt)
{
	mp_buffer_info_t key;
	mp_get_buffer_raise(key_obj, &key, MP_BUFFER_READ);
	mp_buffer_info_t in;
	mp_get_buffer_raise(in_obj, &in, MP_BUFFER_READ);
	mp_buffer_info_t out;
	mp_get_buffer_raise(out_obj, &out, MP_BUFFER_WRITE);

	if (in.len > out.len)
		mp_raise_ValueError("out buffer must be same size as in");

	if (key.len == AES128_BLOCK_LEN)
	{
		if ((in.len & (AES128_BLOCK_LEN-1)) != 0)
			mp_raise_ValueError("in buffer must be multiple of 16 bytes");

		CRYPTO_AES_ECB128(
			CRYPTO,
			out.buf, // out
			in.buf, // in
			in.len, // len
			key.buf,
			encrypt
		);
	} else
	if (key.len == AES256_BLOCK_LEN)
	{
		if ((in.len & (AES256_BLOCK_LEN-1)) != 0)
			mp_raise_ValueError("in buffer must be multiple of 32 bytes");

		CRYPTO_AES_ECB256(
			CRYPTO,
			out.buf, // out
			in.buf, // in
			in.len, // len
			key.buf,
			encrypt
		);
	} else
		mp_raise_ValueError("key must be 16 or 32 bytes");

	CRYPTO_InstructionSequenceWait(CRYPTO);

	return mp_const_none;
}

static mp_obj_t mp_aes_ecb_encrypt(mp_obj_t key_obj, mp_obj_t in_obj, mp_obj_t out_obj)
{
	return _mp_aes_ecb(key_obj, in_obj, out_obj, 1);
}

MP_DEFINE_CONST_FUN_OBJ_3(mp_aes_ecb_encrypt_obj, mp_aes_ecb_encrypt);

static mp_obj_t mp_aes_ecb_decrypt(mp_obj_t key_obj, mp_obj_t in_obj, mp_obj_t out_obj)
{
	return _mp_aes_ecb(key_obj, in_obj, out_obj, 0);
}

MP_DEFINE_CONST_FUN_OBJ_3(mp_aes_ecb_decrypt_obj, mp_aes_ecb_decrypt);


STATIC const mp_map_elem_t mp_crypto_globals_table[] = {
    { MP_OBJ_NEW_QSTR(MP_QSTR___name__), MP_OBJ_NEW_QSTR(MP_QSTR_crypto) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_aes_decryptkey), (mp_obj_t) &mp_aes_decryptkey_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_aes_ecb_encrypt), (mp_obj_t) &mp_aes_ecb_encrypt_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_aes_ecb_decrypt), (mp_obj_t) &mp_aes_ecb_decrypt_obj },
    //{ MP_OBJ_NEW_QSTR(MP_QSTR_aes128ctr), (mp_obj_t) &mp_aes128_ctr_obj },
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
