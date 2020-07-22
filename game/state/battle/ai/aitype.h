#pragma once

// --- Read note about serialization at the end of the file ---

namespace OpenApoc
{

// Unit's AI type (AI may check this and act differently based on values here)
enum class AIType
{
	None,        // Chrysalises
	Civilian,    // Civilians
	Loner,       // Poppers, Large units
	Group,       // Majority of the units
	PanicFreeze, // During Panic (the one that makes you freeze in place)
	PanicRun,    // During Panic (the one that makes you drop weapon and run)
	Berserk,     // During Berserk
};
} // namespace OpenApoc

/*
// Alexey Andronov (Istrebitel):

Due to the way OpenAPoc serializes things, we do not know what class is stored in the file.
So when we have a pointer to derived class stored in a pointer to base class, and we serialized it,
we have no way of knowing which class is stored there.
Therefore, every AI has to have a type attached to it that is it's class name, and method in
gamestate_serialize needs to be updated for any new AI introduced.

*/