#include "framework/data.h"
#include "framework/framework.h"
#include "game/state/gamestate.h"
#include "game/state/rules/doodadtype.h"
#include "game/state/shared/agent.h"
#include "library/strings_format.h"
#include "tools/extractors/common/doodads.h"
#include "tools/extractors/extractors.h"
#include <limits>

namespace OpenApoc
{

void InitialGameStateExtractor::extractDoodads(GameState &state) const
{
	static const int frameTTL = 1;

	// DOODADS UFO2P
	{
		static const std::vector<Vec2<int>> doodadTabOffsets = {
		    {0, 4},     {4, 8},     {8, 28},    {28, 29},   {29, 42},
		    {42, 74},   {74, 84},   {84, 90},   {90, 96},   {96, 102},
		    {102, 108}, {108, 114}, {114, 126}, {126, 143}, {143, 160},
		};

		for (int i = 1; i <= 15; i++)
		{
			auto d = mksp<DoodadType>();
			UString doodad_id;
			bool slow = false;
			switch (i)
			{
				case UFO_DOODAD_1:
					doodad_id = "DOODAD_1_AUTOCANNON";
					break;
				case UFO_DOODAD_2:
					doodad_id = "DOODAD_2_AIRGUARD";
					break;
				case UFO_DOODAD_3:
					doodad_id = "DOODAD_3_EXPLOSION";
					break;
				case UFO_DOODAD_4:
					doodad_id = "DOODAD_4_BLUEDOT";
					break;
				case UFO_DOODAD_5:
					doodad_id = "DOODAD_5_SMOKE_EXPLOSION";
					slow = true;
					break;
				case UFO_DOODAD_6:
					doodad_id = "DOODAD_6_DIMENSION_GATE";
					slow = true;
					d->repeatable = true;
					break;
				case UFO_DOODAD_7:
					doodad_id = "DOODAD_7_JANITOR";
					break;
				case UFO_DOODAD_8:
					doodad_id = "DOODAD_8_LASER";
					break;
				case UFO_DOODAD_9:
					doodad_id = "DOODAD_9_PLASMA";
					break;
				case UFO_DOODAD_10:
					doodad_id = "DOODAD_10_DISRUPTOR";
					break;
				case UFO_DOODAD_11:
					doodad_id = "DOODAD_11_SUBVERSION_BIG";
					break;
				case UFO_DOODAD_12:
					doodad_id = "DOODAD_12_SUBVERSION_SMALL";
					break;
				case UFO_DOODAD_13:
					doodad_id = "DOODAD_13_SMOKE_FUME";
					d->repeatable = true;
					break;
				case UFO_DOODAD_14:
					doodad_id = "DOODAD_14_INFILTRATION_BIG";
					break;
				case UFO_DOODAD_15:
					doodad_id = "DOODAD_15_INFILTRATION_SMALL";
					break;
			}

			auto tabOffsets = doodadTabOffsets[i - 1];

			d->imageOffset = CITY_IMAGE_OFFSET;
			d->lifetime = (tabOffsets.y - tabOffsets.x) * frameTTL * (slow ? 2 : 1);
			for (int j = tabOffsets.x; j < tabOffsets.y; j++)
			{

				d->frames.push_back(
				    {fw().data->loadImage(format("PCK:xcom3/ufodata/ptang.pck:xcom3/ufodata/"
				                                 "ptang.tab:%d",
				                                 j)),
				     frameTTL * (slow ? 2 : 1)});
			}

			state.doodad_types[doodad_id] = d;
		}
	}

	// DOODADS TACP
	{
		static const std::vector<Vec2<int>> doodadTabOffsets = {
		    {28, 32},   {32, 44},   {44, 52},   {52, 60},   {60, 68},   {115, 126}, {126, 137},
		    {137, 148}, {148, 159}, {159, 170}, {170, 181}, {181, 186}, {186, 192}, {8, 26}};

		for (int i = 16; i <= 29; i++)
		{
			UString id;
			int ttlmult = 1;
			switch (i)
			{
				case CUSTOM_DOODAD_16: // tac 28, 32
					id = "DOODAD_16_BURNING_OBJECT";
					break;
				case CUSTOM_DOODAD_17: // tac 32, 44
					id = "DOODAD_17_FIRE";
					ttlmult = 2;
					break;
				case TAC_DOODAD_18: // tac 44, 52
					id = "DOODAD_18_SMOKE";
					ttlmult = 2;
					break;
				case TAC_DOODAD_19: // tac 52, 60
					id = "DOODAD_19_ALIEN_GAS";
					ttlmult = 2;
					break;
				case TAC_DOODAD_20: // tac 60, 68
					id = "DOODAD_20_STUN_GAS";
					ttlmult = 2;
					break;
				case TAC_DOODAD_21: // tac 115 - 125
					id = "DOODAD_21_AP";
					break;
				case TAC_DOODAD_22: // tac 126 - 136
					id = "DOODAD_22_LASER";
					break;
				case TAC_DOODAD_23: // tac 137 - 147
					id = "DOODAD_23_PLASMA";
					break;
				case TAC_DOODAD_24: // tac 148 - 158
					id = "DOODAD_24_DISRUPTOR";
					break;
				case TAC_DOODAD_25: // tac 159 - 169
					id = "DOODAD_25_DEVASTATOR";
					break;
				case TAC_DOODAD_26: // tac 170 - 180
					id = "DOODAD_26_STUN";
					break;
				case TAC_DOODAD_27: // tac 181 - 185 shield
					id = "DOODAD_27_SHIELD";
					break;
				case TAC_DOODAD_28: // tac 186 - 192
					id = "DOODAD_28_ENZYME";
					break;
				case CUSTOM_DOODAD_29: // tac 8, 26
					id = "DOODAD_29_EXPLODING_TERRAIN";
					break;
			}

			auto tabOffsets = doodadTabOffsets[i - 16];
			auto d = mksp<DoodadType>();

			// For some reason, not equal to other offsets, which are 23,34?
			// d->imageOffset = { 23,32 };
			// Let's try common one
			// FIXME: ENSURE CORRECT
			d->imageOffset = BATTLE_IMAGE_OFFSET;
			d->lifetime = (tabOffsets.y - tabOffsets.x) * frameTTL * ttlmult;
			d->repeatable = false;
			for (int j = tabOffsets.x; j < tabOffsets.y; j++)
			{
				d->frames.push_back(
				    {fw().data->loadImage(format("PCK:xcom3/tacdata/ptang.pck:xcom3/tacdata/"
				                                 "ptang.tab:%d",
				                                 j)),
				     frameTTL * ttlmult});
			}

			state.doodad_types[id] = d;
		}

		// CUSTOM_DOODAD_30 30 // tac 78, 77
		{
			UString id = "DOODAD_30_EXPLODING_PAYLOAD";
			auto d = mksp<DoodadType>();

			// FIXME: ENSURE CORRECT
			d->imageOffset = BATTLE_IMAGE_OFFSET;
			d->lifetime = (2) * frameTTL;
			d->repeatable = false;
			d->frames.push_back(
			    {fw().data->loadImage(format("PCK:xcom3/tacdata/ptang.pck:xcom3/tacdata/"
			                                 "ptang.tab:%d",
			                                 78)),
			     frameTTL});
			d->frames.push_back(
			    {fw().data->loadImage(format("PCK:xcom3/tacdata/ptang.pck:xcom3/tacdata/"
			                                 "ptang.tab:%d",
			                                 77)),
			     frameTTL});
			state.doodad_types[id] = d;
		}

		// CUSTOM EXPLOSION DOODADS
		{
			int ttlmult = 2;
			static const std::vector<int> indexes = {68, 79, 88, 97, 106};

			static const std::map<int, UString> facingMap = {
			    {0, "00"}, // North West
			    {1, "01"}, // West
			    {2, "10"}, // North
			    {3, "02"}, // South West
			    {4, "11"}, // Center
			    {5, "20"}, // North East
			    {6, "12"}, // South
			    {7, "21"}, // East
			    {8, "22"}, // South East

			    // NW W N SW C NE S E SE

			};

			for (int facing = 0; facing < 9; facing++)
			{
				UString id = format("DOODAD_BATTLE_EXPLOSION_%s", facingMap.at(facing));
				auto d = mksp<DoodadType>();

				// FIXME: ENSURE CORRECT
				d->imageOffset = BATTLE_IMAGE_OFFSET;
				d->lifetime = (int)indexes.size() * frameTTL * ttlmult;
				d->repeatable = false;

				for (int frame = 0; frame < (int)indexes.size(); frame++)
				{
					int idx = indexes[frame] + facing;

					d->frames.push_back(
					    {fw().data->loadImage(format("PCK:xcom3/tacdata/ptang.pck:xcom3/tacdata/"
					                                 "ptang.tab:%d",
					                                 idx)),
					     frameTTL * ttlmult});
				}

				state.doodad_types[id] = d;
			}
		}
	}
}

} // namespace OpenApoc
