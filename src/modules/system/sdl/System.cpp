/**
 * Copyright (c) 2006-2023 LOVE Development Team
 *
 * This software is provided 'as-is', without any express or implied
 * warranty.  In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 **/

// LOVE
#include "System.h"
#include "window/Window.h"

// SDL
#include <SDL_clipboard.h>
#include <SDL_cpuinfo.h>
#include <SDL_version.h>

#if SDL_VERSION_ATLEAST(2, 0, 14)
#include <SDL_locale.h>
#endif

namespace love
{
namespace system
{
namespace sdl
{

System::System()
{
}

const char *System::getName() const
{
	return "love.system.sdl";
}

int System::getProcessorCount() const
{
	return SDL_GetCPUCount();
}

bool System::isWindowOpen() const
{
	auto window = Module::getInstance<window::Window>(M_WINDOW);
	return window != nullptr && window->isOpen();
}

void System::setClipboardText(const std::string &text) const
{
	// SDL requires the video subsystem to be initialized and a window to be
	// opened in order for clipboard text to work, on at least some platforms.
	if (!isWindowOpen())
		throw love::Exception("A window must be created in order for setClipboardText to function properly.");

	SDL_SetClipboardText(text.c_str());
}

std::string System::getClipboardText() const
{
	if (!isWindowOpen())
		throw love::Exception("A window must be created in order for getClipboardText to function properly.");

	std::string text("");

	char *ctext = SDL_GetClipboardText();
	if (ctext)
	{
		text = std::string(ctext);
		SDL_free(ctext);
	}

	return text;
}

love::system::System::PowerState System::getPowerInfo(int &seconds, int &percent) const
{
	SDL_PowerState sdlstate = SDL_GetPowerInfo(&seconds, &percent);

	PowerState state = POWER_UNKNOWN;
	powerStates.find(sdlstate, state);

	return state;
}

std::vector<std::string> System::getPreferredLocales() const
{
	std::vector<std::string> result;

#if SDL_VERSION_ATLEAST(2, 0, 14)
	SDL_Locale *locales = SDL_GetPreferredLocales();

	if (locales)
	{
		for (SDL_Locale* locale = locales; locale->language != nullptr; locale++)
		{
			if (locale->country)
				result.push_back(std::string(locale->language) + "_" + std::string(locale->country));
			else
				result.push_back(locale->language);
		}

		SDL_free(locales);
	}
#endif

	return result;
}

EnumMap<System::PowerState, SDL_PowerState, System::POWER_MAX_ENUM>::Entry System::powerEntries[] =
{
	{System::POWER_UNKNOWN, SDL_POWERSTATE_UNKNOWN},
	{System::POWER_BATTERY, SDL_POWERSTATE_ON_BATTERY},
	{System::POWER_NO_BATTERY, SDL_POWERSTATE_NO_BATTERY},
	{System::POWER_CHARGING, SDL_POWERSTATE_CHARGING},
	{System::POWER_CHARGED, SDL_POWERSTATE_CHARGED},
};

EnumMap<System::PowerState, SDL_PowerState, System::POWER_MAX_ENUM> System::powerStates(System::powerEntries, sizeof(System::powerEntries));

} // sdl
} // system
} // love
