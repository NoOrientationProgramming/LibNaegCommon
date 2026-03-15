/*
  This file is part of the DSP-Crowd project
  https://www.dsp-crowd.com

  Author(s):
      - Johannes Natter, office@dsp-crowd.com

  File created on 01.02.2024

  Copyright (C) 2024, Johannes Natter

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

#include "ThreadPooling.h"

#define dForEach_ProcState(gen) \
		gen(StStart) \
		gen(StBrokerStart) \
		gen(StBrokerMain) \
		gen(StInternalStart) \
		gen(StInternalMain) \

#define dGenProcStateEnum(s) s,
dProcessStateEnum(ProcState);

#define dForEach_SdState(gen) \
		gen(StSdStart) \
		gen(StBrokerSdStart) \
		gen(StInternalSdStart) \
		gen(StInternalSdMain) \

#define dGenSdStateEnum(s) s,
dProcessStateEnum(SdState);

#if 0
#define dGenProcStateString(s) #s,
dProcessStateStr(ProcState);
#define dGenSdStateString(s) #s,
dProcessStateStr(SdState);
#endif

using namespace std;

mutex ThreadPooling::mtxBroker;
bool ThreadPooling::brokerPresent = false;
Pipe<PoolRequest> ThreadPooling::ppPoolRequests;

ThreadPooling::ThreadPooling()
	: Processing("ThreadPooling")
	, mStateSd(StSdStart)
	, mCntInternals(1)
	, mpFctDriverCreate(NULL)
	, mIsInternal(false)
	, mNumProcessing(0)
	, mNumFinished(0)
	, mMtxBrokerInternal()
{
	mState = StStart;
}

/* member functions */

void ThreadPooling::cntWorkerSet(uint16_t cnt)
{
	mCntInternals = cnt;
}

void ThreadPooling::driverCreateSet(FuncDriverPoolCreate pFctDriverCreate)
{
	mpFctDriverCreate = pFctDriverCreate;
}

Success ThreadPooling::process()
{
	//Success success;
#if 0
	dStateTrace;
#endif
	switch (mState)
	{
	case StStart:

		{
			Guard lock(mtxBroker);

			if (!mIsInternal && brokerPresent)
			{
				procDbgLog("thread pool running already");
				return -1;
			}

			if (!mIsInternal)
				brokerPresent = true;
		}

		if (mIsInternal)
		{
			mState = StInternalStart;
			break;
		}

		mState = StBrokerStart;

		break;
	case StBrokerStart:

		if (!mCntInternals)
			return procErrLog(-1, "no workers configured");

		mVecInternals.reserve(mCntInternals);

		for (uint16_t i = 0; i < mCntInternals; ++i)
		{
			ThreadPooling *pInternal;

			pInternal = ThreadPooling::create();
			if (!pInternal)
				return procErrLog(-1, "could not create thread pool worker");

			pInternal->mIsInternal = true;
			mVecInternals.push_back(pInternal);

			if (mpFctDriverCreate)
			{
				start(pInternal, DrivenByExternalDriver);
				mpFctDriverCreate(pInternal, i);
			}
			else
				start(pInternal, DrivenByNewInternalDriver);
		}

		mState = StBrokerMain;

		break;
	case StBrokerMain:

		poolRequestsProcess();

		break;
	case StInternalStart:

		mState = StInternalMain;

		break;
	case StInternalMain:

		procsDrive();

		break;
	default:
		break;
	}

	return Pending;
}

Success ThreadPooling::shutdown()
{
	uint16_t i;

	switch (mStateSd)
	{
	case StSdStart:

		if (mIsInternal)
		{
			mStateSd = StInternalSdStart;
			break;
		}

		for (i = 0; i < mVecInternals.size(); ++i)
			cancel(mVecInternals[i]);

		mStateSd = StBrokerSdStart;

		break;
	case StBrokerSdStart:

		for (i = 0; i < mVecInternals.size(); ++i)
		{
			if (!mVecInternals[i]->shutdownDone())
				return Pending;
		}

		return Positive;

		break;
	case StInternalSdStart:

		procsDrive();

		if (mListProcs.size())
		{
			//procDbgLog("driving not finished");

			mStateSd = StInternalSdMain;
			break;
		}

		return Positive;

		break;
	case StInternalSdMain:

		procsDrive();

		if (mListProcs.size())
			break;

		return Positive;

		break;
	default:
		break;
	}

	return Pending;
}

void ThreadPooling::poolRequestsProcess()
{
	PipeEntry<PoolRequest> entryReq;
	PoolRequest req;
	size_t idDriver;

	while (ppPoolRequests.get(entryReq) > 0)
	{
		req = entryReq.particle;

		//procDbgLog("pool request received");

		if (req.idDriverDesired >= 0 && req.idDriverDesired < mCntInternals)
			idDriver = (size_t)req.idDriverDesired;
		else
			idDriver = idDriverNextGet();

		mVecInternals[idDriver]->procInternalAdd(req.pProc);
	}
}

void ThreadPooling::procsDrive()
{
	list<Processing *>::iterator iter;
	Processing *pProc;

	{
		Guard lock(mMtxBrokerInternal);
		mListProcs.splice(mListProcs.end(), mListProcsReq);
	}

	iter = mListProcs.begin();
	while (iter != mListProcs.end())
	{
		pProc = *iter;

		pProc->treeTick();

		if (pProc->progress())
		{
			++iter;
			continue;
		}

		//procDbgLog("finished driving process %p", pProc);

		{
			Guard lock(mMtxBrokerInternal);
			--mNumProcessing;
		}
		++mNumFinished;

		undrivenSet(pProc);
		iter = mListProcs.erase(iter);
	}
}

size_t ThreadPooling::idDriverNextGet()
{
	size_t idCurrent = 1;
	size_t numProcessingCurrent;
	size_t idSelected = 0;
	size_t numProcessingSelected =
			mVecInternals[idSelected]->numProcessingGet();

	for (; idCurrent < mVecInternals.size(); ++idCurrent)
	{
		numProcessingCurrent =
			mVecInternals[idCurrent]->numProcessingGet();

		if (numProcessingCurrent >= numProcessingSelected)
			continue;
		numProcessingSelected = numProcessingCurrent;

		idSelected = idCurrent;
	}

	return idSelected;
}

size_t ThreadPooling::numProcessingGet()
{
	Guard lock(mMtxBrokerInternal);
	return mNumProcessing;
}

// Executed by broker (different driver)
void ThreadPooling::procInternalAdd(Processing *pProc)
{
	Guard lock(mMtxBrokerInternal);
	mListProcsReq.push_back(pProc);
	++mNumProcessing;
}

void ThreadPooling::procAdd(Processing *pProc, int32_t idDriver)
{
	PoolRequest req;

	req.pProc = pProc;
	req.idDriverDesired = idDriver;

	//dbgLog("adding proc %p to queue", pProc);
	ppPoolRequests.commit(req);
}

void ThreadPooling::processInfo(char *pBuf, char *pBufEnd)
{
#if 0
	dInfo("State\t\t\t%s\n", ProcStateString[mState]);
	dInfo("State shutdown\t\t%s\n", SdStateString[mStateSd]);
#endif
	if (!mIsInternal)
		return;

	dInfo("Processing\t\t%zu\n", mNumProcessing);
	//dInfo("Finished\t\t%zu\n", mNumFinished);
}

/* static functions */

