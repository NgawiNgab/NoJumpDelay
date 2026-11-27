# libReverseSigma static feature inventory

## 1. Binary Metadata

- **File:** `/home/user/libReverseSigma.so`, 3,473,688 bytes; SHA-256 `c48da8d725e3515ef80e686c58a6a6fa2178ec7cce6b21d1356f53468d0c5b6f`.
- **Architecture:** ELF64, little-endian AArch64 Android shared object (ET_DYN), SONAME `libReverseSigma.so`. `readelf -h/-S` verified the header and sections. The `file` utility is not installed; no `file` result is claimed.
- **Compiler/linker:** `.comment` records Android clang 22.0.1, Android clang 21.0.0 (including a HomuHomu833 custom build), and LLD 21.0.0. No `.symtab` or DWARF debug data: stripped of ordinary private symbols, but `.dynsym` and standard-library C++ RTTI/vtables remain.
- **Count:** 48 editorially grouped **candidate features**, based on config/strings/state, **not a verified total of registered modules**. There are **293 distinct config keys** in both Ghidra-observed config routines. Some groups could be settings rather than standalone modules, while unrecognized features may remain.
- **Dependencies:** `liblog.so`, `libz.so`, `libandroid.so`, `libEGL.so`, `libGLESv2.so`, `libc.so`, `libm.so`, `libdl.so`. Dear ImGui 1.90.0, nlohmann JSON and shadowhook 2.0.1 strings are present. `JNI_OnLoad` ELF offset `0xb5568`; `GLES3JNIView_init` `0x942fc`; `GLES3JNIView_step` `0x9866c`.
- **Method:** Ghidra 11.2.1 headless, AARCH64:LE:64:v8A, 1,400 MiB heap and SerialGC, persistent project `/home/user/ghidra_projects/ReverseSigma`. 4,725 functions recognized. JSON load `FUN_001950f4`, save `FUN_001986cc` and ImGui-related initialization/shortcut feedback were decompiled. ELF strings, dynamic symbols and RTTI were cross-checked. Exact app module classes and widget exports were not found.
- **Limits:** Main UI function `FUN_001a0958` references many feature globals, but its decompilation failed because the decompiler process died. Therefore actual tabs, widget types, keybindings, hook targets, defaults and full control-to-feature links are **not established**. Ghidra reported unhandled AArch64 TLSDESC relocations and unresolved Android libraries; no runtime verification was performed. Categories and associations below are analytical groupings, not claims of the binary's actual tabs. The first Ghidra run used default `/var/tmp` for its cache before cache configuration was corrected; installation, project, and input were kept under `/home/user`. Subsequent Ghidra cache and temporary storage are configured under `/home/user/analysis`.

## 2. Categorized Module List

**Address convention:** Ghidra loads ELF at image base `0x00100000`; `DAT` is a Ghidra state-variable address, and `FUN` is a Ghidra function address. Subtract `0x100000` for ELF virtual offsets. Every config name also has a string address and loader/saver XREF in the exhaustive appendix. A dash means unconfirmed, not absent. Controls sharing a name stem are plausible associations only, unless specifically described as evidenced. Global XREFs prove use, not hook semantics.

### Combat

| Exact config name | State address / code references | Related keys / controls | Technical note |
|---|---|---|---|
| `killAura` | 0x00457eb4; `FUN_001abb30`, `FUN_001b5240` | killRange, minKillAuraCPS, maxKillAuraCPS, auraType, killAuraShortcut | Attack-related state; same global used by FUN_001abb30 and shortcut feedback. |
| `mobAura` | 0x00457ebc; `FUN_001abb30`, `FUN_001b5240` | mobAuraRange, minMobAuraCPS, maxMobAuraCPS, mobAuraShortcut; no* entity filters | Mob-targeting implied by keys; association of filters inferred. |
| `targetLock` | 0x00457ed4; `FUN_001abb30`, `FUN_001b5240` | targetLockShortcut, rotations, rotationsRange | Target-lock state; aim/rotation relationship inferred from names. |
| `autoClicker` | 0x00457ee8; `FUN_001abb30`, `FUN_001b5240` | minAutoClickerCPS, maxAutoClickerCPS, autoClickerShortcut | Click-rate configuration. |
| `hitBox` | 0x00457eb0; `FUN_001abb30` | hitBoxValue | Hit-box modification implied by name. |
| `antiBot` | 0x002d47d4; `FUN_001abb30` | botList | Bot filtering implied; linkage inferred. |
| `antiInvis` | 0x00457ed0; `FUN_001b4a24` | — | Invisibility-related behavior implied by name. |
| `antiFireball` | 0x00457ec4; `FUN_001abb30` | — | Fireball-related behavior implied by name. |
| `emulateNativeAttack` | 0x00457ee0; `FUN_001abb30`, `FUN_001ae86c` | — | Native attack emulation implied by name. |
| `protectedPlayerCheck` | 0x00458000; `FUN_001abb30` | friendList; hurtCheck | Player-protection / hurt checks; association inferred. |

### Movement

| Exact config name | State address / code references | Related keys / controls | Technical note |
|---|---|---|---|
| `blockFly` | 0x00457fdc; `FUN_001b5240`, `FUN_0020e850` | blockFlyShortcut; blockFlyTypeShortcut; Block Fly Up/Down/Direction/Default | Movement/placement implied; feedback references Block Fly. |
| `ladderFly` | 0x00457ec0; `FUN_001b5240`, `FUN_001abb30` | ladderFlyShortcut | Ladder movement implied; feedback references Ladder Fly. |
| `noclip` | 0x00457ea0; `FUN_001b5240`, `FUN_001b3aec` | noclipShortcut | Collision bypass implied; feedback references Noclip. |
| `noJumpDelay` | 0x00457ee4; `FUN_001abb30` | — | Jump delay-related state. |
| `noSlowDown` | 0x00457e98; `FUN_001b3ab0` | offSlowdownTrigger | Slowdown-related states; association inferred. |
| `velo` | not resolved | velocityType, velocityX, velocityY | Velocity-related state; associations inferred. |
| `freeCam` | 0x00457ff4; `FUN_001b5240`, `FUN_001b16ac` | freeCamSpeed, freeCamShortcut | Camera movement implied; feedback references Free Camera. |
| `look360` | 0x00457ea8; `FUN_001b5240`, `FUN_001b399c` | look360Shortcut; Freelook label | View rotation implied; label association inferred. |

### Visuals / Render

| Exact config name | State address / code references | Related keys / controls | Technical note |
|---|---|---|---|
| `espBlocks` | 0x00457f3c; `FUN_001abb30`, `FUN_001b283c` | espBlocksLine, espBlocksBox; esp* block/container selectors and color vectors | Block ESP implied; multiple block-hook functions reference its global. |
| `espLine` | 0x00457f2c; `FUN_001f61d8` | espBox, espNametag, espHealth, espDistance, espHunger, espPlatform | Overlay selectors; grouping is name-based. |
| `xrayDefault` | 0x00457eec; `FUN_001b5240`, `FUN_001abb30` | xrayDefaultShortcut; xrayCoal, xrayIron, xrayGold, xrayDiamond, xrayEmerald, xrayLapis, xrayRedstone, xrayCopper, xrayNetherite, xrayQuartz, xrayAmethyst, xrayObsidian; xrayCam | X-ray rendering implied; associations inferred. |
| `fullBright` | 0x00457e88; `FUN_001b39dc` | — | Brightness-related state. |
| `noFog` | 0x00457fc0; `FUN_001b3f38` | fogEnable | Fog state; association inferred. |
| `noWeather` | 0x00457f90; `FUN_001b3bf8` | — | Weather rendering behavior implied. |
| `noBlur` | 0x00457ed8; `FUN_001b3b88`, `FUN_001b3bc0` | — | Blur-related state. |
| `noCaveVignette` | 0x00457f1c; `FUN_001b3c60` | — | Cave vignette state. |
| `zoom` | not resolved | zoomShortcut | Zoom state; global not recovered by the simple extractor. |
| `overrideRender` | 0x00457ffc; `FUN_001b3f7c`, `FUN_001b16ac` | chunksOverride | Render/chunk overriding implied; association inferred. |
| `seeHiddenBlocks` | 0x00457fe4; `FUN_001b5240`, `FUN_00198238` | — | Hidden-block visibility implied. |

### Building

| Exact config name | State address / code references | Related keys / controls | Technical note |
|---|---|---|---|
| `scaffold` | 0x00457fcc; `FUN_001abb30`, `FUN_001b5240` | scaffoldType, scaffoldShortcut; Save Y and Save Y (Air Place) strings | Placement/scaffold implied; same state referenced in FUN_001abb30. |
| `autoTool` | 0x00458004; `FUN_001b40d0`, `FUN_001b414c` | — | Tool-selection behavior implied; global also referenced by FUN_001b40d0 and others. |
| `noBreakDelay` | 0x00457fbc; `FUN_001b352c` | — | Block breaking delay implied. |

### Player / World

| Exact config name | State address / code references | Related keys / controls | Technical note |
|---|---|---|---|
| `autoFish` | 0x00457fd4; `FUN_001abb30`, `FUN_001b4844` | — | Fishing implied; item.fishing_rod is referenced by FUN_001abb30, but linkage not established. |
| `invCleaner` | 0x00457fd8; `FUN_001abb30` | clearItemList | Inventory cleanup implied; association inferred. |
| `killItems` | 0x00457eb8; `FUN_001b5240`, `FUN_001abb30` | killItemsShortcut; Kill Near Items label | Nearby-item behavior implied; feedback references label. |
| `fastDrop` | 0x00457e90; `FUN_001b23bc` | — | Item-dropping behavior implied. |
| `instantRespawn` | 0x00458010; `FUN_001b3ff0` | — | Respawn behavior implied. |
| `offDrown` | 0x00457eac; `FUN_001b3b58` | — | Drowning-related state. |
| `noCamSleep` | 0x00457e94; `FUN_001b3a44` | — | Sleep-camera-related state. |
| `stayOnline` | 0x0045801c; `FUN_001b4a68` | — | Stay-online behavior implied. |
| `deathPosition` | 0x00457ecc; `FUN_001abb30` | playersDeathPositionsBDS | Death-position persistence implied; association inferred. |

### Exploits / Other

| Exact config name | State address / code references | Related keys / controls | Technical note |
|---|---|---|---|
| `blink` | 0x00458020; `FUN_001abb30`, `FUN_001b5240` | blinkShortcut | Blink state; same global used in several other functions and feedback. |
| `serverBlink` | 0x00458014; `FUN_001b5240`, `FUN_001b3ff0` | serverBlinkShortcut | Network/server behavior implied; Server Blink appears in feedback. |
| `hotspotMultuplayerFix` | 0x00457fe8; `FUN_001b5240`, `FUN_00198238` | myMaxPlayers | Multiplayer compatibility implied; association inferred; spelling as embedded. |
| `panic` | 0x00457f68; `FUN_001b5240`, `FUN_00198238` | Panic label | Emergency toggle implied; label occurs in data table, main-tab exposure unproven. |

### UI / HUD

| Exact config name | State address / code references | Related keys / controls | Technical note |
|---|---|---|---|
| `showPosition` | 0x00457ec8; `FUN_001b3ff0` | — | Position display implied; global referenced by FUN_001b3ff0. |
| `showPaperdoll` | 0x00457edc; `FUN_001b2258` | — | Paper-doll display implied. |
| `physicalHotkeys` | not resolved | Android KEYCODE_*; shortcut enable, Pos.x/y and Size.x/y fields | Hotkey configuration array; exact per-feature bindings not recovered. |

### Complete config-key cross-check

The following is the entire set of distinct strings directly referenced by `FUN_001950f4` as config keys. The matching save function also references all 293. String offset is an ELF `.rodata` file/virtual offset; state addresses are best-effort results from the decompiled load routine. “—” means the simple mapping could not resolve a state global. Presence in config does **not** prove a UI tab, keybind, active hook, or a standalone module.

| Exact config key | String offset | State (Ghidra address) |
|---|---:|---:|
| `antiBot` | `0x408cb` | `0x002d47d4` |
| `antiFireball` | `0x436d3` | `0x00457ec4` |
| `antiInvis` | `0x3d3ae` | `0x00457ed0` |
| `auraType` | `0x3d6f6` | `0x00458030` |
| `autoClicker` | `0x408b9` | `0x00457ee8` |
| `autoClickerShortcut` | `0x3df27` | `0x00457f9c` |
| `autoClickerShortcutPos.x` | `0x418a0` | `0x002d4990` |
| `autoClickerShortcutPos.y` | `0x3fcd9` | `0x002d4994` |
| `autoClickerShortcutSize.x` | `0x41fa5` | `0x002d4998` |
| `autoClickerShortcutSize.y` | `0x4054e` | `0x002d499c` |
| `autoFish` | `0x40538` | `0x00457fd4` |
| `autoTool` | `0x41f3d` | `0x00458004` |
| `blink` | `0x410ad` | `0x00458020` |
| `blinkShortcut` | `0x3f497` | `0x00458024` |
| `blinkShortcutPos.x` | `0x41855` | `0x002d4a10` |
| `blinkShortcutPos.y` | `0x3b256` | `0x002d4a14` |
| `blinkShortcutSize.x` | `0x3ca95` | `0x002d4a18` |
| `blinkShortcutSize.y` | `0x42f00` | `0x002d4a1c` |
| `blockBedColor.x` | `0x42776` | `0x002d490c` |
| `blockBedColor.y` | `0x4015e` | `0x002d4910` |
| `blockBedColor.z` | `0x42b54` | `0x002d4914` |
| `blockBrewingStandColor.x` | `0x42b64` | `0x002d48ec` |
| `blockBrewingStandColor.y` | `0x3d32b` | `0x002d48f0` |
| `blockBrewingStandColor.z` | `0x3e7db` | `0x002d48f4` |
| `blockCreakingHeartColor.x` | `0x4279e` | `0x002d489c` |
| `blockCreakingHeartColor.y` | `0x3e7f4` | `0x002d48a0` |
| `blockCreakingHeartColor.z` | `0x3ba3c` | `0x002d48a4` |
| `blockDispenserColor.x` | `0x4155e` | `0x002d48cc` |
| `blockDispenserColor.y` | `0x3d344` | `0x002d48d0` |
| `blockDispenserColor.z` | `0x3f8c1` | `0x002d48d4` |
| `blockEnchantingTableColor.x` | `0x41f75` | `0x002d48dc` |
| `blockEnchantingTableColor.y` | `0x4016e` | `0x002d48e0` |
| `blockEnchantingTableColor.z` | `0x42b7d` | `0x002d48e4` |
| `blockFly` | `0x3f4ba` | `0x00457fdc` |
| `blockFlyShortcut` | `0x3ada2` | `0x00457fe0` |
| `blockFlyShortcutPos.x` | `0x41f46` | `0x002d49d0` |
| `blockFlyShortcutPos.y` | `0x410ce` | `0x002d49d4` |
| `blockFlyShortcutSize.x` | `0x3b67b` | `0x002d49d8` |
| `blockFlyShortcutSize.y` | `0x4275f` | `0x002d49dc` |
| `blockFlyTypeShortcutPos.x` | `0x3b269` | `0x002d49e0` |
| `blockFlyTypeShortcutPos.y` | `0x3ba0b` | `0x002d49e4` |
| `blockFlyTypeShortcutSize.x` | `0x42744` | `0x002d49e8` |
| `blockFlyTypeShortcutSize.y` | `0x410b3` | `0x002d49ec` |
| `blockFurnaceColor.x` | `0x4188c` | `0x002d48ac` |
| `blockFurnaceColor.y` | `0x3adb3` | `0x002d48b0` |
| `blockFurnaceColor.z` | `0x41f91` | `0x002d48b4` |
| `blockHopperColor.x` | `0x3f00e` | `0x002d48bc` |
| `blockHopperColor.y` | `0x3f4d9` | `0x002d48c0` |
| `blockHopperColor.z` | `0x3fcc6` | `0x002d48c4` |
| `blockMobSpawnerColor.x` | `0x3b2f6` | `0x002d485c` |
| `blockMobSpawnerColor.y` | `0x3ae09` | `0x002d4860` |
| `blockMobSpawnerColor.z` | `0x405c5` | `0x002d4864` |
| `blockSculkShriekerColor.x` | `0x4326c` | `0x002d488c` |
| `blockSculkShriekerColor.y` | `0x4110b` | `0x002d4890` |
| `blockSculkShriekerColor.z` | `0x3fd21` | `0x002d4894` |
| `blockShelfColor.x` | `0x423bf` | `0x002d48fc` |
| `blockShelfColor.y` | `0x40d6c` | `0x002d4900` |
| `blockShelfColor.z` | `0x43687` | `0x002d4904` |
| `blockTrialSpawnerColor.x` | `0x3f04c` | `0x002d486c` |
| `blockTrialSpawnerColor.y` | `0x42bdc` | `0x002d4870` |
| `blockTrialSpawnerColor.z` | `0x43286` | `0x002d4874` |
| `blockVaultColor.x` | `0x3d35a` | `0x002d487c` |
| `blockVaultColor.y` | `0x40d95` | `0x002d4880` |
| `blockVaultColor.z` | `0x3b2e4` | `0x002d4884` |
| `botList` | `0x3b340` | — |
| `chunksOverride` | `0x4187d` | `0x002d479c` |
| `clearItemList` | `0x3ba25` | — |
| `containerBarrelColor.x` | `0x4019f` | `0x002d482c` |
| `containerBarrelColor.y` | `0x3d37c` | `0x002d4830` |
| `containerBarrelColor.z` | `0x41fd0` | `0x002d4834` |
| `containerChestColor.x` | `0x40da7` | `0x002d47fc` |
| `containerChestColor.y` | `0x3ce02` | `0x002d4800` |
| `containerChestColor.z` | `0x3b30d` | `0x002d4804` |
| `containerCopperChestColor.x` | `0x3f073` | `0x002d483c` |
| `containerCopperChestColor.y` | `0x3ae20` | `0x002d4840` |
| `containerCopperChestColor.z` | `0x42f4d` | `0x002d4844` |
| `containerEnderChestColor.x` | `0x3f8ef` | `0x002d481c` |
| `containerEnderChestColor.y` | `0x3e3ab` | `0x002d4820` |
| `containerEnderChestColor.z` | `0x418cb` | `0x002d4824` |
| `containerShulkerColor.x` | `0x40df7` | `0x002d484c` |
| `containerShulkerColor.y` | `0x3e3c6` | `0x002d4850` |
| `containerShulkerColor.z` | `0x436ae` | `0x002d4854` |
| `containerTrappedChestColor.x` | `0x40dbd` | `0x002d480c` |
| `containerTrappedChestColor.y` | `0x40dda` | `0x002d4810` |
| `containerTrappedChestColor.z` | `0x423ec` | `0x002d4814` |
| `deathPosition` | `0x3f0b8` | `0x00457ecc` |
| `emulateNativeAttack` | `0x3d39a` | `0x00457ee0` |
| `espBarrel` | `0x3ec0b` | `0x00457f54` |
| `espBed` | `0x3f007` | `0x00457fc8` |
| `espBlocks` | `0x3fd3b` | `0x00457f3c` |
| `espBlocksBox` | `0x436c6` | `0x00457f40` |
| `espBlocksLine` | `0x4329f` | `0x00457f44` |
| `espBox` | `0x41c57` | `0x00457f28` |
| `espBrewingStand` | `0x3cddd` | `0x00457fb4` |
| `espChest` | `0x3f90a` | `0x00457f48` |
| `espCopperChest` | `0x401b6` | `0x00457f58` |
| `espCreakingHeart` | `0x423db` | `0x00457f74` |
| `espDispenser` | `0x42b99` | `0x00457fac` |
| `espDistance` | `0x41c4b` | `0x00457f30` |
| `espEnchantingTable` | `0x410e4` | `0x00457fb0` |
| `espEnderChest` | `0x3b323` | `0x00457f50` |
| `espFurnace` | `0x3adc7` | `0x00457fa4` |
| `espHealth` | `0x41c41` | `0x00457f34` |
| `espHopper` | `0x3d6a5` | `0x00457fa8` |
| `espHunger` | `0x43223` | `0x0045800c` |
| `espLine` | `0x3ce18` | `0x00457f2c` |
| `espMobSpawner` | `0x3f065` | `0x00457f60` |
| `espNametag` | `0x41574` | `0x00457f24` |
| `espPlatform` | `0x42409` | `0x00457f38` |
| `espSculkShrieker` | `0x41fbf` | `0x00457f70` |
| `espShelf` | `0x43243` | `0x00457fc4` |
| `espShulker` | `0x3b6a8` | `0x00457f5c` |
| `espTrappedChest` | `0x3e3de` | `0x00457f4c` |
| `espTrialSpawner` | `0x3d36c` | `0x00457f64` |
| `espVault` | `0x3caa9` | `0x00457f6c` |
| `fastDrop` | `0x40e20` | `0x00457e90` |
| `fogEnable` | `0x40e16` | `0x00457e8c` |
| `freeCam` | `0x3fc9a` | `0x00457ff4` |
| `freeCamShortcut` | `0x40865` | `0x00457ff8` |
| `freeCamShortcutPos.x` | `0x3f4a5` | `0x002d49f0` |
| `freeCamShortcutPos.y` | `0x3b666` | `0x002d49f4` |
| `freeCamShortcutSize.x` | `0x3b650` | `0x002d49f8` |
| `freeCamShortcutSize.y` | `0x4366c` | `0x002d49fc` |
| `freeCamSpeed` | `0x41544` | `0x002d4798` |
| `friendList` | `0x3ce37` | — |
| `fullBright` | `0x3d3b8` | `0x00457e88` |
| `hitBox` | `0x3d6ef` | `0x00457eb0` |
| `hitBoxValue` | `0x3b6d1` | `0x002d47d8` |
| `hotspotMultuplayerFix` | `0x42f20` | `0x00457fe8` |
| `hurtCheck` | `0x3fd45` | `0x00457ff0` |
| `instantRespawn` | `0x41c1c` | `0x00458010` |
| `invCleaner` | `0x3e7d0` | `0x00457fd8` |
| `killAura` | `0x42bfc` | `0x00457eb4` |
| `killAuraShortcut` | `0x3f501` | `0x00457f84` |
| `killAuraShortcutPos.x` | `0x3db0a` | `0x002d4940` |
| `killAuraShortcutPos.y` | `0x40581` | `0x002d4944` |
| `killAuraShortcutSize.x` | `0x40d7e` | `0x002d4948` |
| `killAuraShortcutSize.y` | `0x3d6c7` | `0x002d494c` |
| `killItems` | `0x41c95` | `0x00457eb8` |
| `killItemsShortcut` | `0x4088f` | `0x00457f94` |
| `killItemsShortcutPos.x` | `0x3fcf2` | `0x002d4970` |
| `killItemsShortcutPos.y` | `0x42f36` | `0x002d4974` |
| `killItemsShortcutSize.x` | `0x3b29b` | `0x002d4978` |
| `killItemsShortcutSize.y` | `0x3b283` | `0x002d497c` |
| `killRange` | `0x432c9` | `0x002d47a4` |
| `ladderFly` | `0x3fd6a` | `0x00457ec0` |
| `ladderFlyShortcut` | `0x418b9` | `0x00457f7c` |
| `ladderFlyShortcutPos.x` | `0x405ae` | `0x002d4920` |
| `ladderFlyShortcutPos.y` | `0x40597` | `0x002d4924` |
| `ladderFlyShortcutSize.x` | `0x3fd09` | `0x002d4928` |
| `ladderFlyShortcutSize.y` | `0x3b2cc` | `0x002d492c` |
| `look360` | `0x3e42f` | `0x00457ea8` |
| `look360Shortcut` | `0x3df3b` | `0x00457f98` |
| `look360ShortcutPos.x` | `0x43699` | `0x002d4980` |
| `look360ShortcutPos.y` | `0x3ade5` | `0x002d4984` |
| `look360ShortcutSize.x` | `0x3f021` | `0x002d4988` |
| `look360ShortcutSize.y` | `0x3ebf5` | `0x002d498c` |
| `maxAutoClickerCPS` | `0x3fd58` | `0x002d47c4` |
| `maxKillAuraCPS` | `0x42f84` | `0x002d47b4` |
| `maxMobAuraCPS` | `0x3df54` | `0x002d47bc` |
| `minAutoClickerCPS` | `0x4157f` | `0x002d47c0` |
| `minKillAuraCPS` | `0x42424` | `0x002d47b0` |
| `minMobAuraCPS` | `0x41c87` | `0x002d47b8` |
| `mobAura` | `0x3f93d` | `0x00457ebc` |
| `mobAuraRange` | `0x3db4f` | `0x002d47ac` |
| `mobAuraShortcut` | `0x42bb8` | `0x00457f8c` |
| `mobAuraShortcutPos.x` | `0x4018a` | `0x002d4960` |
| `mobAuraShortcutPos.y` | `0x3f4ec` | `0x002d4964` |
| `mobAuraShortcutSize.x` | `0x41c2b` | `0x002d4968` |
| `mobAuraShortcutSize.y` | `0x3b692` | `0x002d496c` |
| `myMaxPlayers` | `0x3db5c` | `0x002d47a0` |
| `noAllays` | `0x41fef` | `0x002d470c` |
| `noArmadillos` | `0x3b6ba` | `0x002d4710` |
| `noAxolotls` | `0x3db3a` | `0x002d4714` |
| `noBats` | `0x3db33` | `0x002d4718` |
| `noBees` | `0x3b6b3` | `0x002d471c` |
| `noBlur` | `0x42bf5` | `0x00457ed8` |
| `noBreakDelay` | `0x41551` | `0x00457fbc` |
| `noCamels` | `0x3df4b` | `0x002d4720` |
| `noCamSleep` | `0x3e424` | `0x00457e94` |
| `noCats` | `0x40e0f` | `0x002d4724` |
| `noCaveVignette` | `0x3e3ee` | `0x00457f1c` |
| `noChickens` | `0x408ae` | `0x00457ef0` |
| `noclip` | `0x3fd74` | `0x00457ea0` |
| `noclipShortcut` | `0x3adfa` | `0x00457f80` |
| `noclipShortcutPos.x` | `0x42bc8` | `0x002d4930` |
| `noclipShortcutPos.y` | `0x410f7` | `0x002d4934` |
| `noclipShortcutSize.x` | `0x3f037` | `0x002d4938` |
| `noclipShortcutSize.y` | `0x3cded` | `0x002d493c` |
| `noCods` | `0x3d393` | `0x00457ef4` |
| `noCopperGolems` | `0x3e407` | `0x002d473c` |
| `noCows` | `0x3d6de` | `0x00457ef8` |
| `noDolphins` | `0x42419` | `0x002d4740` |
| `noDonkeys` | `0x3b6c7` | `0x002d4738` |
| `noFog` | `0x3df0f` | `0x00457fc0` |
| `noFoxes` | `0x41fe7` | `0x002d4744` |
| `noFrogs` | `0x401e4` | `0x002d4748` |
| `noGlowSquids` | `0x408a1` | `0x002d474c` |
| `noGoats` | `0x3db2b` | `0x00457efc` |
| `noHappyGhasts` | `0x401d6` | `0x002d4750` |
| `noHorses` | `0x41c7e` | `0x002d4728` |
| `noIronGolem` | `0x41125` | `0x002d4734` |
| `noJumpDelay` | `0x3f931` | `0x00457ee4` |
| `noLlamas` | `0x3ba66` | `0x002d4754` |
| `noNautilus` | `0x3e3a0` | `0x002d4788` |
| `noOcelots` | `0x432bf` | `0x002d4758` |
| `noPandas` | `0x432b6` | `0x002d475c` |
| `noParrots` | `0x3cab2` | `0x002d4760` |
| `noPiglins` | `0x3e3fd` | `0x00457f04` |
| `noPigs` | `0x3ae3c` | `0x00457f00` |
| `noPufferfishes` | `0x3b331` | `0x00457f08` |
| `noRabbits` | `0x3f927` | `0x00457f0c` |
| `noSalmons` | `0x405f1` | `0x00457f10` |
| `noSheeps` | `0x432ad` | `0x00457f14` |
| `noSlowDown` | `0x3ce42` | `0x00457e98` |
| `noSniffers` | `0x3f91c` | `0x002d4764` |
| `noSnowMan` | `0x3ce2d` | `0x002d4730` |
| `noSquids` | `0x3f913` | `0x002d4768` |
| `noStriders` | `0x405e6` | `0x002d476c` |
| `noSulfurCubes` | `0x3eff9` | `0x002d4708` |
| `noTadpoles` | `0x3db20` | `0x002d4770` |
| `noTropicalfishes` | `0x401c5` | `0x002d4774` |
| `noTurtles` | `0x405dc` | `0x002d4778` |
| `noVillagers` | `0x3f0ac` | `0x002d472c` |
| `noWanderingtraders` | `0x3f08f` | `0x002d477c` |
| `noWeather` | `0x423d1` | `0x00457f90` |
| `noWolfs` | `0x41c5e` | `0x002d4780` |
| `noZombiepiglins` | `0x3ba56` | `0x002d4784` |
| `noZombievillagers` | `0x42f69` | `0x00457f18` |
| `offDrown` | `0x3db69` | `0x00457eac` |
| `offSlowdownTrigger` | `0x41591` | `0x00457e9c` |
| `overrideRender` | `0x41535` | `0x00457ffc` |
| `panic` | `0x408c5` | `0x00457f68` |
| `physicalHotkeys` | `0x3d31b` | — |
| `playersDeathPositionsBDS` | `0x3b637` | `0x00458008` |
| `protectedPlayerCheck` | `0x41868` | `0x00458000` |
| `rotations` | `0x3b6dd` | `0x002d47d0` |
| `rotationsRange` | `0x3ae43` | `0x002d47a8` |
| `scaffold` | `0x3ba33` | `0x00457fcc` |
| `scaffoldShortcut` | `0x3f897` | `0x00457fd0` |
| `scaffoldShortcutPos.x` | `0x3f4c3` | `0x002d49c0` |
| `scaffoldShortcutPos.y` | `0x4322d` | `0x002d49c4` |
| `scaffoldShortcutSize.x` | `0x42b3d` | `0x002d49c8` |
| `scaffoldShortcutSize.y` | `0x40147` | `0x002d49cc` |
| `scaffoldType` | `0x40541` | `0x00458028` |
| `seeHiddenBlocks` | `0x3fca2` | `0x00457fe4` |
| `serverBlink` | `0x42f14` | `0x00458014` |
| `serverBlinkShortcut` | `0x4010d` | `0x00458018` |
| `serverBlinkShortcutPos.x` | `0x3e387` | `0x002d4a00` |
| `serverBlinkShortcutPos.y` | `0x40121` | `0x002d4a04` |
| `serverBlinkShortcutSize.x` | `0x3daf0` | `0x002d4a08` |
| `serverBlinkShortcutSize.y` | `0x3dad6` | `0x002d4a0c` |
| `showPaperdoll` | `0x3e416` | `0x00457edc` |
| `showPosition` | `0x408d3` | `0x00457ec8` |
| `speedValue` | `0x42433` | `0x002d4794` |
| `stayOnline` | `0x3f88c` | `0x0045801c` |
| `swing` | `0x3ba87` | `0x002d47cc` |
| `targetLock` | `0x3b348` | `0x00457ed4` |
| `targetLockShortcut` | `0x43259` | `0x00457f88` |
| `targetLockShortcutPos.x` | `0x42786` | `0x002d4950` |
| `targetLockShortcutPos.y` | `0x3d6af` | `0x002d4954` |
| `targetLockShortcutSize.x` | `0x3b2b3` | `0x002d4958` |
| `targetLockShortcutSize.y` | `0x40568` | `0x002d495c` |
| `velo` | `0x43682` | — |
| `velocityType` | `0x4013a` | `0x0045802c` |
| `velocityX` | `0x3f0a2` | `0x002d478c` |
| `velocityY` | `0x3db45` | `0x002d4790` |
| `width` | `0x410a7` | `0x004580c0` |
| `xrayAmethyst` | `0x41c66` | `0x0045809c` |
| `xrayCam` | `0x41ff8` | `0x00457ea4` |
| `xrayCoal` | `0x3fd4f` | `0x002d47e8` |
| `xrayCopper` | `0x427b8` | `0x00458098` |
| `xrayDefault` | `0x3ba7b` | `0x00457eec` |
| `xrayDefaultShortcut` | `0x3fcb2` | `0x00457fb8` |
| `xrayDefaultShortcutPos.x` | `0x41f5c` | `0x002d49b0` |
| `xrayDefaultShortcutPos.y` | `0x3f8a8` | `0x002d49b4` |
| `xrayDefaultShortcutSize.x` | `0x3cdc3` | `0x002d49b8` |
| `xrayDefaultShortcutSize.y` | `0x40875` | `0x002d49bc` |
| `xrayDiamond` | `0x418e6` | `0x002d47dc` |
| `xrayEmerald` | `0x3ba6f` | `0x002d47f0` |
| `xrayGold` | `0x42f7b` | `0x002d47e4` |
| `xrayIron` | `0x401ec` | `0x002d47e0` |
| `xrayLapis` | `0x3d6e5` | `0x002d47ec` |
| `xrayNetherite` | `0x3ec15` | `0x002d47f8` |
| `xrayObsidian` | `0x3ce20` | `0x004580a4` |
| `xrayQuartz` | `0x41c73` | `0x004580a0` |
| `xrayRedstone` | `0x405fb` | `0x002d47f4` |
| `zoom` | `0x3f8ea` | — |
| `zoomShortcut` | `0x4324c` | `0x00457fa0` |
| `zoomShortcutPos.x` | `0x42ba6` | `0x002d49a0` |
| `zoomShortcutPos.y` | `0x3df15` | `0x002d49a4` |
| `zoomShortcutSize.x` | `0x3f8d7` | `0x002d49a8` |
| `zoomShortcutSize.y` | `0x3add2` | `0x002d49ac` |

## 3. Unmounted / Hidden / Leftover Features

- `Panic`, `Xray`, `Zoom`, `Freelook`, `KillAura`, `MobAura`, `Scaffold`, `Blink`, `Server Blink`, `Noclip`, `Auto Clicker`, `Target Lock`, `Free Camera`, `Ladder Fly`, `Block Fly` and `Kill Near Items` are embedded display strings. Some occur in a data table; the shortcut-feedback function references several. Data-table-only XREFs are **not proof** of hidden/unmounted status. Labels may also be constructed inline rather than appearing as contiguous strings.
- `Save Y`, `Save Y (Air Place)`, and `Block Fly Up`, `Block Fly Down`, `Block Fly Direction`, `Block Fly Default` are mode/shortcut labels. Their precise widget registration remains unresolved.
- Numerous `no<entity>` keys (including `noNautilus`, `noSulfurCubes`, `noHappyGhasts`, `noCopperGolems`) are config-backed entity options; their attachment to MobAura or any other feature is not proven. `botList`, `friendList`, `clearItemList`, `playersDeathPositionsBDS` and `physicalHotkeys` are serialized data, not necessarily standalone modules.
- `Bedrock Anti Cheat Detected.` appears in shortcut-feedback code. It is not evidence of a separate anti-cheat-bypass module. Native key-handler, Android `KEYCODE_*`, JNI, OpenGL, and shadowhook strings indicate infrastructure; they are not proof of individual hooks.
- No custom module class hierarchy or hidden/developer-only class has been established. Absence from the recovered UI strings is insufficient to classify any feature as disabled or unmounted. A complete verified registry requires successful analysis of the large UI function and/or authorized runtime observation.

### Reproducibility and raw evidence

Scripts: `/home/user/ghidra_scripts/extract_modules.py`, `/home/user/ghidra_scripts/trace_globals.py`. Raw files: `/home/user/analysis/ghidra_xrefs.tsv`, `global_xrefs.tsv`, `key_globals.tsv`, `ghidra_functions.tsv`, `ghidra_decompiled.c`, `headless.log`, `nm_demangled.txt`, `strings.txt`. Config locations include `/files/config.json` and `/storage/emulated/0/Android/data/com.mojang.minecraftpe/files/config.json`. The workspace copy of this report, scripts, binary, and compressed project backup is in `ghidra_analysis/`. The live `/home/user` installation and project are outside the repository snapshot boundary; their persistence across sandbox replacements cannot be guaranteed. Restore the project backup under `/home/user/ghidra_projects` if needed; reinstall Ghidra from the Drive archive if necessary.
