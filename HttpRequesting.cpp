/*
  This file is part of the DSP-Crowd project
  https://www.dsp-crowd.com

  Author(s):
      - Johannes Natter, office@dsp-crowd.com

  File created on 15.09.2018

  Copyright (C) 2018, Johannes Natter

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
#include <iomanip>
#include <regex>
#include "HttpRequesting.h"

#define dForEach_ProcState(gen) \
		gen(StStart) \
		gen(StDnsResolvStart) \
		gen(StDnsResolvDoneWait) \
		gen(StUrlReAsm) \
		gen(StEasyInit) \
		gen(StEasyBind) \
		gen(StReqStart) \
		gen(StReqDoneWait) \

#define dGenProcStateEnum(s) s,
dProcessStateEnum(ProcState);

#define dForEach_SdState(gen) \
		gen(StSdStart) \

#define dGenSdStateEnum(s) s,
dProcessStateEnum(SdState);

#if 1
#define dGenProcStateString(s) #s,
dProcessStateStr(ProcState);
#endif

using namespace std;

//#define ENABLE_CURL_SHARE

mutex HttpRequesting::mtxCurlMulti;
CURLM *HttpRequesting::pCurlMulti = NULL;

mutex HttpRequesting::sessionMtx;
list<HttpSession> HttpRequesting::sessions;

HttpRequesting::HttpRequesting()
	: Processing("HttpRequesting")
	, mStateSd(StSdStart)
	, mUrl("")
	, mProtocol("")
	, mNameHost("")
	, mAddrHost("")
	, mTypeNameHost(AF_UNSPEC)
	, mPort(0)
	, mPath("")
	, mQueries("")
	, mMethod("get")
	, mUserPw("")
	, mLstHdrs()
	, mData()
	, mAuthMethod("basic")
	, mVersionTls("")
	, mVersionHttp("HTTP/2")
	, mModeDebug(false)
#if CONFIG_LIB_DSPC_HAVE_C_ARES
	, mpResolv(NULL)
#endif
	, mpCurl(NULL)
	, mCurlBound(false)
	, mpListHeader(NULL)
	, mpListResolv(NULL)
	, mCurlRes(CURLE_OK)
	, mRespCode(0)
	, mRespHdr("")
	, mRespData()
#if 0 // TODO: Implement
	, mRetries(2)
#endif
	, mDoneCurl(Pending)
{
	mState = StStart;
	mpCurl = curl_easy_init();
}

HttpRequesting::HttpRequesting(const string &url)
	: Processing("HttpRequesting")
	, mStateSd(StSdStart)
	, mUrl(url)
	, mProtocol("")
	, mNameHost("")
	, mAddrHost("")
	, mTypeNameHost(AF_UNSPEC)
	, mPort(0)
	, mPath("")
	, mQueries("")
	, mMethod("get")
	, mUserPw("")
	, mLstHdrs()
	, mData()
	, mAuthMethod("basic")
	, mVersionTls("")
	, mVersionHttp("")
	, mModeDebug(false)
#if CONFIG_LIB_DSPC_HAVE_C_ARES
	, mpResolv(NULL)
#endif
	, mpCurl(NULL)
	, mCurlBound(false)
	, mpListHeader(NULL)
	, mpListResolv(NULL)
	, mCurlRes(CURLE_OK)
	, mRespCode(0)
	, mRespHdr("")
	, mRespData()
#if 0 // TODO: Implement
	, mRetries(2)
#endif
	, mDoneCurl(Pending)
{
	mState = StStart;
	mpCurl = curl_easy_init();
}

HttpRequesting::~HttpRequesting()
{
	if (!mpCurl)
		return;

	curl_easy_cleanup(mpCurl);
	mpCurl = NULL;
}

// input
void HttpRequesting::urlSet(const string &url)
{
	if (!url.size())
		return;

	mUrl = url;
}

void HttpRequesting::addrHostAdd(const string &addrHost)
{
	if (!addrHost.size())
		return;

	mAddrHost = addrHost;
}

void HttpRequesting::methodSet(const string &method)
{
	if (!method.size())
		return;

	mMethod = method;
}

void HttpRequesting::userPwSet(const string &userPw)
{
	if (!userPw.size())
		return;

	mUserPw = userPw;
}

void HttpRequesting::hdrAdd(const string &hdr)
{
	mLstHdrs.push_back(hdr);
}

void HttpRequesting::dataSet(const string &data)
{
	mData.assign(data.begin(), data.end());
}

void HttpRequesting::dataSet(const uint8_t *pData, size_t len)
{
	mData.assign(pData, pData + len);
}

void HttpRequesting::authMethodSet(const string &authMethod)
{
	if (!authMethod.size())
		return;

	mAuthMethod = authMethod;
}

void HttpRequesting::versionTlsSet(const string &versionTls)
{
	if (!versionTls.size())
		return;

	mVersionTls = versionTls;
}

void HttpRequesting::versionHttpSet(const string &versionHttp)
{
	if (!versionHttp.size())
		return;

	mVersionHttp = versionHttp;
}

void HttpRequesting::modeDebugSet(bool en)
{
	mModeDebug = en;
}

CURL *HttpRequesting::easyHandleCurl()
{
	return mpCurl;
}

// output
uint16_t HttpRequesting::respCode() const
{
	return mRespCode;
}

string &HttpRequesting::respHdr()
{
	return mRespHdr;
}

string HttpRequesting::respStr()
{
	return string(mRespData.begin(), mRespData.end());
}

vector<uint8_t> &HttpRequesting::respBytes()
{
	return mRespData;
}

Success HttpRequesting::process()
{
	//uint32_t curTimeMs = millis();
	//uint32_t diffMs = curTimeMs - mStartMs;
	Success success;
	//bool ok;
#if 0
	dStateTrace;
#endif
	switch (mState)
	{
	case StStart:

		if (!mpCurl)
			return procErrLog(-1, "could not initialize curl easy handle");

		if (!mUrl.size())
			return procErrLog(-1, "url not set");

		curlGlobalInit();

		urlToParts(mUrl, mProtocol, mNameHost, mPort, mPath, mQueries);

		if (!mProtocol.size())
			mProtocol = "https";

		mTypeNameHost = typeIp(mNameHost);

		if (!mPort)
			mPort = mProtocol == "https" ? 443 : 80;
#if 0
		procWrnLog("URL           %s", mUrl.c_str());
		procWrnLog("Protocol      %s", mProtocol.c_str());
		procWrnLog("Host name     %s", mNameHost.c_str());
		procWrnLog("Host address  %s", mAddrHost.c_str());
		procWrnLog("Port          %u", mPort);
		procWrnLog("Path          %s", mPath.c_str());
		procWrnLog("Queries       %s", mQueries.c_str());
#endif
		if (mTypeNameHost == AF_UNSPEC && !mAddrHost.size())
		{
			procDbgLog("resolving host");
			mState = StDnsResolvStart;
			break;
		}

		mState = StUrlReAsm;

		break;
	case StDnsResolvStart:

#if CONFIG_LIB_DSPC_HAVE_C_ARES
		mpResolv = DnsResolving::create();
		if (!mpResolv)
			return procErrLog(-1, "could not create process");

		mpResolv->hostnameSet(mNameHost);

		start(mpResolv);
#endif
		mState = StDnsResolvDoneWait;

		break;
	case StDnsResolvDoneWait:

#if CONFIG_LIB_DSPC_HAVE_C_ARES
		success = mpResolv->success();
		if (success == Pending)
			break;

		if (success == Positive)
		{
			const list<string> &lstAddr = mpResolv->lstIPv4();

			if (lstAddr.size())
				mAddrHost = *lstAddr.begin();
		}

		repel(mpResolv);
		mpResolv = NULL;
#endif
		if (!mAddrHost.size())
			procDbgLog("using curl internal DNS resolver");

		mState = StUrlReAsm;

		break;
	case StUrlReAsm:

		mUrl = mProtocol;
		mUrl += "://";

		mUrl += mNameHost;

		if (mPath.size())
		{
			mUrl.push_back('/');
			mUrl += mPath;
		}

		if (mQueries.size())
		{
			mUrl.push_back('?');
			mUrl += mQueries;
		}
#if 0
		procWrnLog("URL re-asm    %s", mUrl.c_str());
#endif
		mState = StEasyInit;

		break;
	case StEasyInit:

		success = easyHandleCurlConfigure();
		if (success != Positive)
			return procErrLog(-1, "could not configure curl easy handle");

		//procDbgLog("easy handle curl created");

		mState = StEasyBind;

		break;
	case StEasyBind:

		success = easyHandleCurlBind();
		if (success != Positive)
			return procErrLog(-1, "could not bind curl easy handle");

		//procDbgLog("easy handle curl bound");

		mState = StReqStart;

		break;
	case StReqStart:

		multiProcess();

		mState = StReqDoneWait;

		break;
	case StReqDoneWait:

		multiProcess();

		if (mDoneCurl == Pending)
			break;

		if (mCurlRes != CURLE_OK)
			return procErrLog(-1, "curl performing failed: %s (%d)",
						curl_easy_strerror(mCurlRes), mCurlRes);

		procDbgLog("server returned status code %d", mRespCode);

		return Positive;

		break;
	default:
		break;
	}

	return Pending;
}

/*
 * Literature
 * - https://curl.se/libcurl/c/curl_multi_remove_handle.html
 */
Success HttpRequesting::shutdown()
{
	switch (mStateSd)
	{
	case StSdStart:

		easyHandleCurlUnbind();

		curlListFree(&mpListHeader);
		curlListFree(&mpListResolv);

		return Positive;

		break;
	default:
		break;
	}

	return Pending;
}

/*
 * Literature
 * - https://curl.se/libcurl/c/
 * - https://curl.se/libcurl/c/curl_multi_add_handle.html
 * - https://curl.se/libcurl/c/libcurl-errors.html
 */
Success HttpRequesting::easyHandleCurlBind()
{
#if CONFIG_PROC_HAVE_DRIVERS
	Guard lock(mtxCurlMulti);
#endif
	if (!pCurlMulti)
		pCurlMulti = multiHandleCurlInit();

	if (!pCurlMulti)
		return procErrLog(-1, "curl multi handle not set");

	CURLMcode code;

	code = curl_multi_add_handle(pCurlMulti, mpCurl);
	if (code != CURLM_OK)
		return procErrLog(-1, "could not bind curl easy handle");

	mCurlBound = true;

	return Positive;
}

CURLM *HttpRequesting::multiHandleCurlInit()
{
	CURLM *pMulti;

	Processing::globalDestructorRegister(curlMultiDeInit);

	pMulti = curl_multi_init();
	if (!pMulti)
		return NULL;
#if 0
	curl_multi_setopt(pMulti, CURLMOPT_MAX_HOST_CONNECTIONS, 5L);
	curl_multi_setopt(pMulti, CURLMOPT_MAX_TOTAL_CONNECTIONS, 10L);
#endif
	dbgLog("global init curl multi done");

	return pMulti;
}

void HttpRequesting::easyHandleCurlUnbind()
{
#if CONFIG_PROC_HAVE_DRIVERS
	Guard lock(mtxCurlMulti);
#endif
	if (!mCurlBound)
		return;

	CURLMcode code;

	code = curl_multi_remove_handle(pCurlMulti, mpCurl);
	if (code != CURLM_OK)
		procWrnLog("could not unbind curl easy handle");

	mCurlBound = false;
	//procDbgLog("easy handle curl unbound");
}

/*
 * Literature regex
 * - https://regexr.com/
 * - https://regex101.com/
 * - https://www.regular-expressions.info/posixbrackets.html
 * - http://www.cplusplus.com/reference/regex/ECMAScript/
 *
 * Literature
 * - https://curl.se/libcurl/c/
 * - https://curl.se/libcurl/c/libcurl-easy.html
 * - https://curl.se/libcurl/c/post-callback.html
 * - https://curl.se/libcurl/c/multithread.html
 * - https://curl.se/libcurl/c/debug.html
 * - https://curl.se/libcurl/c/CURLOPT_RESOLVE.html
 * - https://curl.se/libcurl/c/CURLOPT_URL.html
 * - https://curl.se/libcurl/c/CURLOPT_HTTPAUTH.html
 * - https://curl.se/libcurl/c/CURLOPT_SSLVERSION.html
 * - https://curl.se/libcurl/c/CURLOPT_SSL_VERIFYPEER.html
 * - https://curl.se/libcurl/c/CURLOPT_SSL_VERIFYHOST.html
 * - https://curl.se/libcurl/c/CURLOPT_CAPATH.html
 * - https://curl.se/libcurl/c/CURLOPT_PRIVATE.html
 * - https://curl.se/libcurl/c/CURLOPT_SSL_OPTIONS.html
 * - https://curl.se/docs/sslcerts.html
 * - https://curl.se/mail/archive-2015-05/0006.html
 * - https://gist.github.com/leprechau/e6b8fef41a153218e1f4
 * - https://gist.github.com/whoshuu/2dc858b8730079602044
 * - https://curl.se/libcurl/c/libcurl-multi.html
 *   - https://curl.se/libcurl/c/multi-app.html
 * - https://curl.se/mail/lib-2018-12/0011.html
 */
Success HttpRequesting::easyHandleCurlConfigure()
{
	list<string>::const_iterator iter;
	struct curl_slist *pEntry;
	string versionTls;
	Success success = Positive;

	if (!mpCurl)
		return procErrLog(-1, "curl easy handle not initialized");

	if (mUrl[4] == 's')
		versionTls = "TLSv1.2";

	if (mVersionTls.size())
		versionTls = mVersionTls;
#if 0
	procDbgLog("url        = %s", mUrl.c_str());
	procDbgLog("method     = %s", mMethod.c_str());

	iter = mLstHdrs.begin();
	for (; iter != mLstHdrs.end(); ++iter)
		procDbgLog("hdr        = %s", iter->c_str());

	//hexDump(mData.data(), mData.size());
	procDbgLog("authMethod = %s", mAuthMethod.c_str());
	procDbgLog("versionTls = %s", versionTls.c_str());
#endif
#ifdef ENABLE_CURL_SHARE
	if (sessionCreate(address, port) != Positive)
		return procErrLog(-1, "could not create session");
#endif
	curl_easy_setopt(mpCurl, CURLOPT_URL, mUrl.c_str());

	if (mPort)
		curl_easy_setopt(mpCurl, CURLOPT_PORT, mPort);

	if (mAuthMethod == "digest")
		curl_easy_setopt(mpCurl, CURLOPT_HTTPAUTH, CURLAUTH_DIGEST); // default: CURLAUTH_BASIC

	if (versionTls != "")
	{
		curl_easy_setopt(mpCurl, CURLOPT_SSL_VERIFYPEER, 1L);
		curl_easy_setopt(mpCurl, CURLOPT_SSL_VERIFYHOST, 0L);
		curl_easy_setopt(mpCurl, CURLOPT_SSL_OPTIONS, CURLSSLOPT_NATIVE_CA | CURLSSLOPT_NO_REVOKE);
		//curl_easy_setopt(mpCurl, CURLOPT_SSL_FALSESTART, 1L); // may safe time => test
	}

	if (versionTls == "SSLv2")
		curl_easy_setopt(mpCurl, CURLOPT_SSLVERSION, CURL_SSLVERSION_SSLv2);
	else if (versionTls == "SSLv3")
		curl_easy_setopt(mpCurl, CURLOPT_SSLVERSION, CURL_SSLVERSION_SSLv3);
	else if (versionTls == "TLSv1")
		curl_easy_setopt(mpCurl, CURLOPT_SSLVERSION, CURL_SSLVERSION_TLSv1);
	else if (versionTls == "TLSv1.0")
		curl_easy_setopt(mpCurl, CURLOPT_SSLVERSION, CURL_SSLVERSION_TLSv1_0);
	else if (versionTls == "TLSv1.1")
		curl_easy_setopt(mpCurl, CURLOPT_SSLVERSION, CURL_SSLVERSION_TLSv1_1);
	else if (versionTls == "TLSv1.2")
		curl_easy_setopt(mpCurl, CURLOPT_SSLVERSION, CURL_SSLVERSION_TLSv1_2);
	else if (versionTls == "TLSv1.3")
		curl_easy_setopt(mpCurl, CURLOPT_SSLVERSION, CURL_SSLVERSION_TLSv1_3);
	else if (versionTls != "")
	{
		success = procErrLog(-1, "unknown TLS version");
		goto errCleanupCurl;
	}

	if (mVersionHttp == "HTTP/1.0")
		curl_easy_setopt(mpCurl, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_0);
	else if (mVersionHttp == "HTTP/1.1")
		curl_easy_setopt(mpCurl, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_1);
	else if (mVersionHttp == "HTTP/2")
		curl_easy_setopt(mpCurl, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_2_0);

	// headers
	curlListFree(&mpListHeader);

	iter = mLstHdrs.begin();
	for (; iter != mLstHdrs.end(); ++iter)
	{
		pEntry = curl_slist_append(mpListHeader, iter->c_str());
		if (!pEntry)
		{
			procWrnLog("could not create header list entry");
			break;
		}

		if (!mpListHeader)
			mpListHeader = pEntry;
	}

	if (mpListHeader)
		curl_easy_setopt(mpCurl, CURLOPT_HTTPHEADER, mpListHeader);

	// resolv
	curlListFree(&mpListResolv);

	if (mTypeNameHost == AF_UNSPEC && mAddrHost.size())
	{
		string str = mNameHost + ":" + to_string(mPort) + ":" + mAddrHost;

		mpListResolv = curl_slist_append(mpListResolv, str.c_str());
		if (!mpListResolv)
			procWrnLog("could not create resolv list entry");
	}

	if (mpListResolv)
		curl_easy_setopt(mpCurl, CURLOPT_RESOLVE, mpListResolv);

	// continued
	if (mMethod == "post" || mMethod == "put")
	{
		curl_easy_setopt(mpCurl, CURLOPT_POSTFIELDS, mData.data());
		curl_easy_setopt(mpCurl, CURLOPT_POSTFIELDSIZE, mData.size());
	}

	if (mUserPw.size())
		curl_easy_setopt(mpCurl, CURLOPT_USERPWD, mUserPw.c_str());

	curl_easy_setopt(mpCurl, CURLOPT_HEADERFUNCTION, HttpRequesting::curlDataToStringWrite);
	curl_easy_setopt(mpCurl, CURLOPT_HEADERDATA, &mRespHdr);

	curl_easy_setopt(mpCurl, CURLOPT_WRITEFUNCTION, HttpRequesting::curlDataToByteVecWrite);
	curl_easy_setopt(mpCurl, CURLOPT_WRITEDATA, &mRespData);

	curl_easy_setopt(mpCurl, CURLOPT_PRIVATE, this);
#ifdef ENABLE_CURL_SHARE
	curl_easy_setopt(mpCurl, CURLOPT_COOKIEFILE, "");
	curl_easy_setopt(mpCurl, CURLOPT_SHARE, mSession->pCurlShare);
#endif
	if (mModeDebug)
	{
		procWrnLog("verbose mode set");
		curl_easy_setopt(mpCurl, CURLOPT_DEBUGFUNCTION, curlTrace);
		curl_easy_setopt(mpCurl, CURLOPT_VERBOSE, 1L);
	}
#if 0
	//curl_easy_setopt(mpCurl, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_WHATEVER);
	curl_easy_setopt(mpCurl, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_V4);
	curl_easy_setopt(mpCurl, CURLOPT_DNS_CACHE_TIMEOUT, 0L);
	curl_easy_setopt(mpCurl, CURLOPT_DNS_USE_GLOBAL_CACHE, 0L);

	curl_easy_setopt(mpCurl, CURLOPT_CONNECTTIMEOUT, 5L);
	curl_easy_setopt(mpCurl, CURLOPT_TIMEOUT, 10L);

	curl_easy_setopt(mpCurl, CURLOPT_FRESH_CONNECT, 1L);
	curl_easy_setopt(mpCurl, CURLOPT_FORBID_REUSE, 1L);
#endif
	return Positive;

errCleanupCurl:
	curlListFree(&mpListHeader);
	curlListFree(&mpListResolv);

	curl_easy_cleanup(mpCurl);
#ifdef ENABLE_CURL_SHARE
	sessionTerminate();
#endif
	return success;
}

/*
 * Literature libcurl
 * - https://curl.se/libcurl/c/libcurl-share.html
 * - https://curl.se/libcurl/c/curl_share_init.html
 * - https://curl.se/libcurl/c/CURLOPT_SHARE.html
 * - https://curl.se/libcurl/c/curl_share_setopt.html
 * - https://ec.haxx.se/libcurl-sharing.html
 * - https://curl.se/mail/lib-2016-04/0139.html
 * - https://curl.se/libcurl/c/example.html
 * - https://curl.se/libcurl/c/threaded-shared-conn.html
 * - https://curl.se/libcurl/c/threaded-ssl.html
 */
Success HttpRequesting::sessionCreate(const string &address, const uint16_t port)
{
#if CONFIG_PROC_HAVE_DRIVERS
	Guard lock(sessionMtx);
#endif
	list<HttpSession>::iterator iter;
	bool sessionFound = false;

	procDbgLog("remote socket for session: %s:%d", address.c_str(), port);
	procDbgLog("current number of sessions: %d", sessions.size());

	for (iter = sessions.begin(); iter != sessions.end(); ++iter)
	{
		procDbgLog("%d %s %d", iter->numReferences, iter->address.c_str(), iter->port);

		if (iter->address == address && iter->port == port)
		{
			mSession = iter;
			sessionFound = true;
			break;
		}
	}

	// uncomment if you want to use only one request per session
	// not recommendet
	//sessionFound = false;

	if (sessionFound)
	{
		procDbgLog("reusing existing session");

		++mSession->numReferences;
	} else {
		procDbgLog("no existing session found. Creating");

		HttpSession session();
		sessions.push_front(session);
		mSession = sessions.begin();

		mSession->numReferences = 1;
		mSession->maxReferences = 1;
		mSession->address = address;
		mSession->port = port;

		mSession->sharedDataMtxList.resize(numSharedDataTypes, NULL);

		for (size_t i = 0; i < numSharedDataTypes; ++i)
		{
			mSession->sharedDataMtxList[i] = new dNoThrow mutex;

			if (!mSession->sharedDataMtxList[i])
			{
				sharedDataMtxListDelete();
				sessions.erase(mSession);

				return procErrLog(-1, "could not allocate shared data mutexes for session");
			}
		}

		mSession->pCurlShare = curl_share_init();
		if (!mSession->pCurlShare)
		{
			sharedDataMtxListDelete();
			sessions.erase(mSession);

			return procErrLog(-1, "curl_share_init() returned 0");
		}

		int code = CURLSHE_OK;

		code += curl_share_setopt(mSession->pCurlShare, CURLSHOPT_SHARE, CURL_LOCK_DATA_COOKIE);
		code += curl_share_setopt(mSession->pCurlShare, CURLSHOPT_SHARE, CURL_LOCK_DATA_DNS);
		code += curl_share_setopt(mSession->pCurlShare, CURLSHOPT_SHARE, CURL_LOCK_DATA_SSL_SESSION);
		code += curl_share_setopt(mSession->pCurlShare, CURLSHOPT_SHARE, CURL_LOCK_DATA_CONNECT);

		code += curl_share_setopt(mSession->pCurlShare, CURLSHOPT_USERDATA, &mSession->sharedDataMtxList);
		code += curl_share_setopt(mSession->pCurlShare, CURLSHOPT_LOCKFUNC, HttpRequesting::sharedDataLock);
		code += curl_share_setopt(mSession->pCurlShare, CURLSHOPT_UNLOCKFUNC, HttpRequesting::sharedDataUnLock);

		if (code != CURLSHE_OK)
		{
			curl_share_cleanup(mSession->pCurlShare);
			sharedDataMtxListDelete();
			sessions.erase(mSession);

			return procErrLog(-1, "curl_share_setopt() failed");
		}
	}

	if (mSession->numReferences > mSession->maxReferences)
		mSession->maxReferences = mSession->numReferences;

	procDbgLog("current number of session references: %d", mSession->numReferences);

	return Positive;
}

void HttpRequesting::sessionTerminate()
{
#if CONFIG_PROC_HAVE_DRIVERS
	Guard lock(sessionMtx);
#endif
	procDbgLog("dereferencing session: %s:%d", mSession->address.c_str(), mSession->port);

	--mSession->numReferences;
	procDbgLog("%d session references left", mSession->numReferences);

	if (mSession->numReferences)
		return;

	procDbgLog("terminating session. max number of session references were %d",
		mSession->maxReferences);

	curl_share_cleanup(mSession->pCurlShare);
	sharedDataMtxListDelete();
	sessions.erase(mSession);
}

void HttpRequesting::sharedDataMtxListDelete()
{
	size_t i = 0;
	while (i < numSharedDataTypes && mSession->sharedDataMtxList[i])
		delete mSession->sharedDataMtxList[i++];
}

void HttpRequesting::processInfo(char *pBuf, char *pBufEnd)
{
#if 1
	dInfo("State\t%s\n", ProcStateString[mState]);
	dInfo("URL\t\t%s\n", mUrl.c_str());
#endif
}

/* static functions */

/*
 * Literature
 * - https://curl.se/libcurl/c/curl_multi_perform.html
 * - https://curl.se/libcurl/c/curl_multi_info_read.html
 * - https://curl.se/libcurl/c/CURLINFO_RESPONSE_CODE.html
 * - https://curl.se/libcurl/c/CURLINFO_PRIVATE.html
 */
void HttpRequesting::multiProcess()
{
#if CONFIG_PROC_HAVE_DRIVERS
	Guard lock(mtxCurlMulti);
#endif
	int numRunningRequests, numMsgsLeft;
	CURLMsg *curlMsg;
	CURL *pCurl;
	HttpRequesting *pReq;

	curl_multi_perform(pCurlMulti, &numRunningRequests);

	while (curlMsg = curl_multi_info_read(pCurlMulti, &numMsgsLeft), curlMsg)
	{
		//dbgLog("messages left: %d", numMsgsLeft);

		if (curlMsg->msg != CURLMSG_DONE)
			continue;

		pCurl = curlMsg->easy_handle;

		curl_easy_getinfo(pCurl, CURLINFO_PRIVATE, &pReq);

		if (pCurl != pReq->mpCurl)
			wrnLog("pCurl (%p) does not match mpCurl (%p)", pCurl, pReq->mpCurl);

		pReq->mCurlRes = curlMsg->data.result;
		curl_easy_getinfo(pCurl, CURLINFO_RESPONSE_CODE, &pReq->mRespCode);
#if 0
		dbgLog("curl msg done   %p", pReq);
		dbgLog("result          %d", pReq->mCurlRes);
		dbgLog("response code   %d", pReq->mRespCode);
		dbgLog("url             %s", pReq->mUrl.substr(33).c_str());
#endif
		curl_multi_remove_handle(pCurlMulti, pCurl);
		pReq->mCurlBound = false;
		//dbgLog("easy handle curl unbound");

		curlListFree(&pReq->mpListHeader);
		curlListFree(&pReq->mpListResolv);

		curl_easy_cleanup(pReq->mpCurl);
		pReq->mpCurl = NULL;
#ifdef ENABLE_CURL_SHARE
		pReq->sessionTerminate();
#endif
		pReq->mDoneCurl = Positive;
	}
#if 0
	--mRetries;
	if (mRetries)
	{
		procDbgLog("retries left %d", mRetries);
		success = Pending;
	} else
		procDbgLog("no retry");
#endif
}

/*
 * Literature
 * - https://curl.se/mail/lib-2016-09/0047.html
 * - https://stackoverflow.com/questions/29845527/how-to-properly-uninitialize-openssl
 * - https://wiki.openssl.org/index.php/Library_Initialization
 * - Wichtig
 *   - https://rachelbythebay.com/w/2012/12/14/quiet/
 */
void HttpRequesting::curlMultiDeInit()
{
#if CONFIG_PROC_HAVE_DRIVERS
	Guard lock(mtxCurlMulti);
#endif
	if (!pCurlMulti)
		return;

	curl_multi_cleanup(pCurlMulti);
	pCurlMulti = NULL;

	dbgLog("global deinit curl multi done");
}

extern "C" void HttpRequesting::sharedDataLock(CURL *handle, curl_lock_data data, curl_lock_access access, void *userptr)
{
	int dataIdx = data - 1;

	(void)handle;
	(void)access;

	if (dataIdx < numSharedDataTypes)
		(*((vector<mutex *> *)userptr))[dataIdx]->lock();
	else
		cerr << "curl shared data lock: dataIdx(" << dataIdx << ") >= numSharedDataTypes(4)" << endl;
}

extern "C" void HttpRequesting::sharedDataUnLock(CURL *handle, curl_lock_data data, void *userptr)
{
	int dataIdx = data - 1;

	(void)handle;

	if (dataIdx < numSharedDataTypes)
		(*((vector<mutex *> *)userptr))[dataIdx]->unlock();
	else
		cerr << "curl shared data unlock: dataIdx(" << dataIdx << ") >= numSharedDataTypes(4)" << endl;
}

extern "C" size_t HttpRequesting::curlDataToStringWrite(void *ptr, size_t size, size_t nmemb, string *pData)
{
	size_t sz = size * nmemb;
	pData->append((char *)ptr, sz);
	return sz;
}

extern "C" size_t HttpRequesting::curlDataToByteVecWrite(void *ptr, size_t size, size_t nmemb, vector<uint8_t> *pData)
{
	size_t sz = size * nmemb;
	pData->insert(pData->end(), (uint8_t *)ptr, ((uint8_t *)ptr) + sz);
	return sz;
}

extern "C" int HttpRequesting::curlTrace(CURL *pCurl, curl_infotype type, char *pData, size_t size, void *pUser)
{
	int typeInt = (int)type;
	const char *pText;
	HttpRequesting *pReq;

	(void)pUser;

	curl_easy_getinfo(pCurl, CURLINFO_PRIVATE, &pReq);

	switch (typeInt)
	{
		case CURLINFO_TEXT:
			pText = "== Info:";
			break;
		case CURLINFO_HEADER_OUT:
			pText = "=> Send header";
			return 0;
		case CURLINFO_DATA_OUT:
			pText = "=> Send data";
			return 0;
		case CURLINFO_SSL_DATA_OUT:
			pText = "=> Send SSL data";
			break;
		case CURLINFO_HEADER_IN:
			pText = "<= Recv header";
			return 0;
		case CURLINFO_DATA_IN:
			pText = "<= Recv data";
			return 0;
		case CURLINFO_SSL_DATA_IN:
			pText = "<= Recv SSL data";
			break;
		default:
			wrnLog("cURL debug type unknown");
			return 0;
	}

	dbgLog("%-20s from %p", pText, pReq);

	if (pData && size)
		hexDump(pData, size);

	return 0;
}

void HttpRequesting::curlListFree(struct curl_slist **ppList)
{
	if (!ppList || !*ppList)
		return;

	curl_slist_free_all(*ppList);
	*ppList = NULL;
}

