import csv,collections,re,os
base='/home/user/analysis/'
xs=list(csv.DictReader(open(base+'ghidra_xrefs.tsv'),delimiter='\t'))
gl={x['key']:x['global'].replace('DAT_','0x') for x in csv.DictReader(open(base+'key_globals.tsv'),delimiter='\t')}
grefs=list(csv.DictReader(open(base+'global_xrefs.tsv'),delimiter='\t'))
keys={x['string']:x['string_file_offset'] for x in xs if x['function_entry']=='001950f4'}
rows={
'Combat':[
('killAura','killRange, minKillAuraCPS, maxKillAuraCPS, auraType, killAuraShortcut','Attack-related state; same global used by FUN_001abb30 and shortcut feedback.'),
('mobAura','mobAuraRange, minMobAuraCPS, maxMobAuraCPS, mobAuraShortcut; no* entity filters','Mob-targeting implied by keys; association of filters inferred.'),
('targetLock','targetLockShortcut, rotations, rotationsRange','Target-lock state; aim/rotation relationship inferred from names.'),
('autoClicker','minAutoClickerCPS, maxAutoClickerCPS, autoClickerShortcut','Click-rate configuration.'),
('hitBox','hitBoxValue','Hit-box modification implied by name.'),
('antiBot','botList','Bot filtering implied; linkage inferred.'),
('antiInvis','—','Invisibility-related behavior implied by name.'),
('antiFireball','—','Fireball-related behavior implied by name.'),
('emulateNativeAttack','—','Native attack emulation implied by name.'),
('protectedPlayerCheck','friendList; hurtCheck','Player-protection / hurt checks; association inferred.'),
],
'Movement':[
('blockFly','blockFlyShortcut; blockFlyTypeShortcut; Block Fly Up/Down/Direction/Default','Movement/placement implied; feedback references Block Fly.'),
('ladderFly','ladderFlyShortcut','Ladder movement implied; feedback references Ladder Fly.'),
('noclip','noclipShortcut','Collision bypass implied; feedback references Noclip.'),
('noJumpDelay','—','Jump delay-related state.'),
('noSlowDown','offSlowdownTrigger','Slowdown-related states; association inferred.'),
('velo','velocityType, velocityX, velocityY','Velocity-related state; associations inferred.'),
('freeCam','freeCamSpeed, freeCamShortcut','Camera movement implied; feedback references Free Camera.'),
('look360','look360Shortcut; Freelook label','View rotation implied; label association inferred.'),
],
'Visuals / Render':[
('espBlocks','espBlocksLine, espBlocksBox; esp* block/container selectors and color vectors','Block ESP implied; multiple block-hook functions reference its global.'),
('espLine','espBox, espNametag, espHealth, espDistance, espHunger, espPlatform','Overlay selectors; grouping is name-based.'),
('xrayDefault','xrayDefaultShortcut; xrayCoal, xrayIron, xrayGold, xrayDiamond, xrayEmerald, xrayLapis, xrayRedstone, xrayCopper, xrayNetherite, xrayQuartz, xrayAmethyst, xrayObsidian; xrayCam','X-ray rendering implied; associations inferred.'),
('fullBright','—','Brightness-related state.'),
('noFog','fogEnable','Fog state; association inferred.'),
('noWeather','—','Weather rendering behavior implied.'),
('noBlur','—','Blur-related state.'),
('noCaveVignette','—','Cave vignette state.'),
('zoom','zoomShortcut','Zoom state; global not recovered by the simple extractor.'),
('overrideRender','chunksOverride','Render/chunk overriding implied; association inferred.'),
('seeHiddenBlocks','—','Hidden-block visibility implied.'),
],
'Building':[
('scaffold','scaffoldType, scaffoldShortcut; Save Y and Save Y (Air Place) strings','Placement/scaffold implied; same state referenced in FUN_001abb30.'),
('autoTool','—','Tool-selection behavior implied; global also referenced by FUN_001b40d0 and others.'),
('noBreakDelay','—','Block breaking delay implied.'),
],
'Player / World':[
('autoFish','—','Fishing implied; item.fishing_rod is referenced by FUN_001abb30, but linkage not established.'),
('invCleaner','clearItemList','Inventory cleanup implied; association inferred.'),
('killItems','killItemsShortcut; Kill Near Items label','Nearby-item behavior implied; feedback references label.'),
('fastDrop','—','Item-dropping behavior implied.'),
('instantRespawn','—','Respawn behavior implied.'),
('offDrown','—','Drowning-related state.'),
('noCamSleep','—','Sleep-camera-related state.'),
('stayOnline','—','Stay-online behavior implied.'),
('deathPosition','playersDeathPositionsBDS','Death-position persistence implied; association inferred.'),
],
'Exploits / Other':[
('blink','blinkShortcut','Blink state; same global used in several other functions and feedback.'),
('serverBlink','serverBlinkShortcut','Network/server behavior implied; Server Blink appears in feedback.'),
('hotspotMultuplayerFix','myMaxPlayers','Multiplayer compatibility implied; association inferred; spelling as embedded.'),
('panic','Panic label','Emergency toggle implied; label occurs in data table, main-tab exposure unproven.'),
],
'UI / HUD':[
('showPosition','—','Position display implied; global referenced by FUN_001b3ff0.'),
('showPaperdoll','—','Paper-doll display implied.'),
('physicalHotkeys','Android KEYCODE_*; shortcut enable, Pos.x/y and Size.x/y fields','Hotkey configuration array; exact per-feature bindings not recovered.'),
]
}
count=sum(map(len,rows.values()))
head='''# libReverseSigma static feature inventory

## 1. Binary Metadata

- **File:** `/home/user/libReverseSigma.so`, 3,473,688 bytes; SHA-256 `c48da8d725e3515ef80e686c58a6a6fa2178ec7cce6b21d1356f53468d0c5b6f`.
- **Architecture:** ELF64, little-endian AArch64 Android shared object (ET_DYN), SONAME `libReverseSigma.so`. `readelf -h/-S` verified the header and sections. The `file` utility is not installed; no `file` result is claimed.
- **Compiler/linker:** `.comment` records Android clang 22.0.1, Android clang 21.0.0 (including a HomuHomu833 custom build), and LLD 21.0.0. No `.symtab` or DWARF debug data: stripped of ordinary private symbols, but `.dynsym` and standard-library C++ RTTI/vtables remain.
- **Count:** {count} editorially grouped **candidate features**, based on config/strings/state, **not a verified total of registered modules**. There are **293 distinct config keys** in both Ghidra-observed config routines. Some groups could be settings rather than standalone modules, while unrecognized features may remain.
- **Dependencies:** `liblog.so`, `libz.so`, `libandroid.so`, `libEGL.so`, `libGLESv2.so`, `libc.so`, `libm.so`, `libdl.so`. Dear ImGui 1.90.0, nlohmann JSON and shadowhook 2.0.1 strings are present. `JNI_OnLoad` ELF offset `0xb5568`; `GLES3JNIView_init` `0x942fc`; `GLES3JNIView_step` `0x9866c`.
- **Method:** Ghidra 11.2.1 headless, AARCH64:LE:64:v8A, 1,400 MiB heap and SerialGC, persistent project `/home/user/ghidra_projects/ReverseSigma`. 4,725 functions recognized. JSON load `FUN_001950f4`, save `FUN_001986cc` and ImGui-related initialization/shortcut feedback were decompiled. ELF strings, dynamic symbols and RTTI were cross-checked. Exact app module classes and widget exports were not found.
- **Limits:** Main UI function `FUN_001a0958` references many feature globals, but its decompilation failed because the decompiler process died. Therefore actual tabs, widget types, keybindings, hook targets, defaults and full control-to-feature links are **not established**. Ghidra reported unhandled AArch64 TLSDESC relocations and unresolved Android libraries; no runtime verification was performed. Categories and associations below are analytical groupings, not claims of the binary's actual tabs. The first Ghidra run used default `/var/tmp` for its cache before cache configuration was corrected; installation, project, and input were kept under `/home/user`. Subsequent Ghidra cache and temporary storage are configured under `/home/user/analysis`.

## 2. Categorized Module List

**Address convention:** Ghidra loads ELF at image base `0x00100000`; `DAT` is a Ghidra state-variable address, and `FUN` is a Ghidra function address. Subtract `0x100000` for ELF virtual offsets. Every config name also has a string address and loader/saver XREF in the exhaustive appendix. A dash means unconfirmed, not absent. Controls sharing a name stem are plausible associations only, unless specifically described as evidenced. Global XREFs prove use, not hook semantics.
'''.format(count=count)
parts=[head]
for cat,items in rows.items():
 parts.append('\n### '+cat+'\n\n| Exact config name | State address / code references | Related keys / controls | Technical note |\n|---|---|---|---|\n')
 for key,settings,note in items:
  refs=collections.Counter(x['function'] for x in grefs if x['key']==key and x['function'] not in ('001950f4','001986cc','001a0958',''))
  fun=', '.join('`FUN_'+x+'`' for x,_ in refs.most_common(2))
  addr=gl.get(key,'not resolved')+(('; '+fun) if fun else '')
  parts.append('| `'+key+'` | '+addr+' | '+settings+' | '+note+' |\n')
parts.append('''
### Complete config-key cross-check

The following is the entire set of distinct strings directly referenced by `FUN_001950f4` as config keys. The matching save function also references all 293. String offset is an ELF `.rodata` file/virtual offset; state addresses are best-effort results from the decompiled load routine. “—” means the simple mapping could not resolve a state global. Presence in config does **not** prove a UI tab, keybind, active hook, or a standalone module.

| Exact config key | String offset | State (Ghidra address) |
|---|---:|---:|
''')
for k in sorted(keys,key=str.lower): parts.append('| `'+k+'` | `0x'+keys[k]+'` | '+('`'+gl[k]+'`' if k in gl else '—')+' |\n')
parts.append('''
## 3. Unmounted / Hidden / Leftover Features

- `Panic`, `Xray`, `Zoom`, `Freelook`, `KillAura`, `MobAura`, `Scaffold`, `Blink`, `Server Blink`, `Noclip`, `Auto Clicker`, `Target Lock`, `Free Camera`, `Ladder Fly`, `Block Fly` and `Kill Near Items` are embedded display strings. Some occur in a data table; the shortcut-feedback function references several. Data-table-only XREFs are **not proof** of hidden/unmounted status. Labels may also be constructed inline rather than appearing as contiguous strings.
- `Save Y`, `Save Y (Air Place)`, and `Block Fly Up`, `Block Fly Down`, `Block Fly Direction`, `Block Fly Default` are mode/shortcut labels. Their precise widget registration remains unresolved.
- Numerous `no<entity>` keys (including `noNautilus`, `noSulfurCubes`, `noHappyGhasts`, `noCopperGolems`) are config-backed entity options; their attachment to MobAura or any other feature is not proven. `botList`, `friendList`, `clearItemList`, `playersDeathPositionsBDS` and `physicalHotkeys` are serialized data, not necessarily standalone modules.
- `Bedrock Anti Cheat Detected.` appears in shortcut-feedback code. It is not evidence of a separate anti-cheat-bypass module. Native key-handler, Android `KEYCODE_*`, JNI, OpenGL, and shadowhook strings indicate infrastructure; they are not proof of individual hooks.
- No custom module class hierarchy or hidden/developer-only class has been established. Absence from the recovered UI strings is insufficient to classify any feature as disabled or unmounted. A complete verified registry requires successful analysis of the large UI function and/or authorized runtime observation.

### Reproducibility and raw evidence

Scripts: `/home/user/ghidra_scripts/extract_modules.py`, `/home/user/ghidra_scripts/trace_globals.py`. Raw files: `/home/user/analysis/ghidra_xrefs.tsv`, `global_xrefs.tsv`, `key_globals.tsv`, `ghidra_functions.tsv`, `ghidra_decompiled.c`, `headless.log`, `nm_demangled.txt`, `strings.txt`. Config locations include `/files/config.json` and `/storage/emulated/0/Android/data/com.mojang.minecraftpe/files/config.json`. The workspace copy of this report, scripts, binary, and compressed project backup is in `ghidra_analysis/`. The live `/home/user` installation and project are outside the repository snapshot boundary; their persistence across sandbox replacements cannot be guaranteed. Restore the project backup under `/home/user/ghidra_projects` if needed; reinstall Ghidra from the Drive archive if necessary.
''')
report=''.join(parts)
open('/home/user/libReverseSigma_modules.md','w').write(report)
open('ghidra_analysis/libReverseSigma_modules.md','w').write(report)
print('candidate feature groups:',count,'; unique config keys:',len(keys))
