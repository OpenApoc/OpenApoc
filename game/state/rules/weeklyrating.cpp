#include "game/state/rules/weeklyrating.h"
#include "game/state/gamestate.h"

namespace OpenApoc
{

WeeklyRating::Type WeeklyRating::getRating(int score)
{
	if (score < -1600)
	{
		return WeeklyRating::Type::Terrible;
	}
	else if (score < -800)
	{
		return WeeklyRating::Type::Bad;
	}
	else if (score < -400)
	{
		return WeeklyRating::Type::Poor;
	}
	else if (score < 0)
	{
		return WeeklyRating::Type::Questionable;
	}
	else if (score > 12800)
	{
		return WeeklyRating::Type::Outstanding;
	}
	else if (score > 6400)
	{
		return WeeklyRating::Type::Excellent;
	}
	else if (score > 3200)
	{
		return WeeklyRating::Type::Great;
	}
	else if (score > 1600)
	{
		return WeeklyRating::Type::Good;
	}
	else if (score > 800)
	{
		return WeeklyRating::Type::Satisfying;
	}
	else if (score > 400)
	{
		return WeeklyRating::Type::Acceptable;
	}
	return WeeklyRating::Type::Neutral;
}

int WeeklyRating::getRatingModifier(Type rating)
{
	switch (rating)
	{
		case OpenApoc::WeeklyRating::Type::Terrible:
			return -4;
		case OpenApoc::WeeklyRating::Type::Bad:
			return -5;
		case OpenApoc::WeeklyRating::Type::Poor:
			return -10;
		case OpenApoc::WeeklyRating::Type::Questionable:
			return -15;
		case OpenApoc::WeeklyRating::Type::Acceptable:
			return 20;
		case OpenApoc::WeeklyRating::Type::Satisfying:
			return 16;
		case OpenApoc::WeeklyRating::Type::Good:
			return 12;
		case OpenApoc::WeeklyRating::Type::Great:
			return 8;
		case OpenApoc::WeeklyRating::Type::Excellent:
			return 5;
		case OpenApoc::WeeklyRating::Type::Outstanding:
			return 4;
		default:
			break;
	}
	return 0;
}
}; // namespace OpenApoc
