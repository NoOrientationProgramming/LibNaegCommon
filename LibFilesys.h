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

#ifndef LIB_FILESYS_H
#define LIB_FILESYS_H

#include <string>
#include <list>
#ifndef _WIN32
#include <unistd.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/resource.h>
#endif

#include "Processing.h"

struct UserLock
{
	uint32_t type; // Not used at the moment
	std::string nameRes;
};

typedef std::list<UserLock> UserLocks;

struct PairFd
{
	int fdRead;
	int fdWrite;
};

#if defined(__linux__)
bool coreDumpsEnable(void (*pFctReq)(int signum) = NULL);
#endif

#if defined(__unix__)
bool maxFdsSet(rlim_t val);

void pipeInit(PairFd &pair);
void pipeClose(PairFd &pair, bool deInit = true);

int fdCreate(const std::string &path, const std::string &mode, bool closeOnExec = true);
void fdClose(int &fd, bool deInit = true);

bool fileExists(const std::string &path);
bool fileCreate(const std::string &path);
#endif
void filesStdClose();

bool fileNonBlockingSet(int fd);
bool fileCopy(const std::string &pathSrc, const std::string &pathDst);

bool dirExists(const std::string &path);
#if defined(__unix__)
bool dirCreate(const std::string &path, mode_t mode = (S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH));
#else
bool dirCreate(const std::string &path, int mode = 0);
#endif

bool strToFile(const std::string &str, const std::string &path);

#if defined(__unix__)
bool lockDirDefaultOpen(const std::string &dirBase);
void lockDirDefaultClose();
Success sysFlagsIntLock(void *pRequester, const char *filename, const char *function, const int line, UserLocks *pLocks, ...);
#define sysFlagsLock(...)			sysFlagsIntLock(this, __PROC_FILENAME__, __FUNCTION__, __LINE__, &mLocks, ##__VA_ARGS__, NULL)
void sysFlagsIntUnlock(void *pRequester, const char *filename, const char *function, const int line, UserLocks *pLocks);
#define sysFlagsUnlock()				sysFlagsIntUnlock(this, __PROC_FILENAME__, __FUNCTION__, __LINE__, &mLocks)
#endif

#endif

