
// OS.cpp
// Operating system support utilities.
//
// Copyright (c) 2009 Michael Imamura.
//
// Licensed under GrokkSoft HoverRace SourceCode License v1.0(the "License");
// you may not use this file except in compliance with the License.
//
// A copy of the license should have been attached to the package from which
// you have taken this file. If you can not find the license you can not use
// this file.
//
//
// The author makes no representations about the suitability of
// this software for any purpose.  It is provided "as is" "AS IS",
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
// implied.
//
// See the License for the specific language governing permissions
// and limitations under the License.

#include "StdAfx.h"

#include <locale.h>
#include <stdlib.h>

#include <string>

#include <boost/format.hpp>

#include "OS.h"

using boost::format;
using boost::str;

using namespace HoverRace::Util;

/**
 * Parse the resolution from a string.
 * @param s The string (e.g. "1024x768").
 */
OS::Resolution::Resolution(const std::string &s)
{
	const char *cs = s.c_str();
	w = atoi(cs);
	size_t div = s.find('x');
	h = (div == std::string::npos) ? 0 : atoi(cs + div + 1);
}

/**
 * Render the resolution as a string.
 * @return The string (ASCII).
 */
std::string OS::Resolution::AsString() const
{
	return str(format("%dx%d") % w % h);
}

/**
 * Set an environment variable.
 * @param key The variable name (may not be NULL).
 * @param val The variable value (may not be NULL).
 */
void OS::SetEnv(const char *key, const char *val)
{
	ASSERT(key != NULL);
	ASSERT(val != NULL);

	std::string combined(key);
	combined += '=';
	combined += val;

#	ifdef _WIN32
		// Windows has several environments; we need to update them all.
		_putenv(combined.c_str());
		SetEnvironmentVariable(key, val);

		// MSVC6-linked libraries have a separate environment.
		typedef int (_cdecl *putenv_t)(const char *);
		static putenv_t privPutEnv = NULL;
		if (privPutEnv == NULL) {
			HMODULE hmod = GetModuleHandle("msvcrt");
			if (hmod == NULL) return;
			privPutEnv = (putenv_t)GetProcAddress(hmod, "_putenv");
			if (privPutEnv == NULL) return;
		}
		privPutEnv(combined.c_str());
#	else
#		ifdef HAVE_SETENV
			setenv(key, val, 1);
#		else
			putenv(strdup(combined.c_str()));
#		endif
#	endif
}

#ifdef _WIN32
static const char *locLang = NULL;
static const char *locRegion = NULL;
//static const char *locCharset = NULL;
//static const char *locModifier = NULL;
static bool locMatched = false;

// Callback for EnumSystemLocales(), used by OS::SetLocale().
// See EnumLocalesProc in MSDN.
static BOOL CALLBACK SetLocaleProc(LPTSTR locale)
{
	char *s;
	LCID lcid = strtoul(locale, &s, 16);
	if (*s != '\0') return TRUE;

	// Check for matching language name.
	char curLang[16];
	if (GetLocaleInfo(lcid, LOCALE_SISO639LANGNAME, curLang, 16) == 0) return TRUE;
	if (strcmp(curLang, locLang) != 0) return TRUE;

	// Check for matching region.
	if (locRegion == NULL) {
		if (SUBLANGID(LANGIDFROMLCID(lcid)) != SUBLANG_DEFAULT) return TRUE;
	}
	else {
		char curRegion[16];
		if (GetLocaleInfo(lcid, LOCALE_SISO3166CTRYNAME, curRegion, 16) == 0) return TRUE;
		if (strcmp(curRegion, locRegion) != 0) return TRUE;
	}

	// If we made it this far, then we have a match!
	// Note: This does nothing on Vista and later.
	if (SetThreadLocale(lcid) == 0) {
		// Found a matching LCID but couldn't set it.
		/*
		DWORD err = GetLastError();
		LPVOID errMsg;
		FormatMessage(
			FORMAT_MESSAGE_ALLOCATE_BUFFER |
			FORMAT_MESSAGE_FROM_SYSTEM,
			NULL,
			err,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPTSTR)&errMsg,
			0, NULL );
		MessageBox(NULL, (const char*)errMsg, "AIEEE", MB_ICONERROR | MB_APPLMODAL | MB_OK);
		LocalFree(errMsg);
		*/
		ASSERT(FALSE);
	}
	locMatched = true;
	return FALSE;
}
#endif

/**
 * Set the locale based on the current environment.
 */
void OS::SetLocale()
{
	// Common setting.
	setlocale(LC_ALL, "");

#	ifdef _WIN32
		// For Win32, each thread has its own locale setting.
		// We need to map the ISO-abbreviated locale name from the env
		// to a Win32 LCID by enumerating all of the supported locales
		// and finding the one that matches.
		
		char *lang = getenv("LC_ALL");
		if (lang == NULL || *lang == '\0') {
			lang = getenv("LC_MESSAGES");
			if (lang == NULL || *lang == '\0') {
				lang = getenv("LANG");
				if (lang == NULL || *lang == '\0') {
					SetThreadLocale(LOCALE_SYSTEM_DEFAULT);
					return;
				}
			}
		}
		lang = strdup(lang);

		if (strcmp(lang, "C") == 0) {
			// The "C" locale on Win32 roughly means to use system default.
			SetThreadLocale(LOCALE_SYSTEM_DEFAULT);
		}
		else {
			// POSIX locale string format:
			//   lang[_region][.charset][@modifier]
			// lang corresponds to the LOCALE_SISO639LANGNAME and
			// region corresponds to the LOCALE_SISO3166CTRYNAME.
			// We currently ignore the charset and modifier.
			locLang = lang;
			locRegion = NULL;
			//locCharset = NULL;
			//locModifier = NULL;
			for (char *s = lang; *s != '\0'; ++s) {
				switch (*s) {
					case '_':
						*s = '\0';
						locRegion = s + 1;
						break;
					case '.':
						*s = '\0';
						//locCharset = s + 1;
						break;
					case '@':
						*s = '\0';
						//locModifier = s + 1;
						break;
				}
			}

			locMatched = false;
			EnumSystemLocales(SetLocaleProc, LCID_SUPPORTED);
			if (!locMatched) {
				//TODO: Log that we didn't find any matching language.
				ASSERT(FALSE);
				SetThreadLocale(LOCALE_SYSTEM_DEFAULT);
			}
		}

		free(lang);
#	endif
}

/**
 * Retrieve the list of monitors.
 * @return A shared pointer of monitor info (never @c NULL, never empty).
 */
boost::shared_ptr<OS::monitors_t> OS::GetMonitors()
{
	boost::shared_ptr<monitors_t> retv(new monitors_t());
#	ifdef _WIN32
		for (int i = 0; ; ++i) {
			DISPLAY_DEVICE devInfo;
			memset(&devInfo, 0, sizeof(devInfo));
			devInfo.cb = sizeof(devInfo);
			if (!EnumDisplayDevices(NULL, i, &devInfo, 0)) break;
			
			DISPLAY_DEVICE monInfo;
			memset(&monInfo, 0, sizeof(monInfo));
			monInfo.cb = sizeof(monInfo);
			if (EnumDisplayDevices(devInfo.DeviceName, 0, &monInfo, 0)) {
				retv->push_back(Monitor());
				Monitor &monitor = retv->back();
				monitor.primary = (devInfo.StateFlags & DISPLAY_DEVICE_PRIMARY_DEVICE) > 0;
				monitor.name = str(format("%d. %s") % (i + 1) % monInfo.DeviceString);
				
				// Retrieve the supported resolutions.
				for (int j = 0; ; ++j) {
					DEVMODE modeInfo;
					memset(&modeInfo, 0, sizeof(modeInfo));
					modeInfo.dmSize = sizeof(modeInfo);
					if (!EnumDisplaySettings(devInfo.DeviceName, j, &modeInfo)) break;
					monitor.resolutions.insert(Resolution(modeInfo.dmPelsWidth, modeInfo.dmPelsHeight));
				}
			}
		}

		return retv;
#	else
		//TODO
		throw std::exception();
#	endif
}

/**
 * Free a memory buffer created by a function from this class.
 * @param buf The buffer to free (may be NULL).
 */
void OS::Free(void *buf)
{
	free(buf);
}
