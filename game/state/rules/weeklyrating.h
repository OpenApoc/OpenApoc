#pragma once

namespace OpenApoc
{

class WeeklyRating
{
  public:
	enum class Type
	{
		Terrible,     // -1600
		Bad,          // -800
		Poor,         // -400
		Questionable, // -399 to 0
		Neutral,      // 0 to 399
		Acceptable,   // 400+
		Satisfying,   // 800+
		Good,         // 1600+
		Great,        // 3200+
		Excellent,    // 6400+
		Outstanding   // 12800+
	};

	static WeeklyRating::Type getRating(int score);
	static int getRatingModifier(Type rating);
};

}; // namespace OpenApoc
