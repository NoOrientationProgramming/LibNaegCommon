/*
  This file is part of the DSP-Crowd project
  https://www.dsp-crowd.com

  Author(s):
      - Johannes Natter, office@dsp-crowd.com

  File created on 31.01.2023

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

#include "TextBox.h"

using namespace std;

#define dSizeWidthMin	4

TextBox::TextBox()
	: mWidth(10)
	, mLenMax(64)
	, mUstrCurrent()
	, mUstrWork()
	, mClipboard("")
	, mIdxFront()
	, mIdxBack()
	, mPasswordMode(false)
	, mNumbersOnly(false)
	, mDirty(false)
	, mFrameEnabled(true)
	, mPaddingEnabled(true)
	, mCursorShow(true)
{
	mIdxFront.winSet(mWidth);
	mIdxBack.winSet(mIdxFront.win());

	currentSet("");
}

/* configuration */

void TextBox::widthSet(uint16_t width)
{
	if (width < dSizeWidthMin)
		return;

	mWidth = width;

	if (mFrameEnabled)
		width -= 2;

	if (mPaddingEnabled)
		width -= 2;

	mIdxFront.winSet(width);
	mIdxBack.winSet(mIdxFront.win());
}

void TextBox::cursorBoundSet(uint32_t bnd)
{
	mIdxFront.cursorBoundSet(bnd);
	mIdxBack.cursorBoundSet(bnd);
}

void TextBox::focusSet(bool foc, bool accept)
{
	mFocus = foc;

	if (foc)
	{
		mUstrWork = mUstrCurrent;

		mIdxFront = mUstrWork.size() + 1;
		mIdxFront.cursorEndSet();

		mIdxBack = mIdxFront;
		return;
	}

	if (accept)
	{
		mUstrCurrent = mUstrWork;
		utfToStr(mUstrCurrent, mCurrent);
	}

	mIdxFront = mUstrCurrent.size() + 1;
	mIdxFront.reset();

	mIdxBack = mIdxFront;
}

void TextBox::selectionSet(uint16_t idxStart, uint16_t len)
{
	const u32string &str = mFocus ? mUstrWork : mUstrCurrent;
	uint16_t idxEnd = idxStart + len;
	uint16_t idxMax = str.size();
	uint16_t idx = 0;

	mIdxFront.reset();

	if (idxStart > idxMax) idxStart = idxMax;

	for (; idx < idxStart; ++idx)
		mIdxFront.inc();

	mIdxBack = mIdxFront;

	if (!len || idxEnd > idxMax) idxEnd = idxMax;

	for (; idx < idxEnd; ++idx)
		mIdxFront.inc();
}

void TextBox::frameEnabledSet(bool val)
{
	mFrameEnabled = val;
	widthSet(mWidth);
}

void TextBox::paddingEnabledSet(bool val)
{
	mPaddingEnabled = val;
	widthSet(mWidth);
}

void TextBox::cursorShow(bool val)
{
	mCursorShow = val;
}

/* input */

void TextBox::currentSet(const string &str)
{
	mCurrent = str;
	strToUtf(mCurrent, mUstrCurrent);

	if (mFocus)
		return;

	mIdxFront = mUstrCurrent.size() + 1;
	mIdxBack = mIdxFront.size();
}

void TextBox::ustrWorkSet(const u32string &ustr)
{
	mUstrWork = ustr;

	if (!mFocus)
		return;

	mIdxFront = mUstrWork.size() + 1;
	mIdxFront.cursorEndSet();

	mIdxBack = mIdxFront;
}

TextBox &TextBox::operator=(const string &str)
{
	currentSet(str);
	return *this;
}

void TextBox::paste(const string &str)
{
	if (!mFocus)
		currentSet(str);
	else
		selectionReplace(str);
}

/* input / output */

bool TextBox::keyProcess(const KeyUser &key, const char *pListKeysDisabled)
{
	(void)pListKeysDisabled;
	mDirty = false;

	if (key == keyCtrlX)
	{
		clipboardSet();

		if (!mFocus)
			mUstrCurrent.clear();
		else
			selectionReplace();

		return dirtySet();
	}

	if (key == keyCtrlC)
	{
		clipboardSet();
		return true; // processed but not dirty
	}

	if (!mFocus)
		return false;

	// Navigation

	if (key == keyCtrlA)
	{
		mDirty |= mIdxBack.reset();
		mDirty |= mIdxFront.cursorEndSet();
		return true;
	}

	// mIdxFront may change here
	if (navigate(key))
		return dirtySet();

	// Removal

	bool isSelection = mIdxFront.cursorAbs() !=
					mIdxBack.cursorAbs();

	if (keyIsBackspace(key))
	{
		if (isSelection)
		{
			selectionReplace();
			return dirtySet();
		}

		if (!mIdxFront.prevErase())
			return false;

		mUstrWork.erase(mIdxFront.cursorAbs(), 1);
		mIdxBack = mIdxFront;

		return dirtySet();
	}

	if (key == keyDelete)
	{
		if (isSelection)
		{
			selectionReplace();
			return dirtySet();
		}

		// TextBox behavior
		if (mIdxFront.cursorAbs() == mIdxFront.size() - 1)
			return false;

		// 1. Backup cursorAbs
		uint32_t cursorAbs = mIdxFront.cursorAbs();

		// 2. Change list
		if (!mIdxFront.currErase())
			return false;
		mIdxBack = mIdxFront;

		// 3. Change text
		mUstrWork.erase(cursorAbs, 1);

		return dirtySet();
	}

	// Insertion (printables only)

	if (!key.isPrint())
		return false;

	if (mNumbersOnly && (key < '0' || key > '9'))
		return false;

	if (isSelection)
	{
		selectionReplace(key.utf8());
		return dirtySet();
	}

	// TextBox behavior (full)
	if (mUstrWork.size() >= mLenMax)
		return false;

	// 1. Change text
	mUstrWork.insert(mIdxFront.cursorAbs(), 1, key);

	// 2. Change list
	mIdxFront.insert();
	mIdxBack = mIdxFront;

	return dirtySet();
}

/* output */

TextBox::operator string() const
{
	return current();
}

const u32string &TextBox::ustrWork() const
{
	return mUstrWork;
}

bool TextBox::dirty()
{
	bool bkupDirty = mDirty;
	mDirty = false;
	return bkupDirty;
}

bool TextBox::print(string &msg)
{
	u32string strIn = mFocus ? mUstrWork : mUstrCurrent;
	u32string ustrPrint;
	string strPrint;
	size_t idxAbs = mIdxFront.offset();
	size_t idxRel = 0;
	bool cursorPrint;
	char32_t ch;

	size_t idxAbsLow = listIdxLow().cursorAbs();
	size_t idxAbsHigh = listIdxHigh().cursorAbs();
	bool selectionPrint = mFocus && idxAbsLow != idxAbsHigh;
	bool isSelection;

	// extend tmp string
	strIn.push_back(' ');

	// Frame
	utfStrAdd(ustrPrint, mModifierFrame);

	if (mFrameEnabled)
		ustrPrint.push_back(mFocus ? '>' : '|');

	if (mPaddingEnabled)
		ustrPrint.push_back(' ');

	utfStrAdd(ustrPrint, "\033[0m");

	// Content init
	isSelection = selectionPrint && idxAbs >= idxAbsLow;
	utfStrAdd(ustrPrint,
		isSelection ? mModifierSelection : mModifierContent);

	for (; idxRel < mIdxFront.win(); ++idxRel, ++idxAbs)
	{
		if (!idxRel && mIdxFront.offset())
		{
			utfStrAdd(ustrPrint, "\u00AB");
			continue;
		}

		if (idxRel == mIdxFront.win() - 1 && !mIdxFront.endReached())
		{
			utfStrAdd(ustrPrint, "\u00BB");
			continue;
		}

		if (idxAbs >= strIn.size())
			ch = ' ';
		else
		if (mPasswordMode && idxAbs < strIn.size() - 1)
			ch = '*';
		else
			ch = strIn[idxAbs];

		if (selectionPrint && idxAbs == idxAbsLow)
			utfStrAdd(ustrPrint, mModifierSelection);

		if (selectionPrint && idxAbs == idxAbsHigh)
		{
			utfStrAdd(ustrPrint, "\033[0m");
			utfStrAdd(ustrPrint, mModifierContent); // restore after reset only
		}

		cursorPrint = mFocus &&
					mCursorShow &&
					idxAbs == mIdxFront.cursorAbs();

		if (cursorPrint)
			utfStrAdd(ustrPrint, "\033[4m");

		ustrPrint.push_back(ch);

		if (cursorPrint)
		{
			utfStrAdd(ustrPrint, "\033[0m");

			// restore after reset only
			isSelection = selectionPrint && idxAbs < idxAbsHigh;
			utfStrAdd(ustrPrint,
				isSelection ? mModifierSelection : mModifierContent);
		}
	}

	utfStrAdd(ustrPrint, "\033[0m");

	// Frame
	utfStrAdd(ustrPrint, mModifierFrame);

	if (mPaddingEnabled)
		ustrPrint.push_back(' ');

	if (mFrameEnabled)
		ustrPrint.push_back(mFocus ? '<' : '|');

	utfStrAdd(ustrPrint, "\033[0m");

	// Output
	utfToStr(ustrPrint, strPrint);
	msg += strPrint;

	return true;
}

string TextBox::clipboard(bool clear)
{
	if (!clear)
		return mClipboard;

	string strRet = mClipboard;
	mClipboard = "";

	return strRet;
}

size_t TextBox::sizeDisplayed() const
{
	if (mFocus)
		return mUstrWork.size();

	return mCurrent.size();
}

uint32_t TextBox::cursorAbsFront() const
{
	return mIdxFront.cursorAbs();
}

uint32_t TextBox::cursorAbsBack() const
{
	return mIdxBack.cursorAbs();
}

/* private */

void TextBox::clipboardSet()
{
	if (!mFocus)
	{
		mClipboard = mCurrent;
		return;
	}

	size_t idxAbsLow = listIdxLow().cursorAbs();
	size_t len = listIdxHigh().cursorAbs() - idxAbsLow;

	u32string ustr = mUstrWork.substr(idxAbsLow, len);
	utfToStr(ustr, mClipboard);
}

void TextBox::selectionReplace(const string &str)
{
	ListIdx &idxLow = listIdxLow();
	ListIdx &idxHigh = listIdxHigh();
	size_t i, len;

	// 1. clear
	len = idxHigh.cursorAbs() - idxLow.cursorAbs();
	mUstrWork.erase(idxLow.cursorAbs(), len);

	idxLow = mUstrWork.size() + 1;
	idxHigh = idxLow;

	// 2. insert
	u32string ustr;
	strToUtf(str, ustr);

	len = mUstrWork.size() + ustr.size();
	if (len > mLenMax)
		len = mLenMax;
	len -= mUstrWork.size();

	mUstrWork.insert(mIdxFront.cursorAbs(),
					ustr.substr(0, len));

	for (i = 0; i < len; ++i)
		mIdxFront.insert();
	mIdxBack = mIdxFront;
}

bool TextBox::navigate(const KeyUser &key)
{
	bool processed = false;
	bool changed = false;

	if (key == keyHome)
		processed = true, changed = mIdxFront.reset();
	else
	if (key == keyEnd)
		processed = true, changed = mIdxFront.cursorEndSet();
	else
	if ((key == keyLeft || key == keyRight) && key.modCtrl())
		processed = true, changed = cursorJump(key);
	else
	if (key == keyLeft || key == keyUp)
		processed = true, changed = mIdxFront.dec();
	else
	if (key == keyRight || key == keyDown)
		processed = true, changed = mIdxFront.inc();
	else
	if (key == keyPgUp || key == keyPgDn)
		processed = true, changed = mIdxFront.keyProcess(key);

	if (processed && !key.modShift())
	{
		mIdxBack = mIdxFront;
		changed = true;
	}

	return changed;
}

bool TextBox::cursorJump(const KeyUser &key)
{
	if (!mUstrWork.size())
		return false;

	bool isRight = key == keyRight;
	int direction = isRight ? 1 : -1;
	bool stateCursor = (direction + 1) >> 1;
	bool statePrev = !stateCursor;
	uint16_t idxAbs = mIdxFront.cursorAbs();
	uint16_t idxStop = stateCursor ? mUstrWork.size() : 0;
	bool changed = false;

	const char32_t *pCursor = mUstrWork.c_str() + idxAbs;
	const char32_t *pPrev = NULL;

	while (true)
	{
		if (idxAbs == idxStop)
			break;

		changed = true;

		if (isRight)
			mIdxFront.inc();
		else
			mIdxFront.dec();

		pCursor += direction;
		idxAbs += direction;

		if (idxAbs)
			pPrev = pCursor - 1;

		if (!pPrev)
			continue;

		if (keyIsBoundary(*pPrev) == statePrev &&
			keyIsBoundary(*pCursor) == stateCursor)
			break;
	}

	return changed;
}

ListIdx &TextBox::listIdxLow()
{
	return mIdxFront.cursorAbs() > mIdxBack.cursorAbs()
			? mIdxBack : mIdxFront;
}

ListIdx &TextBox::listIdxHigh()
{
	return mIdxFront.cursorAbs() > mIdxBack.cursorAbs()
			? mIdxFront : mIdxBack;
}

bool TextBox::dirtySet(bool dirty)
{
	mDirty = dirty;
	return mDirty;
}

