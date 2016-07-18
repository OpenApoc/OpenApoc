#include "game/state/gametime.h"

namespace OpenApoc
{
UString GameTime::getTimeString() const
{
	auto seconds = ticks / TICKS_PER_SECOND;
	auto minutes = seconds / 60;
	auto hours = minutes / 60;

	unsigned secondsClamped = seconds % 60;
	unsigned minutesClamped = minutes % 60;
	unsigned hoursClamped = hours % 24;

	return UString::format("%02u:%02u:%02u", hoursClamped, minutesClamped, secondsClamped);
}

unsigned int GameTime::getDay() const { return (unsigned int)(ticks / TICKS_PER_DAY); }

unsigned int GameTime::getHours() const { return getDayTime() / TICKS_PER_HOUR; }

unsigned int GameTime::getMinutes() const { return getHours() / TICKS_PER_MINUTE; }

unsigned int GameTime::getDayTime() const { return (unsigned int)(ticks % TICKS_PER_DAY); }

unsigned int GameTime::getTicks() const { return ticks; }

bool GameTime::dayPassed() const { return dayPassedFlag; }

bool GameTime::weekPassed() const { return weekPassedFlag; }

void GameTime::clearFlags()
{
	dayPassedFlag = false;
	weekPassedFlag = false;
}

void GameTime::addTicks(unsigned int ticks)
{
	this->ticks += ticks;
	unsigned int dayTicks = this->ticks % TICKS_PER_DAY;
	if (dayTicks < ticks)
	{
		unsigned int days = this->ticks / TICKS_PER_DAY;
		dayPassedFlag = true;
		if (days % 7 == 0)
		{
			weekPassedFlag = true;
		}
	}
}

GameTime GameTime::midday() { return GameTime(TICKS_PER_HOUR * 12); }
}