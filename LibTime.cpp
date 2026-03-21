/*
  This file is part of the DSP-Crowd project
  https://www.dsp-crowd.com

  Author(s):
      - Johannes Natter, office@dsp-crowd.com

  File created on 17.01.2023

  Copyright (C) 2023, Johannes Natter

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#include <sstream>
#include <iomanip>

#include "LibTime.h"

using namespace std;
using namespace chrono;

uint32_t millis()
{
	auto now = steady_clock::now();
	auto nowMs = time_point_cast<milliseconds>(now);
	return (uint32_t)nowMs.time_since_epoch().count();
}

string nowUtc()
{
	TimePoint tp = system_clock::now();
	string res = tpToStr(tp, "%Y-%m-%dT%H:%M:%S.");

	auto timeSinceEpoch = tp.time_since_epoch();
	auto sec = duration_cast<seconds>(timeSinceEpoch);
	auto nanoSec = duration_cast<nanoseconds>(timeSinceEpoch) - duration_cast<nanoseconds>(sec);

	stringstream ss;
	ss << setw(7) << setfill('0') << nanoSec.count() << "Z";

	res += ss.str();

	return res;
}

TimePoint nowTp()
{
	return system_clock::now();
}

string nowToStr(const char *pFmt)
{
	TimePoint tp = system_clock::now();

	return tpToStr(tp, pFmt);
}

// https://cplusplus.com/reference/ctime/strftime/
string tpToStr(const TimePoint &tp, const char *pFmt)
{
	time_t tt_t = system_clock::to_time_t(tp);
#if defined(_WIN32)
	tm vtm_t;
	tm *tm_t = &vtm_t;

	errno_t numErr = ::localtime_s(tm_t, &tt_t);
	if (numErr)
		return "";
#else
	const tm *tm_t = ::localtime(&tt_t);
#endif
	char timeBuf[32];
	size_t res;

	if (!pFmt)
		pFmt = "%d.%m.%y %H:%M:%S";

	timeBuf[0] = 0;
	res = strftime(timeBuf, sizeof(timeBuf), pFmt, tm_t);
	if (!res)
		return "";

	return timeBuf;
}

TimePoint strToTp(const string &str, const char *pFmt)
{
	TimePoint tp;
	tm tm = {};

	if (!pFmt)
		pFmt = "%d.%m.%y %H:%M:%S";

	stringstream ss(str);
	ss >> get_time(&tm, pFmt);

	if (ss.fail())
		return tp;

	tp = system_clock::from_time_t(mktime(&tm));

	return tp;
}

size_t tpDiffSec(const TimePoint &tpEnd, const TimePoint &tpStart)
{
	if (tpStart > tpEnd)
		return 0;

	return (size_t)(duration_cast<seconds>(tpEnd - tpStart).count());
}

size_t tpDiffMs(const TimePoint &tpEnd, const TimePoint &tpStart)
{
	if (tpStart > tpEnd)
		return 0;

	return (size_t)(duration_cast<milliseconds>(tpEnd - tpStart).count());
}

TimePoint utcToCet(const TimePoint &tpUtc)
{
	time_t tt_t = system_clock::to_time_t(tpUtc);
#if defined(_WIN32)
	tm vtm_t;
	tm *tm_t = &vtm_t;

	errno_t numErr = ::localtime_s(tm_t, &tt_t);
	if (numErr)
		return "";
#else
	const tm *tm_t = ::localtime(&tt_t);
#endif
	return tpUtc + hours(tm_t->tm_isdst ? 2 : 1);
}

