/*
  This file is part of the DSP-Crowd project
  https://www.dsp-crowd.com

  Author(s):
      - Johannes Natter, office@dsp-crowd.com

  File created on 03.01.2023

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

#include <iostream>
#include <sstream>
#include <chrono>
#include <cinttypes>
#include <string.h>
#include <locale>
#include <codecvt>

#ifdef _WIN32
/* See http://stackoverflow.com/questions/12765743/getaddrinfo-on-win32 */
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0501  /* Windows XP. */
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#endif

#include "LibDspc.h"

using namespace std;
using namespace chrono;
#if CONFIG_LIB_DSPC_HAVE_JSONCPP
using namespace Json;
#endif
#if CONFIG_LIB_DSPC_HAVE_CRYPTOPP
using namespace CryptoPP;
#endif

#define dLenIp4Max		15

const char *hexDigits = "0123456789abcdef";

#if CONFIG_LIB_DSPC_HAVE_CURL
static mutex mtxCurlGlobal;
static bool curlGlobalInitDone = false;
#endif

#if CONFIG_LIB_DSPC_HAVE_C_ARES
static mutex mtxCaresGlobal;
static bool caresGlobalInitDone = false;
#endif

// Debugging

string appVersion()
{
	string err = "<unknown>-yy.mm-n";

	const Resource *pHistory = resourceFind("history_txt");
	if (!pHistory)
		return err;

	const char *pVersStart;

	pVersStart = strchr(pHistory->pSrc, 'T');
	if (!pVersStart)
		return err;

	++pVersStart;

	pVersStart = strchr(pVersStart, 'T');
	if (!pVersStart)
		return err;

	const char *pVersEnd = strchr(pVersStart, '\n');
	if (!pVersEnd)
		return err;

	size_t len = (size_t)(pVersEnd - pVersStart);
	return string(pVersStart, len);
}

void hexDump(const void *pData, size_t len,
			const char *pName, size_t colWidth)
{
	if (!pData)
		return;

	const char *pByte = (const char *)pData;
	uint32_t addressAbs = 0;
	char bufLine[256];
	char *pBuf, *pBufEnd;
	const char *pLine;
	size_t lenPrinted;
	size_t numBytesPerLine = colWidth;
	size_t i;

	pBufEnd = bufLine;
	*pBufEnd = 0;

	pBufEnd = bufLine + sizeof(bufLine);

	pBuf = bufLine;
	*pBuf = 0;

	dInfo("%p  %s", pData, pName ? pName : "Data");
	cout << bufLine << endl;

	while (len)
	{
		pBuf = bufLine;
		*pBuf = 0;
		pLine = pByte;
		lenPrinted = 0;

		dInfo("%08" PRIx32, addressAbs);

		for (i = 0; i < numBytesPerLine; ++i)
		{
			if (!(i & 7))
				dInfo(" ");

			if (!len)
			{
				dInfo("   ");
				continue;
			}

			dInfo(" %02" PRIx8, (uint8_t)*pByte);

			++pByte;
			--len;
			++lenPrinted;
		}

		dInfo("  |");

		for (i = 0; i < lenPrinted; ++i, ++pLine)
		{
			char c = *pLine;

			if (c < 32 || c > 126)
			{
				dInfo(".");
				continue;
			}

			dInfo("%c", c);
		}

		dInfo("|");

		cout << bufLine << endl;

		addressAbs += lenPrinted;
	}
}

size_t hexDumpPrint(char *pBuf, char *pBufEnd,
			const void *pData, size_t len,
			const char *pName, size_t colWidth)
{
	if (!pData)
		return 0;

	const char *pBufStart = pBuf;
	const char *pByte = (const char *)pData;
	uint32_t addressAbs = 0;
	const char *pLine;
	size_t lenPrinted;
	size_t numBytesPerLine = colWidth;
	size_t i;

	dInfo("%p  %s\n", pData, pName ? pName : "Data");

	while (len)
	{
		pLine = pByte;
		lenPrinted = 0;

		dInfo("%08" PRIx32, addressAbs);

		for (i = 0; i < numBytesPerLine; ++i)
		{
			if (!(i & 7))
				dInfo(" ");

			if (!len)
			{
				dInfo("   ");
				continue;
			}

			dInfo(" %02" PRIx8, (uint8_t)*pByte);

			++pByte;
			--len;
			++lenPrinted;
		}

		dInfo("  |");

		for (i = 0; i < lenPrinted; ++i, ++pLine, ++pBuf)
		{
			char c = *pLine;

			if (c < 32 || c > 126)
			{
				*pBuf = '.';
				continue;
			}

			*pBuf = c;
		}

		dInfo("|\n");

		addressAbs += lenPrinted;
	}

	return (size_t)(pBuf - pBufStart);
}

string toHexStr(const string &strIn)
{
	string strOut;
	char ch;

	strOut.reserve(strIn.size() * 2);

	for (size_t i = 0; i < strIn.size(); ++i)
	{
		ch = strIn[i];

		strOut.push_back(hexDigits[ch >> 4]);
		strOut.push_back(hexDigits[ch & 0xF]);
	}

	return strOut;
}

vector<char> toHex(const string &strIn)
{
	size_t szStr = strIn.size();
	uint8_t highByteDone = szStr & 1;
	char ch, digit, b = 0;
	vector<char> res;

	if (szStr)
		res.reserve((szStr + 1) >> 1);

	for (size_t i = 0; i < szStr; ++i)
	{
		ch = strIn[i];

		if (ch >= 'a' && ch <= 'f')
			digit = ch - 'a' + 10;
		else
		if (ch >= 'A' && ch <= 'F')
			digit = ch - 'A' + 10;
		else
		if (ch >= '0' && ch <= '9')
			digit = ch - '0';
		else
			digit = 0;

		if (highByteDone)
		{
			b |= digit;
			res.push_back(b);
		}
		else
			b = digit << 4;

		highByteDone ^= 1;
	}

	return res;
}

size_t strReplace(string &strIn, const string &strFind, const string &strReplacement)
{
	size_t pos = strIn.find(strFind);

	if (pos == string::npos)
		return pos;

	string strTail = strIn.substr(pos + strFind.size());
	string str2 = strIn.substr(0, pos);

	strIn = str2 + strReplacement + strTail;

	return pos;
}

// Json

#if CONFIG_LIB_DSPC_HAVE_JSONCPP
bool jKeyFind(const Value &val, const string &nameKey)
{
	bool searchable = val.isObject() || val.isNull();

	if (!searchable || !val.isMember(nameKey))
		return false;

	return true;
}

void jsonPrint(const Value &val)
{
	StyledWriter jWriter;
	cout << endl << jWriter.write(val) << endl;
}
#endif

// Cryptography

#if CONFIG_LIB_DSPC_HAVE_CRYPTOPP
string base64enc(const string &msg)
{
	Base64Encoder encoder(NULL, false, 0);
	string encoded;
	size_t size;

	encoder.Put((const byte *)msg.data(), msg.size());
	encoder.MessageEnd();

	size = encoder.MaxRetrievable();
	if (!size)
		return "";

	encoded.resize(size);
	encoder.Get((byte *)&encoded[0], encoded.size());

	return encoded;
}

string md5(const string &msg, const string &prefix)
{
	Weak::MD5 hasher;
	string digest;

	if (prefix.size())
		hasher.Update((const byte *)prefix.data(), prefix.size());

	hasher.Update((const byte *)msg.data(), msg.size());

	digest.resize(hasher.DigestSize());
	hasher.Final((byte *)&digest[0]);

	return digest;
}

string sha256(const string &msg, const string &prefix)
{
	SHA256 hasher;
	string digest;

	if (prefix.size())
		hasher.Update((const byte *)prefix.data(), prefix.size());

	hasher.Update((const byte *)msg.data(), msg.size());

	digest.resize(hasher.DigestSize());
	hasher.Final((byte *)&digest[0]);

	return digest;
}

string sha256(const SecByteBlock &msg, const string &prefix)
{
	SHA256 hasher;
	string digest;

	if (prefix.size())
		hasher.Update((const byte *)prefix.data(), prefix.size());

	hasher.Update(msg.data(), msg.size());

	digest.resize(hasher.DigestSize());
	hasher.Final((byte *)&digest[0]);

	return digest;
}

bool isValidSha256(const string &digest)
{
	if (digest.size() != (SHA256::DIGESTSIZE << 1))
		return false;

	const char *pCh = digest.c_str();

	for (size_t i = 0; i < digest.size(); ++i, ++pCh)
	{
		if (*pCh >= '0' && *pCh <= '9')
			continue;

		if (*pCh >= 'a' && *pCh <= 'f')
			continue;

		if (*pCh >= 'A' && *pCh <= 'F')
			continue;

		return false;
	}

	return true;
}

string hmacSha256(const string &msg, const SecByteBlock &key)
{
	HMAC<SHA256> hmac(key.data(), key.size());
	SecByteBlock res(hmac.DigestSize());

	hmac.CalculateDigest(res, (const byte *)msg.data(), msg.size());

	return string((const char *)res.data(), res.size());
}
#endif

// curl

#if CONFIG_LIB_DSPC_HAVE_CURL
void curlGlobalInit()
{
	lock_guard<mutex> lock(mtxCurlGlobal);

	if (curlGlobalInitDone)
		return;

	Processing::globalDestructorRegister(curlGlobalDeInit);

	curl_global_init(CURL_GLOBAL_ALL);
	curlGlobalInitDone = true;

	dbgLog("global init curl done");
}

/*
 * Literature
 * - https://curl.haxx.se/mail/lib-2016-09/0047.html
 * - https://stackoverflow.com/questions/29845527/how-to-properly-uninitialize-openssl
 * - https://wiki.openssl.org/index.php/Library_Initialization
 * - Wichtig
 *   - https://rachelbythebay.com/w/2012/12/14/quiet/
 */
void curlGlobalDeInit()
{
	lock_guard<mutex> lock(mtxCurlGlobal);

	if (!curlGlobalInitDone)
		return;

	curl_global_cleanup();
	curlGlobalInitDone = false;

	dbgLog("global deinit curl done");
}

string versionCurl()
{
	return curl_version();
}

void versionCurlInfoPrint()
{
	curl_version_info_data *pInfo = curl_version_info(CURLVERSION_NOW);
	if (!pInfo)
		return;

	cout << "libcurl    " << pInfo->version << endl;
	cout << "libz       " << pInfo->libz_version << endl;
	cout << "SSL        " << (pInfo->ssl_version ? pInfo->ssl_version : "none") << endl;
}
#endif

// c-ares

#if CONFIG_LIB_DSPC_HAVE_C_ARES
/*
 * Literature
 * - https://c-ares.org/docs.html
 * - https://c-ares.org/docs/ares_library_init.html
 * - https://c-ares.org/docs/ares_library_cleanup.html
 */
void caresGlobalInit()
{
	lock_guard<mutex> lock(mtxCaresGlobal);

	if (caresGlobalInitDone)
		return;

	Processing::globalDestructorRegister(caresGlobalDeInit);

	ares_library_init(ARES_LIB_INIT_ALL);
	caresGlobalInitDone = true;

	dbgLog("global init c-ares done");
}

void caresGlobalDeInit()
{
	lock_guard<mutex> lock(mtxCaresGlobal);

	if (!caresGlobalInitDone)
		return;

	ares_library_cleanup();
	caresGlobalInitDone = false;

	dbgLog("global deinit c-ares done");
}
#endif

// Internet

bool isValidEmail(const string &mail)
{
	size_t pos;

	pos = mail.find('@');
	if (pos == string::npos)
		return false;

	return true;
}

static bool isValidIp4(const string &ip)
{
	struct in_addr addr;
	return inet_pton(AF_INET, ip.c_str(), &addr) == 1;
}

static bool isValidIp6(const string &ip)
{
	struct in_addr addr6;
	return inet_pton(AF_INET6, ip.c_str(), &addr6) == 1;
}

int typeIp(const string &ip)
{
	if (isValidIp4(ip))
		return AF_INET;

	string ipCopy;

	if (ip.size() > 1 && ip[0] == '[')
		ipCopy = ip.substr(1, ip.size() - 2);

	const string &ipCheck = ipCopy.size() ? ipCopy : ip;

	if (isValidIp6(ipCheck))
		return AF_INET6;

	return AF_UNSPEC;
}

string remoteAddr(int socketFd)
{
	struct sockaddr_in addr;
	socklen_t addrLen;

	memset(&addr, 0, sizeof(addr));

	addrLen = sizeof(addr);
	::getpeername(socketFd, (struct sockaddr*)&addr, &addrLen);

	return ::inet_ntoa(addr.sin_addr);
}

string urlToHost(const string &url)
{
	string protocol, host, path, queries;
	uint16_t port;

	urlToParts(url, protocol, host, port, path, queries);
	return host;
}

// https://dev.dsp-crowd.com:80/foo?bar=bas
void urlToParts(const string &url,
				string &protocol,
				string &host,
				uint16_t &port,
				string &path,
				string &queries)
{
	string::size_type idxRight = url.size() ? url.size() - 1 : 0;
	string::size_type idxLeft = 0;
	string::size_type idx, find, len;

	// defaults
	protocol = "";
	host = "";
	port = 0;
	path = "";
	queries = "";

	// protocol
	find = url.find("://", idxLeft);
	if (find != string::npos)
	{
		len = find - idxLeft;
		protocol = url.substr(idxLeft, len);
		idxLeft = find + 3;
	}

	// queries
	find = url.rfind('?', idxRight);
	if (find != string::npos)
	{
		len = idxRight - find;
		queries = url.substr(find + 1, len);
		idxRight = find;
		if (idxRight) --idxRight;
	}

	// path
	find = url.find('/', idxLeft);
	if (find != string::npos)
	{
		len = idxRight - find;
		path = url.substr(find + 1, len);
		idxRight = find;
		if (idxRight) --idxRight;
	}

	// port
	find = string::npos;
	idx = idxRight;

	for (; idx > idxLeft; --idx)
	{
		if (url[idx] != ':')
			continue;

		find = idx;
		break;
	}

	if (find != string::npos)
	{
		len = idxRight - find;
		const string &str = url.substr(find + 1, len);

		port = atoi(str.c_str());

		idxRight = find;
		if (idxRight) --idxRight;
	}

	// host
	len = (idxRight + 1) - idxLeft;
	host = url.substr(idxLeft, len);
}

// Strings

void strPadCutTo(string &str, size_t width, bool dots, bool padLeft)
{
	wstring_convert<codecvt_utf8<char32_t>, char32_t> converter;
	u32string u32str = converter.from_bytes(str);
	size_t sz = u32str.size();
	size_t szOld = sz;

	if (sz > width)
		u32str.erase(width);
	else
	if (width > sz)
	{
		if (padLeft)
			u32str.insert(0, width - sz, ' ');
		else
			u32str.append(width - sz, ' ');
	}

	if (szOld > width && width >= 2 && dots)
	{
		u32str.pop_back();
		u32str.pop_back();
		u32str.push_back('.');
		u32str.push_back('.');
	}

	str = converter.to_bytes(u32str);
}

void strToVecStr(const string &str, VecStr &vStr, char delim)
{
	istringstream ss(str);
	string line;

	while (getline(ss, line, delim))
		vStr.push_back(line);
}

void resToVecStr(const string &nameRes, VecStr &vStr)
{
	const Resource *pRes = resourceFind(nameRes.c_str());
	if (!pRes)
		return;

	strToVecStr(pRes->pSrc, vStr);
}

string trim(const string &str)
{
	const char *pNonWhite = " \t\n\r\f\v";
	size_t start = str.find_first_not_of(pNonWhite);
	if (start == string::npos)
		return "";

	size_t end = str.find_last_not_of(pNonWhite);
	return str.substr(start, end - start + 1);
}

