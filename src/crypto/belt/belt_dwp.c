/*
*******************************************************************************
\file belt_dwp.c
\brief STB 34.101.31 (belt): DWP (datawrap = data encryption + authentication)
\project bee2 [cryptographic library]
\author (C) Sergey Agievich [agievich@{bsu.by|gmail.com}]
\created 2012.12.18
\version 2020.03.20
\license This program is released under the GNU General Public License 
version 3. See Copyright Notices in bee2/info.h.
*******************************************************************************
*/

#include "bee2/core/blob.h"
#include "bee2/core/err.h"
#include "bee2/core/mem.h"
#include "bee2/core/util.h"
#include "bee2/crypto/belt.h"
#include "bee2/math/ww.h"
#include "belt_lcl.h"

/*
*******************************************************************************
Шифрование и имитозащита данных (DWP)
*******************************************************************************
*/

typedef struct
{
	belt_ctr_st ctr[1];		/*< состояние функций CTR */
	word r[W_OF_B(128)];	/*< переменная r */
	word t[W_OF_B(128)];	/*< переменная t */
	word len[W_OF_B(128)];	/*< обработано открытых || критических данных */
	octet block[16];		/*< блок данных */
	size_t filled;			/*< накоплено октетов в блоке */
	octet mac[8];			/*< имитовставка для StepV */
	octet stack[];			/*< стек умножения */
} belt_dwp_st;

size_t beltDWP_keep()
{
	return sizeof(belt_dwp_st) + beltPolyMul_deep();
}

void beltDWPStart(void* state, const octet key[], size_t len, 
	const octet iv[16])
{
	belt_dwp_st* s = (belt_dwp_st*)state;
	ASSERT(memIsDisjoint2(iv, 16, state, beltDWP_keep()));
	// настроить CTR
	beltCTRStart(s->ctr, key, len, iv);
	// установить r, s
	beltBlockCopy(s->r, s->ctr->ctr);
	beltBlockEncr2((u32*)s->r, s->ctr->key);
#if (OCTET_ORDER == BIG_ENDIAN && B_PER_W != 32)
	beltBlockRevU32(s->r);
	beltBlockRevW(s->r);
#endif
	wwFrom(s->t, beltH(), 16);
	// обнулить счетчики
	memSetZero(s->len, sizeof(s->len));
	s->filled = 0;
}

void beltDWPStepE(void* buf, size_t count, void* state)
{
	beltCTRStepE(buf, count, state);
}

void beltDWPStepI(const void* buf, size_t count, void* state)
{
	belt_dwp_st* s = (belt_dwp_st*)state;
	ASSERT(memIsDisjoint2(buf, count, state, beltDWP_keep()));
	// критические данные не обрабатывались?
	ASSERT(count == 0 || beltHalfBlockIsZero(s->len + W_OF_B(64)));
	// обновить длину
	beltHalfBlockAddBitSizeW(s->len, count);
	// есть накопленные данные?
	if (s->filled)
	{
		if (count < 16 - s->filled)
		{
			memCopy(s->block + s->filled, buf, count);
			s->filled += count;
			return;
		}
		memCopy(s->block + s->filled, buf, 16 - s->filled);
		count -= 16 - s->filled;
		buf = (const octet*)buf + 16 - s->filled;
#if (OCTET_ORDER == BIG_ENDIAN)
		beltBlockRevW(s->block);
#endif
		beltBlockXor2(s->t, s->block);
		beltPolyMul(s->t, s->t, s->r, s->stack);
		s->filled = 0;
	}
	// цикл по полным блокам
	while (count >= 16)
	{
		beltBlockCopy(s->block, buf);
#if (OCTET_ORDER == BIG_ENDIAN)
		beltBlockRevW(s->block);
#endif
		beltBlockXor2(s->t, s->block);
		beltPolyMul(s->t, s->t, s->r, s->stack);
		buf = (const octet*)buf + 16;
		count -= 16;
	}
	// неполный блок?
	if (count)
		memCopy(s->block, buf, s->filled = count);
}

void beltDWPStepA(const void* buf, size_t count, void* state)
{
	belt_dwp_st* s = (belt_dwp_st*)state;
	ASSERT(memIsDisjoint2(buf, count, state, beltDWP_keep()));
	// первый непустой фрагмент критических данных?
	// есть необработанные открытые данные?
	if (count && beltHalfBlockIsZero(s->len + W_OF_B(64)) && s->filled)
	{
		memSetZero(s->block + s->filled, 16 - s->filled);
#if (OCTET_ORDER == BIG_ENDIAN)
		beltBlockRevW(s->block);
#endif
		beltBlockXor2(s->t, s->block);
		beltPolyMul(s->t, s->t, s->r, s->stack);
		s->filled = 0;
	}
	// обновить длину
	beltHalfBlockAddBitSizeW(s->len + W_OF_B(64), count);
	// есть накопленные данные?
	if (s->filled)
	{
		if (count < 16 - s->filled)
		{
			memCopy(s->block + s->filled, buf, count);
			s->filled += count;
			return;
		}
		memCopy(s->block + s->filled, buf, 16 - s->filled);
		count -= 16 - s->filled;
		buf = (const octet*)buf + 16 - s->filled;
#if (OCTET_ORDER == BIG_ENDIAN)
		beltBlockRevW(s->block);
#endif
		beltBlockXor2(s->t, s->block);
		beltPolyMul(s->t, s->t, s->r, s->stack);
		s->filled = 0;
	}
	// цикл по полным блокам
	while (count >= 16)
	{
		beltBlockCopy(s->block, buf);
#if (OCTET_ORDER == BIG_ENDIAN)
		beltBlockRevW(s->block);
#endif
		beltBlockXor2(s->t, s->block);
		beltPolyMul(s->t, s->t, s->r, s->stack);
		buf = (const octet*)buf + 16;
		count -= 16;
	}
	// неполный блок?
	if (count)
		memCopy(s->block, buf, s->filled = count);
}

void beltDWPStepD(void* buf, size_t count, void* state)
{
	beltCTRStepD(buf, count, state);
}

static void beltDWPStepG_internal(void* state)
{
	belt_dwp_st* s = (belt_dwp_st*)state;
	ASSERT(memIsValid(state, beltDWP_keep()));
	// есть накопленные данные?
	if (s->filled)
	{
		memSetZero(s->block + s->filled, 16 - s->filled);
#if (OCTET_ORDER == BIG_ENDIAN)
		beltBlockRevW(s->block);
#endif
		beltBlockXor2(s->t, s->block);
		beltPolyMul(s->t, s->t, s->r, s->stack);
		s->filled = 0;
	}
	// обработать блок длины
	beltBlockXor2(s->t, s->len);
	beltPolyMul(s->t, s->t, s->r, s->stack);
#if (OCTET_ORDER == BIG_ENDIAN && B_PER_W != 32)
	beltBlockRevW(s->s);
	beltBlockRevU32(s->s);
#endif
	beltBlockEncr2((u32*)s->t, s->ctr->key);
}

void beltDWPStepG(octet mac[8], void* state)
{
	belt_dwp_st* s = (belt_dwp_st*)state;
	ASSERT(memIsValid(mac, 8));
	beltDWPStepG_internal(state);
	u32To(mac, 8, (u32*)s->t);
}

bool_t beltDWPStepV(const octet mac[8], void* state)
{
	belt_dwp_st* s = (belt_dwp_st*)state;
	ASSERT(memIsValid(mac, 8));
	beltDWPStepG_internal(state);
#if (OCTET_ORDER == BIG_ENDIAN)
	s->t[0] = u32Rev(s->t[0]);
	s->t[1] = u32Rev(s->t[1]);
#endif
	return memEq(mac, s->t, 8);
}

err_t beltDWPWrap(void* dest, octet mac[8], const void* src1, size_t count1,
	const void* src2, size_t count2, const octet key[], size_t len,
	const octet iv[16])
{
	void* state;
	// проверить входные данные
	if (len != 16 && len != 24 && len != 32 ||
		!memIsValid(src1, count1) ||
		!memIsValid(src2, count2) ||
		!memIsValid(key, len) ||
		!memIsValid(iv, 16) ||
		!memIsValid(dest, count1) ||
		!memIsValid(mac, 8))
		return ERR_BAD_INPUT;
	// создать состояние
	state = blobCreate(beltDWP_keep());
	if (state == 0)
		return ERR_OUTOFMEMORY;
	// установить защиту (I перед E из-за разрешенного пересечения src2 и dest)
	beltDWPStart(state, key, len, iv);
	beltDWPStepI(src2, count2, state);
	memMove(dest, src1, count1);
	beltDWPStepE(dest, count1, state);
	beltDWPStepA(dest, count1, state);
	beltDWPStepG(mac, state);
	// завершить
	blobClose(state);
	return ERR_OK;
}

err_t beltDWPUnwrap(void* dest, const void* src1, size_t count1,
	const void* src2, size_t count2, const octet mac[8], const octet key[],
	size_t len, const octet iv[16])
{
	void* state;
	// проверить входные данные
	if (len != 16 && len != 24 && len != 32 ||
		!memIsValid(src1, count1) ||
		!memIsValid(src2, count2) ||
		!memIsValid(mac, 8) ||
		!memIsValid(key, len) ||
		!memIsValid(iv, 16) ||
		!memIsValid(dest, count1))
		return ERR_BAD_INPUT;
	// создать состояние
	state = blobCreate(beltDWP_keep());
	if (state == 0)
		return ERR_OUTOFMEMORY;
	// снять защиту
	beltDWPStart(state, key, len, iv);
	beltDWPStepI(src2, count2, state);
	beltDWPStepA(src1, count1, state);
	if (!beltDWPStepV(mac, state))
	{
		blobClose(state);
		return ERR_BAD_MAC;
	}
	memMove(dest, src1, count1);
	beltDWPStepD(dest, count1, state);
	// завершить
	blobClose(state);
	return ERR_OK;
}
