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

#ifndef THREAD_POOLING_H
#define THREAD_POOLING_H

#include <vector>
#include <list>
#include <mutex>

#include "Processing.h"
#include "Pipe.h"

typedef void (*FuncDriverPoolCreate)(Processing *pProc, uint16_t idProc);

struct PoolRequest
{
	Processing *pProc;
	int32_t idDriverDesired;
};

class ThreadPooling : public Processing
{

public:

	static ThreadPooling *create()
	{
		return new dNoThrow ThreadPooling;
	}

	void workerCntSet(uint16_t cnt);
	void driverCreateSet(FuncDriverPoolCreate pFctDriverCreate);

	static void procAdd(Processing *pProc, int32_t idDriver = -1);

protected:

	virtual ~ThreadPooling() {}

private:

	ThreadPooling();
	ThreadPooling(const ThreadPooling &) = delete;
	ThreadPooling &operator=(const ThreadPooling &) = delete;

	/*
	 * Naming of functions:  objectVerb()
	 * Example:              peerAdd()
	 */

	/* member functions */
	Success process();
	Success shutdown();
	void processInfo(char *pBuf, char *pBufEnd);

	void poolRequestsProcess();
	void procsDrive();
	size_t idDriverNextGet();
	size_t numProcessingGet();
	void procInternalAdd(Processing *pProc);

	/* member variables */
	uint32_t mStateSd;

	// Broker
	uint16_t mCntInternals;
	std::vector<ThreadPooling *> mVecInternals;
	FuncDriverPoolCreate mpFctDriverCreate;

	// Internal
	bool mIsInternal;
	size_t mNumProcessing;
	size_t mNumFinished;
	std::list<Processing *> mListProcsReq;
	std::list<Processing *> mListProcs;
	std::mutex mMtxBrokerInternal;

	/* static functions */

	/* static variables */
	static Pipe<PoolRequest> ppPoolRequests;

	/* constants */

};

#endif

