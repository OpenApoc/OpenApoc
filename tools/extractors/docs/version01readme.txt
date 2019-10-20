OpenApoc 0.1 Playable Alpha


I. Intro:

This is the first playable release of OpenApoc. The aim of this release is to showcase project progress so far and allow the community to try the game and give feedback. The game is by no means yet fully playable, however, certain systems are partially or fully implemented and can be tried out and playtested. Please continue reading to find out what can you expect to see in this version and what can you playtest.


II: Cityscape

Cityscape is barely functional right now. Random vehicles fly around and you can equip your vehicles and send them to fly around or fire at stuff. Not much works properly but vehicles should fly around properly, move from building to building properly, attack properly. At night, UFOs appear, and they should fly, deposit aliens and retreat. Attacks should properly hit and deal damage. Vehicle AI is very basic right now and does not represent how it works in vanilla yet (like, vehicles won't "dogfight", won't "dodge" etc., will just fly face on and make attacks). Building destruction algorithm is not fully implemented yet.

There is no way to move agents around yet and no way to initiate missions in a proper way (sending vehicles with agents to ufos or buildings). In order to start a tactical game, use the new Skirmish feature

Do not try to order ground vehicles around as they are not yet implemented and will most likely crash.


III. Battlescape:

Most of the battlescape mechanics are fully implemented. Therefore, it is easier to list what is not yet implemented or not yet properly implemented.

List of unimplemented battlescape features:
- Unit behavior (Cautious and Normal units automatically moving into cover, taking potshots from cover and so on)
- Music (right now just a loop of several tracks plays in the background)

List of incomplete battlescape features:
- Proper drawing of large and moving units (some graphical glitches may happen when displaying them)
- Proper alien AI that mimics Vanilla (current AI is a simple placeholder that acts in a basic way)
- Proper enzyme (current implementation is an approximation but may be far from how vanilla works)
- Proper fire (current implementation is an approximation but may be far from how vanilla works)
- Proper cloaking (current implementation is an approximation but may be far from how vanilla works)
- Disabling notifications does not work (even if you uncheck "pause when this happens" the dialog will pop up anyway)
- Equipping multiple agents at once
- Edit agent names
- Viewing agent medals and mission stats

Debug features:
- F1 toggles between cheat hotkeys and normal hotkeys (for anything from this list to work, press F1 once, to be back to using normal hotkeys again, press F1 again)
- Middle click to activate teleportation for any unit (even w/o a teleport in hand)
- "e" will force end current turn in TB mode
- "r" will reveal whole map and show debug lines for which unit sees which unit
- "s" to stun units 
  - with Ctrl held will affect small area around cursor, without will affect only unit under cursor
  - with Shift held will affect everything except cursor
  - with Shift and Ctrl held will affect everything except what's in small area around cursor
- "k" same as "s" but removes units from map (units count as reterated)
- "p" lowers morale of every unit to the point where they will eventually suffer a low morale event
- Shift + "p" gives every unit 0 psi defense and 100 psi energy/attack
- "h" restores stats of every unit, heals stun damage and fatal wounds
- "t" restores TU of every player unit
- "n" will toggle notification popups

Hotkeys, less known features and new stuff in OpenApoc:
- Right click a grenade to prime it to explode on contact
- Right click enemy in RT to make soldier focus that unit
- Space to start/stop time (including TB!)
- Page Up/Down to go Up/Down levels
- Tab to go in/out of map mode
- Ctrl + Click to add agent to selection
- Shift + Click to fire at target tile's center
- Shift + Alt + Click to fire at target tile's ground
- Alt + Click to move while keeping facing (agent will starfe or move backwards)
- "j" while cursor is over adjacent tile orders unit to jump there (allowing unit to drop down from a cliff) (note that flying units cannot jump)
- Units can jump over small gaps if there's no quick way around
- Units with special functions (popper suicicde, brainsucker attack, multiworm burst) now have an item their hands so that player can activate it manually if player mind controls them (might require psi debug key because many of them are psi immune in vanilla)

AI:
- units should act aggresively, patrol map, attack when seeing enemies, throwing grenades if AI assumes that will do more damage faster than using their weapon, and avoid using AOE attacks if friendly fire is likely to happen.


IV: Skirmish

This will eventually become a "quick combat" feature like the one OpenXCom has, but for now it's a way to initiate some tactical missions. To access it, go into options menu from cityscape and click the button in the top right.

1. First choose a map. Choose a base, UFO, alien building or city building map. 

2. Next you can specify your squad and other settings.
- Specify number of agents you will have (Human agents, Hybrids, Androids). Note that you can have at max 36 agents at your command.
- Specify numer of training days your agents will receive (this allows you to control what stats your agents have - do you want a weak squad or endgame supersoldiers)
- "Default equipment tech level" will equip your soldiers in same manner that organisation gets equipped in vanilla. This setting goes from 1 to 12, and you get normal starting weapons at 1 and personal shields with disruptors at 12. "NO" will not equip your soldiers with any items 
- "Alien score level" is the way vanilla chooses alien equipment. It depends on player score - the higher the player score, the better gear aliens have. This allows you to choose your score and thus choose alien equipment.
- "Organisation tech level" controls what equipment organisation forces receive.
- "Default equipped armor" gives your units armor of specific kind. You can choose any armor set (megapol,marsec,x-com) and you can choose to mix any armor set with marsec body
- "Customize forces" will allow you to specify what kind of aliens you will face, as well as building security and civilians
- "Raid mode" will make you raid organisation if you pick a human building location. Without this it is assumed you are in "Alien extermination" mode
- "Hotseat multiplayer" lets you control other organisations if you choose Turn Based mode

3. Next you will get an option to specify other battle forces. The "Customize Forces" window will open if you either checked the relevant box, or if you chose alien extermination but building has no aliens in it.

If target location already contains aliens (i.e. UFO crash or building with aliens) then you can either leave aliens as "Default" or uncheck that checkbox. If the checkbox is unchecked you can specify how many of every alien species will appear in the mission. 

In the same way, you can specify amount of guards and civilians that will appear in the mission. Note that civilians can't spawn if map provides no spawn points for them, neither can they spawn if it's a human building and organisation is hostile to player

You can click "Esc" to go back to the previous screen

4. Finally, you will see a typical briefing screen (briefing text does not work yet) and be able to choose the mode (RT/TB). Then you will see a typical squad assignment screen. Again, assigning squad does not work yet, however, you can click the inventory button in the top right and equip your agents with whatever equipment you want.

V. What to playtest in general:
- Game should run smoothly. If something takes a long time or acts sluggishly, if performance is not really adequate (like, moving one unit around the map makes the game hand up every now and then), please report it.

VI. What to playtest in Battlescape:
- Try different maps, different modes (raids and alien exterminations). They should all generate properly and spawn guards/aliens/civilians properly (except alien dimension where units can spawn in wrong places, but maps should properly spawn)
- Try finishing missions. They should properly finish, like, if all your units retreat or you kill all aliens or you destroy everything in alien building
- Try moving around, run, prone, jump. Everything should work properly.
- Try fighting enemies. They should behave reasonably - fire back, go prone when under heavy fire, suiciders should run towards you, suckers should try to brainsuck non-androids etc.

VII. What to playtest in Cityscape:
- Try building base facilities
- Try researching and manufacturing
- Equip your craft with different weapons
- Fly around shooting at other flyers
- Wait for night and attack alien ufos
- If ufos succeeed at depositing aliens click building to find out its name. Then launch skirmish using this building as location and you should get aliens spawned on the map. Alternatively let time pass and eventually you should get the alert 

VII: Bug reports:

If you encounter a bug, please make a bug report at our forums. At the very least please provide an error message and description of what you did to get it. If you are willing to help us, however, please try to reproduce the bug and give us exact instructions on how to get to it.

The way random generator works right now, things should go the same way every time you launch the game. That means, if you start game, start specific difficulty game, immediately go into skirmish screen immediately when loading finishes, pick a specific map, specify specific settings, start it, do something specific and encounter a bug, then if you repeat the same actions, then same bug should repeat itself, because same actions should produce same results. 

For example, if generating an Assault Ship map fails when you choose it to have 60 Anthropods, it should fail every time.

Just make sure to re-start the game every time (to reset the random generator) and make sure to go into skirmish immediately before any vehicle starts flying around the map (that is, if the bug is in battlescape).