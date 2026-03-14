/*
  This file is part of the DSP-Crowd project
  https://www.dsp-crowd.com

  Author(s):
      - Johannes Natter, office@dsp-crowd.com

  File created on 07.02.2023

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

#ifndef KEY_USER_H
#define KEY_USER_H

#define KEY_USER_DEBUG 0

#include <string>
#if KEY_USER_DEBUG
#include <iostream>
#endif
#include <cstdint>
#include <locale>
#include <codecvt>

typedef uint16_t CtrlKeyUser;

// Unicode

void strToUtf(const std::string &str, std::u32string &ustr);
void utfToStr(const std::u32string &ustr, std::string &str);
void utfStrAdd(std::u32string &ustr, const std::string &str);

class KeyUser
{

public:

	KeyUser()
		: mPrint()
		, mCtrl()
		, mIsPrint(false)
		, mIsCtrl(false)
		, mModShift(false)
		, mModAlt(false)
		, mModCtrl(false)
	{
	}

	KeyUser(char32_t key)
		: mPrint(key)
		, mCtrl()
		, mIsPrint(true)
		, mIsCtrl(false)
		, mModShift(false)
		, mModAlt(false)
		, mModCtrl(false)
	{
#if KEY_USER_DEBUG
		std::cout << "KeyUser(char32_t):        key    = " << key << std::endl;
		std::cout << "KeyUser(char32_t):        mPrint = " << mPrint << std::endl;
		std::cout << "KeyUser(char32_t):        mCtrl  = " << mCtrl << std::endl;
#endif
	}

	bool operator==(const char &other) const
	{
#if KEY_USER_DEBUG
		std::cout << "operator==(char):         mPrint = " << mPrint << ", other = " << other << std::endl;
#endif
		if (!mIsPrint) return false;
		return mPrint == char32_t(other);
	}

	bool operator!=(const char &other) const
	{
#if KEY_USER_DEBUG
		std::cout << "operator!=(char):         mPrint = " << mPrint << ", other = " << other << std::endl;
#endif
		if (!mIsPrint) return true;
		return mPrint != char32_t(other);
	}

	bool operator>(const char &other) const
	{
#if KEY_USER_DEBUG
		std::cout << "operator>(char):          mPrint = " << mPrint << ", other = " << other << std::endl;
#endif
		if (!mIsPrint) return false;
		return mPrint > char32_t(other);
	}

	bool operator>=(const char &other) const
	{
#if KEY_USER_DEBUG
		std::cout << "operator>=(char):         mPrint = " << mPrint << ", other = " << other << std::endl;
#endif
		if (!mIsPrint) return false;
		return mPrint >= char32_t(other);
	}

	bool operator<(const char &other) const
	{
#if KEY_USER_DEBUG
		std::cout << "operator<(char):          mPrint = " << mPrint << ", other = " << other << std::endl;
#endif
		if (!mIsPrint) return false;
		return mPrint < char32_t(other);
	}

	bool operator<=(const char &other) const
	{
#if KEY_USER_DEBUG
		std::cout << "operator<=(char):         mPrint = " << mPrint << ", other = " << other << std::endl;
#endif
		if (!mIsPrint) return false;
		return mPrint <= char32_t(other);
	}

	bool operator==(const char32_t &other) const
	{
#if KEY_USER_DEBUG
		std::cout << "operator==(char32_t):     mPrint = " << mPrint << ", other = " << other << std::endl;
#endif
		if (!mIsPrint) return false;
		return mPrint == other;
	}

	bool operator!=(const char32_t &other) const
	{
#if KEY_USER_DEBUG
		std::cout << "operator!=(char32_t):     mPrint = " << mPrint << ", other = " << other << std::endl;
#endif
		if (!mIsPrint) return true;
		return mPrint != other;
	}

	bool operator>(const char32_t &other) const
	{
#if KEY_USER_DEBUG
		std::cout << "operator>(char32_t):      mPrint = " << mPrint << ", other = " << other << std::endl;
#endif
		if (!mIsPrint) return false;
		return mPrint > other;
	}

	bool operator<(const char32_t &other) const
	{
#if KEY_USER_DEBUG
		std::cout << "operator<(char32_t):      mPrint = " << mPrint << ", other = " << other << std::endl;
#endif
		if (!mIsPrint) return false;
		return mPrint < other;
	}

	bool operator==(const CtrlKeyUser &other) const
	{
#if KEY_USER_DEBUG
		std::cout << "operator==(CtrlKeyUser):  mCtrl = " << mCtrl << ", other = " << other << std::endl;
#endif
		if (!mIsCtrl) return false;
		return mCtrl == other;
	}

	bool operator!=(const CtrlKeyUser &other) const
	{
#if KEY_USER_DEBUG
		std::cout << "operator!=(CtrlKeyUser):  mCtrl = " << mCtrl << ", other = " << other << std::endl;
#endif
		if (!mIsCtrl) return true;
		return mCtrl != other;
	}

	bool operator>(const CtrlKeyUser &other) const
	{
#if KEY_USER_DEBUG
		std::cout << "operator>(CtrlKeyUser):   mCtrl = " << mCtrl << ", other = " << other << std::endl;
#endif
		if (!mIsCtrl) return false;
		return mCtrl > other;
	}

	bool operator<(const CtrlKeyUser &other) const
	{
#if KEY_USER_DEBUG
		std::cout << "operator<(CtrlKeyUser):   mCtrl = " << mCtrl << ", other = " << other << std::endl;
#endif
		if (!mIsCtrl) return false;
		return mCtrl < other;
	}

	KeyUser &operator=(char key)
	{
		mPrint = (unsigned char)key;
		mIsPrint = true;
		mIsCtrl = false;
#if KEY_USER_DEBUG
		std::cout << "operator=(char):          mPrint = " << mPrint << std::endl;
#endif
		return *this;
	}

	KeyUser &operator=(char32_t key)
	{
		mPrint = key;
		mIsPrint = true;
		mIsCtrl = false;
#if KEY_USER_DEBUG
		std::cout << "operator=(char32_t):      mPrint = " << mPrint << std::endl;
#endif
		return *this;
	}

	operator char() const
	{
#if KEY_USER_DEBUG
		std::cout << "operator char():          mPrint = " << mPrint << std::endl;
#endif
		return mPrint;
	}

	operator char32_t() const
	{
#if KEY_USER_DEBUG
		std::cout << "operator char32_t():      mPrint = " << mPrint << std::endl;
#endif
		return mPrint;
	}

	void keyCtrlSet(const CtrlKeyUser &key)
	{
		mCtrl = key;
		mIsPrint = false;
		mIsCtrl = true;
#if KEY_USER_DEBUG
		std::cout << "keyCtrlSet(CtrlKeyUser):  key    = " << key << std::endl;
		std::cout << "keyCtrlSet(CtrlKeyUser):  mPrint = " << mPrint << std::endl;
		std::cout << "keyCtrlSet(CtrlKeyUser):  mCtrl  = " << mCtrl << std::endl;
#endif
	}

	std::string utf8() const
	{
		std::string s;

		if (mIsCtrl) return s;

		std::u32string ustr;
		ustr.push_back(mPrint);
		utfToStr(ustr, s);

		return s;
	}

	std::string str() const
	{
		std::string s;

		s.push_back(mModCtrl ? 'C' : '-');
		s.push_back(mModAlt ? 'A' : '-');
		s.push_back(mModShift ? 'S' : '-');
		s.push_back(' ');

		if (mIsPrint)
		{
			s += "P ";
			s += utf8();
		}

		if (mIsCtrl)
		{
			s += "C ";
			s += std::to_string(mCtrl);
		}

		return s;
	}

	char32_t val() const		{ return mPrint;	}
	CtrlKeyUser ctr() const		{ return mCtrl;	}

	bool isPrint() const		{ return mIsPrint;	}
	bool isCtrl() const			{ return mIsCtrl;	}

	bool modShift() const		{ return mModShift;	}
	void modShiftSet(bool mod)	{ mModShift = mod;	}

	bool modAlt() const			{ return mModAlt;	}
	void modAltSet(bool mod)		{ mModAlt = mod;	}

	bool modCtrl() const		{ return mModCtrl;	}
	void modCtrlSet(bool mod)	{ mModCtrl = mod;	}

private:

	/* member variables */

	char32_t mPrint;
	CtrlKeyUser mCtrl;

	bool mIsPrint;
	bool mIsCtrl;

	bool mModShift;
	bool mModAlt;
	bool mModCtrl;

};

const CtrlKeyUser keyCtrlA        = 0x01;
const CtrlKeyUser keyCtrlC        = 0x03;
const CtrlKeyUser keyCtrlD        = 0x04;
const CtrlKeyUser keyBackspaceWin = 0x08;
const CtrlKeyUser keyTab          = 0x09;
const CtrlKeyUser keyEnter        = 0x0D;
const CtrlKeyUser keyCtrlV        = 0x16;
const CtrlKeyUser keyCtrlX        = 0x18;
const CtrlKeyUser keyEsc          = 0x1B;
const CtrlKeyUser keyBackspace    = 0x7F;
const CtrlKeyUser keyHelp         = '?';
const CtrlKeyUser keyOpt          = '=';
const CtrlKeyUser keySpace        = ' ';

enum KeyExtensions : CtrlKeyUser
{
	keyUp = 1000,
	keyDown,
	keyLeft,
	keyRight,
	keyHome,
	keyInsert,
	keyDelete,
	keyEnd,
	keyPgUp,
	keyPgDn,
	keyF0,	keyF1,	keyF2,	keyF3,
	keyF4,	keyF5,	keyF6,	keyF7,
	keyF8,	keyF9,	keyF10,	keyF11,
	keyF12,	keyF13,	keyF14,	keyF15,
	keyF16,	keyF17,	keyF18,	keyF19,
	keyF20,
};

// Keychecks

typedef bool (*FctKeyCheck)(const KeyUser &key);

bool keyIsAlphaNum(const KeyUser &key);
bool keyIsNum(const KeyUser &key);
bool keyIsBoundary(const KeyUser &key);
bool keyIsCtrl(const KeyUser &key);
bool keyIsBackspace(const KeyUser &key);
bool keyIsUserDisconnect(const KeyUser &key);
bool keyIsCommon(const KeyUser &key);
bool keyIsAccept(const KeyUser &key);
bool keyIsAbort(const KeyUser &key);
bool keyIsHome(const KeyUser &key);
bool keyIsEnd(const KeyUser &key);
bool keyIsUp(const KeyUser &key);
bool keyIsDown(const KeyUser &key);
bool keyIsUpJump(const KeyUser &key);
bool keyIsDownJump(const KeyUser &key);

#endif

