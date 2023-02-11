/*
*******************************************************************************
\file apdu.h
\brief Smart card Application Protocol Data Unit
\project bee2 [cryptographic library]
\created 2022.10.31
\version 2022.11.02
\copyright The Bee2 authors
\license Licensed under the Apache License, Version 2.0 (see LICENSE.txt).
*******************************************************************************
*/

/*!
*******************************************************************************
\file apdu.h
\brief Команды и ответы смарт-карт
*******************************************************************************
*/

#ifndef __BEE2_APDU_H
#define __BEE2_APDU_H

#include "bee2/defs.h"

#ifdef __cplusplus
extern "C" {
#endif

/*!
*******************************************************************************
\file apdu.h

\section apdu-cmd Команды APDU

Ренализованы следующие правила (см. СТБ 34.101.79, п. 12.1):

1. Обязательными компонентами команды являются:
- СLA -- класс команды (1 октет);
- INS -- инструкция команды (1 октет),
- P1 и P2 -- параметры команды (1 октет каждый).

2. Необязательным для команды является компонент CDF (массив октетов), который
содержит данные команды. Если компонент CDF присутствует, то команда должна
также содержать необязательный компонент Lc, определяющий длину CDF.

3. Если при выполнении команды ожидается, что в ответе будет содержаться
компонент RDF (см. далее), то команда должна содержать необязательный
компонент Le, определяющий максимально возможную длину компонента RDF
в ожидаемом ответе.

4. При наличии в команде компонентов Lc и Le они могут быть представлены
в короткой или расширенной форме. Форма Le должна соответствовать форме Lc.

5. Короткая и расширенная формы определяются следующим образом:
- Lc в короткой форме состоит из одного октета, отличного от 0x00 и
  определяющего значения от 1 до 255;
- Lc в расширенной форме состоит из трех октетов, при этом первый октет равен
  0x00, а остальные два октета отличны от 0x0000 и определяют значения
  от 1 до 65535;
- Le в короткой форме состоит из одного октета, определяющего значения от 1 до
  256 (значению 256 соответствует 0x00);
- если компонент Lc присутствует в команде, то Le в расширенной форме состоит
  из двух октетов, которые определяют значения от 1 до 65536 (значению 65536
  соответствует 0x0000);
- если компонент Lc отсутствует в команде, то Le в расширенной форме состоит
  из трех октетов, при этом первый октет равен 0x00, а следующие два октета
  определяют значения от 1 до 65536 (значению 65536 соответствует 0x0000).

6. При кодировании длин используются соглашения big-endian.
*******************************************************************************
*/

/*!	\brief Команда APDU

	\remark Поле cdf имеет открытую длину, и поэтому длина всей команды
	варьируется.
*/

typedef struct {
	octet cla;			/*!< клвсс команды */
	octet ins;			/*!< инструкция команды */
	octet p1;			/*!< первый параметр команды */
	octet p2;			/*!< второй параметр команды */
	size_t rdf_len;		/*!< максимальная длина данных ответа */
	size_t cdf_len;		/*!< длина данных команды */
	octet cdf[];		/*!< данные команды */
} apdu_cmd_t;

/*!	\brief Корректная команда?

	Проверяется корректность команды cmd.
	\return Признак корректности.
*/
bool_t apduCmdIsValid(
	const apdu_cmd_t* cmd	/*!< [in] команда */
);

/*!	\brief Кодирование команды

	Определяется число октетов в коде команды cmd. Если apdu != 0, то код
	размещается по этому адресу.
	\pre Команда cmd корректна.
	\pre Если адрес apdu != 0, то по этому адресу зарезервировано
	apduCmdEnc(0, cmd) октетов.
	\remark Буферы apdu и cmd не пересекаются.
	\return Число октетов в коде или SIZE_MAX в случае ошибки.
*/
size_t apduCmdEnc(
	octet apdu[],			/*!< [out] код команды */
	const apdu_cmd_t* cmd	/*!< [in] команда */
);

/*!	\brief Декодирование команды

	Определяется длина буфера памяти для размещения команды, представленной
	кодом [count]apdu. Если cmd != 0, то команда размещается по этому адресу.
	\pre Буфер [count]apdu корректен.
	\pre Если адрес cmd != 0, то по этому адресу зарезервировано
	apduCmdDer(0, apdu, count) октетов.
	\pre Буферы apdu и cmd не пересекаются.
	\return Число октетов для размещения команды или SIZE_MAX в случае ошибки.
*/
size_t apduCmdDec(
	apdu_cmd_t* cmd,		/*!< [out] команда */
	const octet apdu[],		/*!< [in] код команды */
	size_t count			/*!< [in] длина apdu в октетах */
);

/*!
*******************************************************************************
\file apdu.h

\section apdu-resp Ответы APDU

Реализованы следующие правила (см. СТБ 34.101.79, п. 12.1):

1. Обязательными компонентами ответа являются статусы обработки команды
SW1 и SW2 (1 октет каждый).

2. Необязательным для ответа является компонент RDF (массив октетов), который
содержит данные ответа.
*******************************************************************************
*/

/*!	\brief Ответ APDU

	\remark Поле rdf имеет открытую длину, и поэтому длина всего ответа
	варьируется.
*/

typedef struct {
	octet sw1;			/*!< первый статус ответа */
	octet sw2;			/*!< второй статус ответа */
	size_t rdf_len;		/*!< длина данных ответа */
	octet rdf[];		/*!< данные ответа */
} apdu_resp_t;

/*!	\brief Корректный ответ?

	Проверяется корректность ответа resp.
	\return Признак корректности.
*/
bool_t apduRespIsValid(
	const apdu_resp_t* resp		/*!< [in] ответ */
);

/*!	\brief Кодирование ответа

	Определяется число октетов в коде ответа resp. Если apdu != 0, то код
	размещается по этому адресу.
	\pre Ответ resp корректен.
	\pre Если адрес apdu != 0, то по этому адресу зарезервировано
	apduRespEnc(0, resp) октетов.
	\remark Буферы apdu и resp не пересекаются.
	\return Число октетов в коде или SIZE_MAX в случае ошибки.
*/
size_t apduRespEnc(
	octet apdu[],				/*!< [out] код ответа */
	const apdu_resp_t* resp		/*!< [in] ответ */
);

/*!	\brief Декодирование ответа

	Определяется длина буфера памяти для размещения ответа, представленного
	кодом [count]apdu. Если resp != 0, то ответ размещается по этому адресу.
	\pre Буфер [count]apdu корректен.
	\pre Если адрес resp != 0, то по этому адресу зарезервировано
	apduRespDer(0, apdu, count) октетов.
	\remark Буферы apdu и resp не пересекаются.
	\return Число октетов для размещения ответа или SIZE_MAX в случае ошибки.
*/
size_t apduRespDec(
	apdu_resp_t* resp,			/*!< [out] ответ */
	const octet apdu[],			/*!< [in] код ответа */
	size_t count				/*!< [in] длина apdu в октетах */
);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __BEE2_APDU_H */
