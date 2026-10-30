
# Heracles <!-- omit in toc -->

Hey and welcome, thanks for stopping by!

**Heracles** is an active continuation fork of [saturn-xvi's Prometheus](https://github.com/saturn-xvi/Prometheus), a preservation and reverse-engineering project for the **Overwatch 0.8.0 beta (Build 24919, October 2015)**. Since the original Prometheus project has been stalled for a while, Heracles picks up the torch to make the 2015 client genuinely playable offline.

*(Note: Core project files and internal classes still use the `prometheus` name for now).*

- [1. Usage](#1-usage)
  - [Download a Release](#download-a-release)
    - [Linux instructions](#linux-instructions)
    - [Optional stuff](#optional-stuff)
    - [Patcher \& how it works](#patcher--how-it-works)
  - [Compilation](#compilation)
- [2. Game internals](#2-game-internals)
  - [Managers](#managers)
  - [ECS](#ecs)
  - [STU](#stu)
  - [DataFlow](#dataflow)
  - [Game Messages](#game-messages)
  - [Components](#components)
    - [STUPvPGameComponent 0x24](#stupvpgamecomponent-0x24)
    - [STUStatescriptComponent 0x23](#stustatescriptcomponent-0x23)
    - [Component 1 (SceneRendering)](#component-1-scenerendering)
    - [MovementStateSystem](#movementstatesystem)
- [3. What Heracles fixed \& what's still broken](#3-what-heracles-fixed--whats-still-broken)
  - [What works now](#what-works-now)
  - [Still broken / WIP](#still-broken--wip)
- [4. Tips \& Tricks](#4-tips--tricks)
- [5. Contributions Welcome!](#5-contributions-welcome)
- [6. NOTE](#6-note)
- [7. Open Source libraries used](#7-open-source-libraries-used)
- [8. License and Credits](#8-license-and-credits)

# 1. Usage

If you haven't already, download the Overwatch Beta build we target from [Hidden Palace](https://hiddenpalace.org/Overwatch_(Oct_27,_2015_prototype)) and extract the files somewhere.
* ⚠️ Make sure that you don't download any malicious executable and verify that `GameClientApp.exe` is signed by Blizzard. The signature will get broken after applying that patch - but musn't be before.
* You can optionally remove the `BlizzardError` directory.

## Download a Release

* Download a `release.zip` and extract all files to the game directory.
* Execute `patcher.exe`. It will ask for an input `GameClientApp.exe` - select the one you downloaded and verified.
  * The patcher will write a `GameClientApp.patched.exe` file into the same directory. The only thing this patched executable does is load `inject.dll` before running the game code itself.
  * You only need to do this once; this patch is version-independent.
* Rename `prometheus.dll` to `inject.dll`.
* Launch `GameClientApp.patched.exe`. 

### Linux instructions

* Set the Windows version of the Wine prefix to Windows 10.
  * `WINEPREFIX=~/.wine64/ WINEARCH=win64 winecfg`
* Install the .NET 8 runtime. (For patcher only)
  * `WINEPREFIX=~/.wine64/ WINEARCH=win64 winetricks dotnet8`
* Install the .NET 8 desktop framework. (For patcher only)
  * `WINEPREFIX=~/.wine64/ WINEARCH=win64 winetricks dotnetdesktop8`
* Select the unpatched executable.
  * `WINEPREFIX=~/.wine64/ WINEARCH=win64 wine patcher.exe`
* Download Proton from [GloriousEggroll](https://github.com/GloriousEggroll/proton-ge-custom) and install [umu-launcher](https://github.com/Open-Wine-Components/umu-launcher). Launch the patched executable using umu.
  * `WINEPREFIX=~/.wine64/ WINEARCH=win64 PROTONPATH="/home/tracer/GE-Proton10-25/" umu-run GameClientApp.patched.exe`
  
**Please note that the Linux build instructions have remained as-is from the original fork - I do not test building on Linux**

### Optional stuff

* Download the [MonaspaceXenon](https://monaspace.githubnext.com/) font and put the `-Regular.otf` and `-ExtraBold.otf` files in the game directory.
* Download the [Font Awesome v6](https://fontawesome.com/v6/download) free desktop font files and put the `.otf` files into the game directory.
* Once first started, the library will create `hashlibrary.json`. You can add crc32 strings / elements to hash which will be displayed in various places where applicable. You can just add all the strings from the [overtools github repository](https://github.com/overtools/OWLib/tree/develop/TankLibHelper/DataPreHashChange). To do so, add another root JSON element (an array) called `"add"` and put all your strings there.

### Patcher & how it works

The patcher itself just goes to `TlsCallback_0` and patches some bytes so the executable loads `inject.dll` before anything else. `inject.dll` then restores the game to its original state, decrypts everything, emplaces the missing CASC encryption keys, and hooks stuff. Afterwards, it runs the game normally. This was achieved by copying the bytes that `lazy_importer` created for `LoadLibrary` and calling it with `"inject.dll"` as an argument.

## Compilation

* You need Visual Studio 2022 or newer.
* Make sure to initialize the submodules / clone recursively.
* Initialize the vcpkg repository with the `.ps1` file located at `external/vcpkg/scripts/bootstrap.ps1`.
* Compile as `Release/x64`. No other configuration is tested (some flags and settings are missing).

# 2. Game internals

At its core, almost every (in-game) problem with restoring Overwatch 0.8.0 to full playability comes down to one thing: **the client was built to be a puppet controlled by a server - having to ask for permission to do just about anything**. The client has all the models, animations, sounds, physics, and UI it needs, but it is constantly waiting for a server to tell it *"you are allowed to shoot," "that bullet hit someone,"* or *"where is the payload right now?"*. This project is fundamentally all about filling in the seams between what's controlled by the server (or pretends to be as a patch) and what the client tells the server - the game is fundamentally broken simply because of StateScripts and other functionality that was intentionally stripped during the build process.

## Managers
The first thing Overwatch does is initialize all its "Managers". They handle things such as Dataflow, CASC, Window management, etc. This is the lowest level and not really interesting.

## ECS
Afterwards, all the Entity Admins get initialized. First the Lobby, then Game, and finally Replay.

There is one Entity Admin base class on which all others depend. The `LobbyEntityAdmin` includes login, main menu UI, user settings, and an embedded Entity Admin (Component `0x54`) used to render the background map on the main menu.

The `GameEntityAdmin` is the biggest one and contains, amongst others:
* A `uint` that says "this is the local entity". This must reference the **Controller** entity. Systems use Components `0x2F` (local player component) and `0x20` (model reference component) out of that entity to display the local player on screen.
* A JAM virtual function table used to send JAM network messages.
* A table of map callbacks. Systems can subscribe to map change events (for example, if the map state changes from "Global Loading" to "World Loading").
* A virtual function table reference from the `MirrorSystem`, which systems use to subscribe to pre- and post-component deserialization when a server sends updates.
* An Accessor which searches for an Entity with the `PvPGameComponent` (`0x24`) and takes match information from that.

A single player in the `GameEntityAdmin` is actually split across **four layers of entities**:
1. **The Controller ("Brain"):** Reads your mouse/keyboard, tracks what hero you picked (`0x3F`), and holds your camera (`0x2F`, `0x20`).
2. **The 3P Hero Model ("Body"):** The physical hero model that spawns in the world. Holds health (`0x28`), movement/physics (`0x12`, `0x15`), and ability scripts (`0x23`).
3. **The 1P Viewmodel ("Hands"):** The floating first-person arms and guns only you see on your screen (referenced by Component `0x22`).
4. **Weapons & Gun Barrels:** The weapon models attached to your hands (`0x1D`), plus invisible "muzzle" entities attached to the barrel tips where firing particles play (`0x0D`).

## STU

STU is the proprietary structured data / reflection format for Overwatch. It gets used to read data out of CASC archives, and some in-game data gets serialized using this. Everything needed to read STU is located in `STU.h`. If you want to edit STU, include `STU_Editable.h`, but make sure to read the comments so you don't crash ;).

For usage examples, you can have a look in `windows/stu_explorer.h`.

## DataFlow

A pull/publish system used to pipe game data into shaders, effects, and UI. DataFlow members can live both inside and outside of the Entity Component System.

## Game Messages

8-byte GUIDs able to be sent from the client and server and observed by Statescript, with optional additional data. Other systems besides Statescript can also broadcast GameMessages.

## Components

### STUPvPGameComponent 0x24
Holds the current gamemode state, round counters, and overtime timers.

### STUStatescriptComponent 0x23

Statescript is Blizzard's visual scripting engine (very similar to Unreal's Blueprints - and, simmilarly, with obtuse internals) and controls:
* In-game weapons, abilities, and Ultimates (pretty much everything except basic WASD movement)
* User interface interactions and HUD viewmodels
* Login logic
* Music & sound triggers
* Sliding spawn doors, capture points, and health packs

A Statescript component (via its `StatescriptSyncMgr` inside) holds several `StatescriptInstance`s, which run node graphs defined in `STUStatescriptGraph` resources. Scripts read and write variables stored in **VarBags** (dictionaries attached to either the script instance or the entity itself).

In Heracles, we found three major reasons why Statescript was failing - in general:
1. **Missing Client Authority Flags:** Weapon and ability states check an execution bitflag asking *"is this entity the local player's active aim target?"* Without it, firing states started and aborted on the exact same frame (causing i.e. Tracer's guns to just click 5 times a second without shooting).
2. **Negative Client Script IDs:** Online, the server assigns positive IDs (`1, 2, 3...`) to scripts. Offline, client-loaded scripts get **negative IDs** (`-1, -3, -24...`). When the engine packs a negative 16-bit ID into a 13-bit handle and unpacks it without restoring the minus sign, `-24` turns into `+8168` causing muzzle flashes, bullet raycasts, and projectiles to fail their script lookups as well as a host of other downstream issues.
3. **Stripped Server Nodes:** Blizzard stripped the code for several server-only damage, healing, impulse, and entity-spawning nodes from the client executable. We (try our best to) emulate these missing nodes so abilities and Ultimates work offline across all 21 heroes.

### Component 1 (SceneRendering)

Needed if an entity wants to be shown on the map. It holds position, scaling, rotation, visibility flags, and a bitflag (`Bit 16`) that tells the bullet raycaster whether this entity has skeletal bone hitboxes that can be shot (Havok-based).

Interestingly, Component 1 has **two** functions to move an object: one that moves *both* the solid physics collider and the 3D visual mesh, and an internal one used right before rendering a frame that moves *only* the 3D visual mesh.

### MovementStateSystem

(Encompasses `movement_vt`, Movement system(s), Component `0x12` `STUMovementState`, `0x15` `STUCharacterMoverComponent`, and `0x16` `STUSimpleMovementComponent`).

Component `0x12` holds a ring buffer of network movement snapshots from the server to interpolate moving objects smoothly. Offline, that buffer is completely empty. We discovered that when Component `0x12`'s buffer is empty, its fallback code literally writes `(0, 0, 0)` as the object's position every frame - and calls Component 1's *visual-only* move function.

# 3. What Heracles fixed & what's still broken

For further reference for heroes specifically, please check out the TEST_MATRIX.md file in the repo root.

## What works now

* **Weapons shoot, reload, and show muzzle flashes:** Primary and secondary fire, ammo consumption, reloading, and 1P barrel muzzle flashes (mostly) work across the cast. There are still issues - like dual-weapon heroes - but the core logic works.
* **Spawning is reliable:** Spawning uses a deferred state machine that waits for Component `0x23` to finish loading before attaching weapons, so you no longer spawn with missing guns. Spawning natively, without the Player Spawner DeLuxe, has also been thoroughly debugged and should function correctly now.
* **D.Va spawns in her Mech:** Spawning D.Va now puts her directly in her Mech (with 100 HP + 400 Armor) instead of floating around as Pilot D.Va.
* **Pressing `H` to swap heroes works anywhere:** You can press `H` anywhere on any map to open the Hero Select screen and swap heroes in-place at your current position. This is a placeholder for native spawnroom detection.
* **Doors, Capture Points, and Payloads stay in place:** Blocking Component `0x12`'s empty-buffer `(0,0,0)` overwrites keeps all map props, payloads, and doors at their proper map coordinates.
* **Spawn doors slide open and closed:** You can open and close map spawn doors natively (with animations, sound, and collision) from the Player Spawner DeLuxe or the first Diagnostics window.
* **Target Dummies & Full Combat Loop:** You can spawn Enemy or Friendly bots in Player Spawner DeLuxe. Shooting them now hits their actual skeletal bone hitboxes (with headshots, armor reduction, blood/spark VFX, flinch animations, and hurt grunts), shows their overhead enemy healthbar, awards `0% -> 100%` Ultimate charge, and triggers ragdoll knockouts, 3D death screams, red kill skulls, and the `"ELIMINATED [HERO]"` banner on kill.
* **Hero Abilities & Ultimates:** Many of the abilities across the roster already function correctly and reliably. The remaining issues cluster around a couple of very specific problems that generalize fixes across the cluster (targeted abilities, dual-weapon heroes, certain shield interactions and visuals etc.). This is being actively worked on.

## Still broken / WIP

* **Barrier crack visuals (WIP):** Winston's Barrier Projector and Bastion's Frontal Shield spawn, follow, and despawn properly, but their health-ratio shader parameters are still being fixed right now so they don't look cracked immediately on spawn.
* **Main Menu matchmaking buttons:** Main menu play buttons (as well as several other pre-menu items) only send JAM messages to a Battle.net server. Use the top bar workflow (`DP Load` -> `Map`) to load into a map.
* **Gamemode objectives:** Capture point progress rings and payload movement along tracks still need offline gamemode script orchestration, but function visually and, partially, internally as well.
* **"Press H to select Hero" banner:** Controlled by a server flag and will probably stay visible on the HUD after spawning for the time being. Sorry.
* **Communication wheel & dynamic voice lines:** Tracer's spawn voiceline and 3D victim death screams work, but the full server-side communication wheel and contextual voice rules are still a TODO. I think I have most of the architecture nailed down, it just needs some love and several afternoons of work.
* **Map unloading crashes the game:** Just use the **Restart** button in the top bar when switching maps for now - or restart the game entirely.
* **Window resizing:** Resizing the game window messes with viewmodels (a bug in the 0.8 beta itself that may or may not be patch-able when everything else is functional). Borderless Windowed mode works best.

# 4. Tips & Tricks

* **Quick-start in 4 clicks:** Click **`DP Load`** in the top bar -> pick a map under **`Map`** -> open **Game -> Player Spawner DeLuxe** -> pick a hero and click **`Spawn`** (and click **`Open Spawn Doors`** so you aren't locked in spawn yelling door stuck).
* **Hide the debug UI:** Press `~`  to toggle all ImGui windows on/off so you can play with a clean screen.
* **Swap heroes with `H`:** Once you've spawned a hero, just press **`H`** at any time to open the native Hero Select screen and switch heroes on the fly.
* **Spawn Target Dummies:** In **Player Spawner DeLuxe**, click **`Spawn Jeff Kaplan`** to drop a target enemy dummy in front of you to test weapons, headshots, knockbacks, and Ultimates, or **`Spawn Friendly Dummy (Team 0)`** to test ally-based abilities.
* **Teleport & Timescale:** Use the **Up / Down arrow keys** to teleport yourself up or down. Use the **Left / Right arrow keys** to slow down or speed up the physics timescale. This is necessary on... most maps. On Hanamura, specifically just Genji falls through the floor when you try to spawn him. I still don't know why lol.
* **World Map:** Press **`M`** in-game to toggle the overhead map overlay.
* **FreeLookView:** Check out the `FreeLookView` checkbox in Player Spawner DeLuxe to fly around the map with WASD + Shift without needing a hero.
* **Multiplayer ("Awful Demo Server"):** Enable `Demo`, `DP Load`, load a Map, select a hero and go in-game. Then go to **Tools -> Awful Demo Server** and start the server. Open a second instance and connect to the server before selecting a hero. (Currently only syncs player positions and hero models; see `serialization.h` and `state_replicator.h`). Making this work properly is the core aim of this project after things function as-intended offline.

# 5. Contributions Welcome!
We envision a future in which we are able to play any Overwatch version that was released. With help from the community, this isn't just a dream, but a real possibility. Most of the code that's being worked on here is **directly transferrable** between different Overwatch 1 versions. Please help by forking, contributing, opening bug reports, and sharing <3. Remember, great science is always the result of collaboration!

# 6. NOTE

Since this project is actively evolving and reverse-engineered structures are continuously being refined, many bugs, inaccouracies, and edge cases may still exist.

Also don't look into the window manager. It's an abomination. You have been warned.

# 7. Open Source libraries used

* [MinHook](https://github.com/TsudaKageyu/minhook)
* [keystone](https://github.com/keystone-engine/keystone)
* [capstone](https://github.com/capstone-engine/capstone)
* [imgui](https://github.com/ocornut/imgui/)
* [imnodes](https://github.com/BreakingBread0/imnodes/)
* `pe` (Could not find the original repository, please open an issue if you know it!)
* [lazy_importer](https://github.com/JustasMasiulis/lazy_importer)
* [nlohmann_json](https://github.com/nlohmann/json)
* [ixwebsocket](https://github.com/machinezone/IXWebSocket)
* [freetype](https://github.com/freetype/freetype)
* [fast-cpp-csv-parser](https://github.com/ben-strasser/fast-cpp-csv-parser)
* [Monaspace Xenon](https://monaspace.githubnext.com/) (optional)
* [Font Awesome](https://fontawesome.com) (optional)

# 8. License and Credits

* **Original Prometheus Project:** [saturn-xvi / BreakingBread0](https://github.com/BreakingBread0)
* **Contributors:** [tslocum](https://github.com/tslocum), [Hazno-dev](https://github.com/Hazno-dev), [Groot36-yep](https://github.com/Groot36-yep), [EvESpirit](https://github.com/EvESpirit), [BreakingBread0](https://github.com/BreakingBread0)

**AGPL-3.0 License.** Kept under AGPL to ensure this remains a project made by the community, for the community.

![prometheus logo](images/prometheus.png)
