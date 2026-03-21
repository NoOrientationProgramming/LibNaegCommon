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

#include <map>
#include <cstdio>
#include <fstream>
#include <mutex>
#include <stdarg.h>
#include <string.h>

#if defined(__unix__)
#include <signal.h>
#endif
#if defined(__linux__)
#include <sys/prctl.h>
#endif
#ifdef _WIN32
#include <winsock2.h>
#include <direct.h>
#endif

#include "LibFilesys.h"

using namespace std;

#if defined(__unix__)
struct GlobalLock
{
	void *pOwner;
	const char *filename;
	const char *function;
	int line;
	string nameRes;
};

static string lockDefaultDirBase;
static int fdLockDefault = 0;
static mutex globalLocksMtx;
static map<string, struct GlobalLock> globalLocks;
#endif

#if defined(__linux__)
// - https://man7.org/linux/man-pages/man2/prctl.2.html
// - https://man7.org/linux/man-pages/man5/core.5.html
// - https://man7.org/linux/man-pages/man5/proc.5.html
bool coreDumpsEnable(void (*pFctReq)(int signum))
{
	struct rlimit lim;
	int res;

	if (pFctReq)
	{
		signal(SIGILL, pFctReq);
		signal(SIGFPE, pFctReq);
		signal(SIGSEGV, pFctReq);
		signal(SIGBUS, pFctReq);
		signal(SIGTRAP, pFctReq);
	}

	res = getrlimit(RLIMIT_CORE, &lim);
	if (res)
	{
		int numErr = errno;
		errLog(-1, "getrlimit(CORE) failed: %s (%d)", strerror(numErr), numErr);
		return false;
	}

	lim.rlim_cur = RLIM_INFINITY;

	res = setrlimit(RLIMIT_CORE, &lim);
	if (res)
	{
		int numErr = errno;
		errLog(-1, "setrlimit(CORE, unlimited) failed: %s (%d)", strerror(numErr), numErr);
		return false;
	}

	res = prctl(PR_SET_DUMPABLE, 1);
	if (res)
	{
		int numErr = errno;
		errLog(-1, "prctl(PR_SET_DUMPABLE) failed: %s (%d)", strerror(numErr), numErr);
		return false;
	}

	return true;
}
#endif

#if defined(__unix__)
// - https://linux.die.net/man/2/setrlimit
// - https://www.man7.org/linux/man-pages/man3/getrlimit.3p.html
bool maxFdsSet(rlim_t val)
{
	struct rlimit lim;
	int res;

	res = getrlimit(RLIMIT_NOFILE, &lim);
	if (res)
	{
		int numErr = errno;
		errLog(-1, "getrlimit(NOFILE) failed: %s (%d)", strerror(numErr), numErr);
		return false;
	}

	lim.rlim_cur = val;

	res = setrlimit(RLIMIT_NOFILE, &lim);
	if (res)
	{
		int numErr = errno;
		errLog(-1, "setrlimit(NOFILE, %u) failed: %s (%d)", val, strerror(numErr), numErr);
		return false;
	}

	return true;
}

void pipeInit(PairFd &pair)
{
	pair.fdRead = -1;
	pair.fdWrite = -1;
}

void pipeClose(PairFd &pair, bool deInit)
{
	fdClose(pair.fdRead, deInit);
	fdClose(pair.fdWrite, deInit);
}

// - https://man7.org/linux/man-pages/man2/open.2.html
// - https://man7.org/linux/man-pages/man3/fopen.3.html
int fdCreate(const string &path, const string &mode, bool closeOnExec)
{
	if (!path.size() || !mode.size())
		return -1;

	int flags = 0;
	bool ok, createRequested = true;

	if (mode == "r")
	{
		flags = O_RDONLY;
		createRequested = false;
	}
	else
	if (mode == "r+")
	{
		flags = O_RDWR;
		createRequested = false;
	}
	else
	if (mode == "w")
		flags = O_WRONLY | O_TRUNC;
	else
	if (mode == "w+")
		flags = O_RDWR | O_TRUNC;
	else
	if (mode == "a")
		flags = O_WRONLY | O_APPEND;
	else
	if (mode == "a+")
		flags = O_RDWR | O_APPEND;

	if (createRequested)
	{
		ok = fileExists(path);
		if (!ok)
		{
			ok = fileCreate(path);
			if (!ok)
				return -1;
		}
	}

	if (closeOnExec)
		flags |= O_CLOEXEC;

	return open(path.c_str(), flags);
}

void fdClose(int &fd, bool deInit)
{
	if (fd < 0)
		return;

	close(fd);

	if (!deInit)
		return;

	fd = -1;
}

bool fileExists(const string &path)
{
	FILE *pFile = fopen(path.c_str(), "r");

	if (!pFile)
		return false;

	fclose(pFile);

	return true;
}

bool fileCreate(const string &path)
{
	FILE *pFile = fopen(path.c_str(), "w");

	if (!pFile)
		return false;

	fclose(pFile);

	return true;
}

void filesStdClose()
{
	fclose(stdin);
	fclose(stdout);
	fclose(stderr);
}
#endif

bool fileNonBlockingSet(int fd)
{
	int opt = 1;
#ifdef _WIN32
	unsigned long nonBlockMode = 1;

	opt = ioctlsocket(fd, FIONBIO, &nonBlockMode);
	if (opt == SOCKET_ERROR)
		return false;
#else
	opt = fcntl(fd, F_GETFL, 0);
	if (opt == -1)
		return false;

	opt |= O_NONBLOCK;

	opt = fcntl(fd, F_SETFL, opt);
	if (opt == -1)
		return false;
#endif
	return true;
}

bool fileCopy(const string &pathSrc, const string &pathDst)
{
	ifstream src(pathSrc, ios::binary);
	ofstream dst(pathDst, ios::binary);

	if (!src || !dst)
		return false;

	dst << src.rdbuf();

	return true;
}

bool dirExists(const string &path)
{
#if defined(__unix__)
	struct stat sb;
	int res;

	res = stat(path.c_str(), &sb);
	if (res)
		return false;

	if ((sb.st_mode & S_IFMT) != S_IFDIR)
		return false;

	return true;
#else
	return path.size();
#endif
}

#if defined(__unix__)
bool dirCreate(const string &path, mode_t mode)
#else
bool dirCreate(const string &path, int mode)
#endif
{
	size_t pos = 0;
	string node;
	bool ok, endReached = false;
	int res;

	for (; !endReached; ++pos)
	{
		pos = path.find('/', pos);
		node = path.substr(0, pos);

		if (pos == string::npos)
			endReached = true;

		if (!node.size())
			return false;

		ok = dirExists(node);
		if (ok)
			continue;
#if defined(_WIN32)
		(void)mode;
		res = mkdir(node.c_str());
#else
		res = mkdir(node.c_str(), mode);
#endif
		if (res)
			return false;
	}

	return true;
}

bool strToFile(const string &str, const string &path)
{
	FILE *pFile = fopen(path.c_str(), "w");

	if (!pFile)
		return false;

	size_t lenReq, lenWritten;

	lenReq = str.size();
	lenWritten = fwrite(str.c_str(), sizeof(char), lenReq, pFile);

	fclose(pFile);

	if (lenWritten != lenReq)
		return false;

	return true;
}

#if defined(__unix__)
/*
Literature
- https://linux.die.net/man/2/open
- https://linux.die.net/man/2/flock
*/
bool lockDirDefaultOpen(const string &dirBase)
{
	if (fdLockDefault)
		return false;

	if (!dirBase.size())
		return false;

	string dirBaseLocal = dirBase;

	if (dirBaseLocal.back() != '/')
		dirBaseLocal.push_back('/');

	string fileLock = dirBaseLocal + ".lock";

	fdLockDefault = open(fileLock.c_str(),
					/* flags */
					O_RDONLY | O_CREAT,
					/* mode */
					S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);

	if (fdLockDefault < 0)
		return false;

	lockDefaultDirBase = dirBaseLocal;

	return true;
}

void lockDirDefaultClose()
{
	if (fdLockDefault < 0)
		return;

	close(fdLockDefault);
}

Success sysFlagsIntLock(void *pRequester, const char *filename, const char *function, const int line, UserLocks *pLocks, ...)
{
	if (fdLockDefault < 0 || !pLocks)
		return -1;

	int res;

	// Step 1: Try to get the file lock
	{
		lock_guard<mutex> lock(globalLocksMtx);
		res = flock(fdLockDefault, LOCK_EX | LOCK_NB);
	}

	if (res && errno == EWOULDBLOCK)
		return Pending;

	if (res)
		return -1;

	// Start of remote critical section
	// Guarded by function flock()

	va_list args;
	const char *pArg;
	string pathFlag;

	// Step 2: Check if all resources/flags are available
	va_start(args, pLocks);
	while (pArg = va_arg(args, const char *), pArg)
	{
		pathFlag = lockDefaultDirBase + pArg;
		//wrnLog("Checking %s", pathFlag.c_str());

		if (fileExists(pathFlag))
		{
			lock_guard<mutex> lock(globalLocksMtx);
			flock(fdLockDefault, LOCK_UN | LOCK_NB);

			va_end(args);
			return Pending;
		}
	}
	va_end(args);

	// Step 3: Take/create all flags
	va_start(args, pLocks);
	while (pArg = va_arg(args, const char *), pArg)
		fileCreate(lockDefaultDirBase + pArg);
	va_end(args);

	// Step 4: Exit the remote critical section
	{
		lock_guard<mutex> lock(globalLocksMtx);

		// End of remote critical section
		flock(fdLockDefault, LOCK_UN | LOCK_NB);

		// Step 5: Change user and global locks list
		UserLock uLock;
		GlobalLock gLock;

		uLock.type = 0;

		gLock.pOwner = pRequester;
		gLock.filename = filename;
		gLock.function = function;
		gLock.line = line;

		va_start(args, pLocks);
		while (pArg = va_arg(args, const char *), pArg)
		{
			uLock.nameRes = pArg;
			pLocks->push_back(uLock);

			gLock.nameRes = pArg;
			globalLocks[pArg] = gLock;
		}
		va_end(args);
	}

	return Positive;
}

void sysFlagsIntUnlock(void *pRequester, const char *filename, const char *function, const int line, UserLocks *pLocks)
{
	if (fdLockDefault < 0 || !pLocks)
		return;

	if (!pLocks->size())
		return;
#if 0
	wrnLog("Process %p is unlocking", pRequester);
	wrnLog("In file %s in function %s() at line %d", filename, function, line);
#else
	(void)pRequester;
	(void)filename;
	(void)function;
	(void)line;
#endif
	UserLocks::iterator iuLock;

	{
		lock_guard<mutex> lock(globalLocksMtx);

		iuLock = pLocks->begin();
		for (; iuLock != pLocks->end(); ++iuLock)
			globalLocks.erase(iuLock->nameRes);
	}

	string pathFlag;

	iuLock = pLocks->begin();
	for (; iuLock != pLocks->end(); ++iuLock)
	{
#if 0
		wrnLog("Unlocking %s", iuLock->nameRes.c_str());
#endif
		pathFlag = lockDefaultDirBase + iuLock->nameRes;
		remove(pathFlag.c_str());
		iuLock = pLocks->erase(iuLock);
	}
}
#endif

