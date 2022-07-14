/*
*******************************************************************************
\file tm.h
\brief Time and timers
\project bee2 [cryptographic library]
\created 2014.10.13
\version 2022.07.14
\license This program is released under the GNU General Public License 
version 3. See Copyright Notices in bee2/info.h.
*******************************************************************************
*/

/*!
*******************************************************************************
\file tm.h
\brief Время и таймеры
*******************************************************************************
*/

#ifndef __BEE2_TM_H
#define __BEE2_TM_H

#include <time.h>
#include "bee2/defs.h"

#ifdef __cplusplus
extern "C" {
#endif

/*!
*******************************************************************************
\file tm.h

\section tm-timer Таймер

Функции tmFreq(), tmTicks() отвечают за работу с наиболее точным таймером 
среди доступных. Показания выдаются в виде значения типа tm_ticks_t.

Наиболее точным таймером на платформах x86, x64 является регистр RDTSC, 
который обновляется с частотой процессора.

Таймер определяется в следующей очередности (до первого найденного):
--	на платформах x86, x64 использовать регистр RDTSC;
--	в среде Windows использовать функции QueryPerformance[Counter|Frequency]();
--	в среде Unix использовать функции clock_get[time|res]();
--	использовать функцию clock() и константу CLOCK_PER_SEC (см. time.h).

\typedef tm_ticks_t
\brief Число тактов таймера
*******************************************************************************
*/

#ifndef U64_SUPPORT
	typedef u32 tm_ticks_t;
#else 
	typedef u64 tm_ticks_t;
#endif

/*!	\brief Показания таймера

	Возвращается число тактов, выполненных наиболее точным из доступных 
	таймеров. Таймер начинает работу до первого вызова функции. Показания
	таймера монотонно увеличиваются вплоть до переполнения счетчика тактов.
	\return Число тактов или 0 в случае ошибки.
*/
tm_ticks_t tmTicks();

/*!	\brief Частота таймера

	Возвращается число тактов, которое выполняет в секунду наиболее точный из
	доступных таймеров.
	\warning Замер частоты может занять время (в пределах 0.1 секунды).
	\return Число тактов в секунду или 0 в случае ошибки.
*/
tm_ticks_t tmFreq();

/*!	\brief Скорость

	По числу reps экспериментов за ticks тактов определяется число 
	экспериментов, которое можно выполнить за 1 секунду.
	\warning Расчеты ведутся в dword. Может быть потеря точности, связанная
	с переполнением.
	\return Число экспериментов в секунду или SIZE_MAX в случае ошибки.
*/
size_t tmSpeed(
	size_t reps,			/*!< [in] число экспериментов */
	tm_ticks_t ticks		/*!< [in] число тактов */
);

/*!
*******************************************************************************
\file tm.h

\section tm-time Время

Системное время задается числом секунд, прошедших с полуночи 
01 января 1970 года (1970-01-01T00:00:00Z в формате ISO 8601).
Использованную шкалу времени прнято называть UNIX-время. 
Начало отсчета -- старт "эры UNIX" (Unix Epoch).

Отметка времени представляется типом tm_time_t. Этот тип повторяет системный 
тип time_t и наследует его неопределенность по разрядности и знаковости.

\warning 10 января 2004 счетчик секунд принял значение 2^30.
32-битовый счетчик исчерпает себя 19 января 2038 года.
*******************************************************************************
*/

/*!	\brief Время */
typedef time_t tm_time_t;

#define TIME_0 ((tm_time_t)0)
#define TIME_1 ((tm_time_t)1)
#define TIME_ERR ((tm_time_t)(TIME_0 - TIME_1))

/*!	\brief UNIX-время

	Возвращается число секунд, прошедших с момента 1970-01-01T00:00:00Z.
	\return Число секунд или TIME_ERR в случае ошибки.
*/
tm_time_t tmTime();

/*!	\brief Округленное UNIX-время

	Возвращается округленное UNIX-время (tmTime() - t0) / ts,
	где t0 --- базовая отметка времени (начало отсчета), ts -- шаг времени.
	\return Округленное UNIX-время или TIME_ERR в случае ошибки.
	\remark Ошибками считаются следующие ситуации: 
		ts == 0, tmTime() < t0.
	\remark Процедура округления соответствует RFC 6238.
*/
tm_time_t tmTimeRound(
	tm_time_t t0,		/*!< [in] начало отсчета */
	tm_time_t ts		/*!< [in] шаг времени */
);

/*!
*******************************************************************************
\file tm.h

\section tm-date Дата

Дата задается тройкой (год y, месяц m, день месяца d). Тройка кодируется либо
тремя беззнаковыми целыми либо 6 октетами по схеме YYMMDD. В последнем случае
каждый октет представляет один десятичный знак даты:
- первые два октета - год текущего (21-го века) века;
- следующие два октета - месяц;
- последние два октета - день месяцв.

Пример: {0x02, 0x02, 0x0, 0x7, 0x2, 0x9}.

Дата определяется с привязкой к текущему часовому поясу.

\remark Даты, в которых y < 1583, считаются некорректными (григорианский
календарь был введен в 1582 году).

\remark Даты в формате YYMMDD используются в CV-сертификатах (см. btok.h).
*******************************************************************************
*/

/*!	\brief Дата

	Определяются текущие (год y, месяц m, день месяца d). Каждый из указателей
	y, m, d может быть нулевым, и тогда соответствующее поле даты
	не возвращается.
	\pre Ненулевые указатели y, m, d корректны.
	\return TRUE, если дата успешно определена, и FALSE в противном	случае.
*/
bool_t tmDate(
	size_t* y,			/*!< [out] год */
	size_t* m,			/*!< [out] месяц */
	size_t* d			/*!< [out] день */
);

/*!	\brief Дата в формате YYMMDD

	Определяется текущая дата date в формате YYMMDD.
	\pre Указатель date корректен.
	\return TRUE, если дата успешно определена, и FALSE в противном	случае.
*/
bool_t tmDate2(
	octet date[6]		/*!< [out] дата */
);

/*!	\brief Корректная дата?

	Проверяется корректность даты (год y, месяц m, день месяца d).
	\return Признак корректности.
*/
bool_t tmDateIsValid(
	size_t y,			/*!< [in] год */
	size_t m,			/*!< [in] месяц */
	size_t d			/*!< [in] день */
);

/*!	\brief Корректная дата в формате YYMMDD?

	Проверяется корректность даты date в формате YYMMDD.
	\return Признак корректности.
	\remark Проверяется в том числе корректность указателя date.
*/
bool_t tmDateIsValid2(
	const octet date[6]		/*!< [in] дата */
);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __BEE2_TM_H */
