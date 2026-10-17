# AI Code Commentation

This document preserves all the explanatory comments that previously lived inline in `src/mod.cpp`. The comments have been removed from the source file to keep it focused on code; this file keeps their full text, in the same order they appeared in the file, each labeled with the line (or code construct) they used to sit above or next to in `mod.cpp`, so the two can still be cross-referenced.

## Before/at: `#include "Z2AudioLib/Z2AudioMgr.h"`
*(originally `mod.cpp` lines 7-7)*

Game includes

## Before/at: `(blank line)`
*(originally `mod.cpp` lines 28-39)*

--------------------------------------------------------------------------------------------
Fire Arrows

Lets the player mix the Lantern into the Bow's C-button slot, exactly like the vanilla
Bow & Hawkeye combo. While the combo is equipped, arrows are lit on the way out by default,
igniting whatever they hit the same way the fire arrows shot by Bulblin (Bokoblin) archers do,
and whatever enemy they hit briefly catches fire too, just like getting hit by one of those
archers' own fire arrows (a purely cosmetic flame - see igniteBurningActor below for why this
doesn't also add extra damage over time). Each shot costs the same amount of lantern oil as a
single lantern swing. While aiming, the same button press that switches a Bow & Bomb Arrow combo
between bomb and normal arrows switches this combo between fire and normal arrows instead.
--------------------------------------------------------------------------------------------

## Before/at: `static constexpr u8 NO_MIX_ITEM = 0xFF;`
*(originally `mod.cpp` lines 41-41)*

dComIfGs_getMixItemIndex() returns this when a C-button slot has no item mixed into it.

## Before/at: `DEFINE_HOOK(&dMenu_Ring_c::setMixItem, SetMixItem);`
*(originally `mod.cpp` lines 44-50)*

Hook targets: the menu code that lets the player drag one item onto another to combine them
(used natively for Bow+Bomb and Bow+Hawkeye), plus the two functions that decide whether to
show the "Bow & Arrow Combo"/"Combo Off" prompt while an item is highlighted. All three have
their own whitelist switch that doesn't know about the Lantern, so while any of them run we
briefly present the highlighted Lantern as a Hawkeye (a combo the whitelists already accept),
then restore the real item right after the call. This reuses all of the vanilla
combining/bookkeeping and prompt-display logic instead of reimplementing it.

## Before/at: `DEFINE_HOOK(&dMenu_Ring_c::setActiveCursor, SetActiveCursor);`
*(originally `mod.cpp` lines 55-64)*

Hook target: the input-handling function that decides what a menu button press does to the
highlighted item (assign to a C-button, combine, etc.) - setMixItem() (above) is one of the
things it can call, but it's not the only way an item ends up combined with the Bow. Some HUD
mods (e.g. Twilight HD HUD's third item slot) add a "Z" select-item slot the save format
already reserves for the Wii control scheme (SELECT_ITEM_DOWN, d_save.h) but the vanilla
GameCube-style menu code never uses; those mods implement combining into that slot themselves,
directly here in setActiveCursor, entirely bypassing setMixItem() and applying their own
item-type whitelist (which, same as the vanilla one, doesn't know about the Lantern) before
ever calling it. Disguising the Lantern here too, before any such mod's own hook can inspect
the highlighted item, lets their whitelist accept it exactly like the vanilla one already does.

## Before/at: `static constexpr HookOptions kDisguiseHookOptions = {`
*(originally `mod.cpp` lines 67-73)*

Some other mods that hook these same four functions (see above) decide whether to accept the
highlighted item based on its real type, so our disguise has to be in place *before* any of
them look at it - a plain, unprioritized add_pre would only guarantee that by accident, purely
depending on mod load order. This priority is set far above the default (0) so our pre-hooks
always run first regardless of load order; the corresponding post-hooks use the same priority
so our restore also runs before any later mod's own post-hook needs to see the real item again
(e.g. to refresh a combo icon from save data, see on_set_mix_item_post below).

## Before/at: `DEFINE_HOOK(&daArrow_c::arrowShooting, ArrowShooting);`
*(originally `mod.cpp` lines 77-81)*

Hook target: the moment an arrow is actually released, where the game already special-cases
the arrow's attack material per arrow type (see the ARROW_TYPE_LIGHT branch below it in the
original code). This is where we (re-)mark the arrow as a fire arrow if the combo is active,
and where the fire-arrow's ignition effect (as opposed to its purely cosmetic flame, which can
already be showing at this point - see on_alink_make_arrow_post below) actually turns on.

## Before/at: `DEFINE_HOOK(&daAlink_c::makeArrow, AlinkMakeArrow);`
*(originally `mod.cpp` lines 84-87)*

Hook target: the moment the Hero's Bow nocks a new arrow (on drawing the bow back, and again
whenever the arrow type is switched mid-draw), well before it's actually released. This is
where we start the fire arrow's cosmetic flame effect early, so the arrowhead already looks lit
while the shot is being lined up, exactly like it looks once fired.

## Before/at: `DEFINE_HOOK(&daArrow_c::execute, ArrowExecute);`
*(originally `mod.cpp` lines 90-92)*

Hook target: the arrow's per-frame update. daArrow_c has no arrow-type value for "fire" (unlike
the enemy fire arrows in d_a_e_arrow.cpp), so we detect it via the attack material we set
ourselves in on_arrow_shooting_pre and use this to keep the fire trail following the arrow.

## Before/at: `DEFINE_HOOK(&dMeter2Draw_c::drawKanteraMeter, DrawKanteraMeter);`
*(originally `mod.cpp` lines 95-103)*

Hook target: the per-frame HUD draw call that positions and shows/hides the Lantern's oil-gauge
overlay over a C-button icon (dMeter2_c::execute decides the alpha argument every frame: 1.0f,
a sentinel this function itself expands into the button's real current alpha, if that slot's
item is exactly dItemNo_KANTERA_e, 0.0f/hidden otherwise). The gauge is purely an overlay drawn
on top of whatever icon texture the slot is already showing (drawKantera/drawKanteraMeter never
touch the icon itself), and the Bow+Lantern combo's C-button icon is just the ordinary Bow icon
(TP has no unique combined icon art for any Bow combo - Bow+Bomb and Bow+Hawkeye don't get one
either), so no new icon is needed here: only the visibility gate has to widen to also cover the
combo. See on_draw_kantera_meter_pre below for why the gate is otherwise closed for the combo.

## Before/at: `DEFINE_HOOK(&daAlink_c::changeArrowType, ChangeArrowType);`
*(originally `mod.cpp` lines 106-114)*

Hook target: the function that lets the vanilla Bow+Bomb and Bow+Hawkeye combos switch between
their two arrow types while the bow is drawn back and held (mEquipItem == dItemNo_BOMB_ARROW_e/
dItemNo_HAWK_ARROW_e branches below), gated on the same arrowChangeTrigger() press used for
both. The Bow+Lantern combo never changes mEquipItem away from dItemNo_BOW_e (unlike those two,
which get their own dedicated item numbers - see checkBowLanternCombo's comment above), so the
original function's own `mEquipItem == dItemNo_BOW_e && field_0x301e == 0` guard always takes
its early-return branch for it and never reaches the switching logic at all. This hook adds an
equivalent branch of our own ahead of that, active only while the combo is equipped, so the same
physical button press used for Bomb Arrow/Hawkeye switching also toggles Fire Arrow mode.

## Before/at: `DEFINE_HOOK(&daAlink_c::setBowReadyAnime, SetBowReadyAnime);`
*(originally `mod.cpp` lines 117-123)*

Hook target: the moment the Hero's Bow transitions from not-drawn to freshly drawn back (i.e.
nocking the very first arrow of a new aim, as opposed to a mid-draw arrow-type switch). This is
where the vanilla Bomb Arrow combo resets its own toggle based on current ammo (`field_0x301e`,
see the mEquipItem == dItemNo_BOMB_ARROW_e branch below); mirrored here to reset Fire Arrow mode
to its default (on, if there's oil to spend) at the start of every fresh aim, the same way Bomb
Arrow mode always starts from "on" if the player has bombs, regardless of how the previous aim
was left.

## Before/at: `static u8 g_disguisedItemSlot = NO_MIX_ITEM;`
*(originally `mod.cpp` lines 126-128)*

Slot temporarily disguised by disguiseLanternPre(), restored by restoreLanternPost().
NO_MIX_ITEM means "nothing to restore". These hooks never nest (each menu function above runs
to completion before the next is called), so a single slot is enough to track the disguise.

## Before/at: `mods::arg<dMenu_Ring_c*>(args, 0)->setJumpItem(false);`
*(originally `mod.cpp` lines 155-159)*

setMixItem() reloads the sliding combo-icon textures (via setJumpItem -> setSelectItem)
from save data *before* our restore above runs, so if the Lantern was involved it captured
the disguised Hawkeye icon instead. Now that the real item is back in the save slot, call
setJumpItem() again to reload the correct icon. Harmless to call unconditionally: it just
re-reads the (now-correct) current combo state, exactly like the original call did.

## Before/at: `static bool checkBowLanternComboInSlot(int selectItemIdx) {`
*(originally `mod.cpp` lines 190-197)*

True if the Bow is assigned to one of the two C-button slots and the Lantern is mixed into it,
the same way dItemNo_HAWK_ARROW_e/dItemNo_BOMB_ARROW_e detect their combos.

When a combo is active, dComIfGs_getSelectItemIndex(selectItemIdx) holds the *partner* item's
inventory slot (e.g. the Lantern's slot) and dComIfGs_getMixItemIndex(selectItemIdx) holds
SLOT_4 (the Bow's fixed slot) -- not the other way around. dComIfGp_getSelectItem() already
confirms the mixed-in item is the Bow (it only returns dItemNo_BOW_e after swapping in that
case), so all that's left to check here is that the partner slot holds the Lantern.

## Before/at: `static bool checkBowLanternComboInExtraSlot(int selectItemIdx) {`
*(originally `mod.cpp` lines 207-216)*

True if the Bow is assigned to one of the save format's other select-item slots and the
Lantern is mixed into it there. SELECT_ITEM_NUM (2) and up (SELECT_ITEM_DOWN, i.e. a "Z" slot,
and beyond) are reserved by the save format for the Wii control scheme (d_save.h) and never
used by the vanilla GameCube-style menu code, but some HUD mods (e.g. Twilight HD HUD's third
item slot) revive one of them for an extra quick-item slot, managing its mix state directly
through dComIfGs_get/setMixItemIndex rather than through dMenu_Ring_c::setMixItem().
dComIfGp_getSelectItem()'s Bow-swap logic (d_com_inf_game.cpp) only special-cases
SELECT_ITEM_X/Y, so unlike checkBowLanternComboInSlot() above, this checks the raw save-data
mix/select index fields directly instead - the same fields any such mod itself has to use to
establish a combo for a slot the vanilla function doesn't know about.

## Before/at: `static bool g_fireArrowModeOn = true;`
*(originally `mod.cpp` lines 242-250)*

Whether the Bow+Lantern combo should currently light its arrows on release (true) or shoot
plain arrows while leaving the Lantern's oil untouched (false). Mirrors the vanilla Bomb Arrow
combo's own field_0x301e toggle, except tracked externally here rather than reusing that field,
since the Bow+Lantern combo never changes mEquipItem away from dItemNo_BOW_e (unlike Bomb
Arrow/Hawkeye - see checkBowLanternCombo's comment above) and setBowReadyAnime()'s own
field_0x301e reset (the mEquipItem == dItemNo_BOMB_ARROW_e branch) therefore never runs for it;
reusing that field here would leave it forced back to 0 on every fresh draw by that same branch.
Defaults to true so a freshly loaded save (or before the player ever draws the combo) behaves
exactly like this mod did before this toggle existed: fire arrows whenever oil is available.

## Before/at: `static HookAction on_change_arrow_type_pre(ModContext*, void* args, void*, void*) {`
*(originally `mod.cpp` lines 253-259)*

Runs once per frame while the Bow+Lantern combo is drawn back and held (the same call site the
vanilla function itself uses for the Bomb Arrow/Hawkeye combos - see ChangeArrowType's hook
comment above). Toggles g_fireArrowModeOn on arrowChangeTrigger(), exactly mirroring the vanilla
Bomb Arrow branch just below in the original function: switching out of fire mode is always
allowed, but switching into it is blocked while there's no oil to spend on it, the same way the
vanilla guard blocks entering Bomb Arrow mode with zero bombs loaded (field_0x301e == 0 &&
dComIfGp_getSelectItemNum(mSelectItemId) == 0) while always allowing leaving it.

## Before/at: `if (link->mItemAcKeep.getActor() != NULL) {`
*(originally `mod.cpp` lines 276-278)*

Re-nock the already-drawn arrow so its cosmetic flame (or lack thereof) and the oil it
will spend on release immediately reflect the new mode, exactly like the vanilla
function re-nocks the arrow after toggling field_0x301e for the Bomb Arrow combo.

## Before/at: `static HookAction on_set_bow_ready_anime_pre(ModContext*, void* args, void*, void*) {`
*(originally `mod.cpp` lines 289-296)*

Runs once, when the bow transitions from not-drawn to freshly drawn back (see
SetBowReadyAnime's hook comment above) - checkBowAnime() is checked here, as a pre-hook, the
same way the original function itself gates its own field_0x301e reset on it
(`if (!checkBowAnime())`) before doing anything else, so this only fires on that same
transition, not on every frame the bow stays drawn. Must run before the original: the original
sets the very animation state checkBowAnime() reads (via setUpperAnimeBase() further down), so
checking it from a post-hook instead would always see the anim it had just set and never detect
the transition at all.

## Before/at: `static HookAction on_draw_kantera_meter_pre(ModContext*, void* args, void*, void*) {`
*(originally `mod.cpp` lines 307-321)*

dMeter2Draw_c::SELECT_X_e/SELECT_Y_e (0/1) are the same values checkBowLanternComboInSlot()
above already expects as its selectItemIdx - both identify a C-button slot via
dComIfGp_getSelectItem()'s indexing, so the button index drawKanteraMeter() is called with can
be passed straight through without any translation.

drawKanteraMeter() is always called once per button per frame, with the gauge-visibility
decision (dItemNo_KANTERA_e vs. anything else) already baked into the incoming alpha before our
hook ever sees it: 1.0f (a sentinel drawKanteraMeter() itself expands into the button's real,
already fade/cutscene-aware current alpha) when visible, 0.0f when not. The Bow+Lantern combo
makes dComIfGp_getSelectItem() resolve to dItemNo_BOW_e for that slot (the same swap
checkBowLanternComboInSlot() itself accounts for), so the vanilla check always takes the "not
Kantera" branch and calls this with 0.0f even while oil is actively being spent. Re-arming that
same 1.0f sentinel here - instead of inventing our own alpha - keeps every other visibility rule
(HUD hidden, fading, wolf form, etc.) working exactly like it already does for the plain
Lantern, since it's the same lookup drawKanteraMeter() would have done for a real Kantera slot.

## Before/at: `static const dCcD_SrcCps l_igniteCpsSrc = {`
*(originally `mod.cpp` lines 333-374)*

A second, independent attack collider (separate from the arrow's own main collider,
field_0x688) used purely so a fire-combo arrow's impact ignites flammable objects (candles,
firewood, torches, etc.) exactly like a lantern swing would. Ignitable objects all check for an
*exact* match on AtType (== AT_TYPE_LANTERN_SWING, not a bitmask test) before igniting, so this
can't be folded into field_0x688's own AtType: that collider has to keep its default
AT_TYPE_ARROW so normal arrow damage and enemy-specific arrow-hit reactions (several enemies key
special reactions off ChkAtType(AT_TYPE_ARROW)) keep working. This mirrors the game's own enemy
fire-arrow actor (d_a_e_arrow.cpp), which likewise layers a second, separate collider
(mCcFireEffSph) alongside its main hit collider (mCcAtSph) rather than overloading one
collider's AtType for two purposes - so yes, both AT_TYPE_ARROW and an AT_TYPE_LANTERN_SWING
effect can coexist on the same arrow, just not on the same collider.

This is a CAPSULE (dCcD_Cps), not a sphere, and is re-Set() every frame from the arrow's actual
per-frame travel segment (old.pos -> current.pos, see updateFireArrowEffect below), exactly the
same way the arrow's own field_0x688 sweeps a capsule from its previous position out to a
forward-projected target every frame (daArrow_c::setArrowAt, d_a_arrow.cpp). A dCcD_Sph, in
contrast, is a purely static "is the sphere overlapping the target *this instant*" test -
cM3dGSph::cross()/cM3d_Cross_CylSph() (c_m3d_g_sph.cpp) never look at where the sphere was on
the previous frame, only its current center - so at typical arrow speeds (tens of units per
frame) a plain sphere can end up centered on either side of a torch's collider on consecutive
frames without a single frame's test ever finding the two overlapping, i.e. it can tunnel
straight through. A capsule spanning the actual distance travelled that frame can't skip past
anything in between, which is exactly why the game itself uses one (not a sphere) for the
arrow's own hit detection.

mObj.mSrcObjHitInf.mObjAt.mBase.mSPrm's low nibble-and-a-bit (mask 0x1E, read back via
GetAtGrp()) is the collider's "attack group": cCcS::ChkNoHitAtTg() (c_cc_s.cpp) requires
(GetAtGrp() & targetTgGrp) != 0 before it will even attempt the geometric cross test at all, no
matter how the two colliders overlap in space. Leaving this at 0 (as an earlier version of this
collider did) makes GetAtGrp() always return 0, so the AND is always 0 and every single ignitable
object's Tg collider (all of which have a non-zero TgGrp, e.g. daFireWood_c::mCcDObjInfo's
0xD8FBFFFF/0x1F) gets silently skipped every frame - the collider could never register a hit on
anything at all, regardless of position/radius/timing. 0x1A is the exact value daAlink_c's own
real lantern-swing collider (mAtSph) carries for its entire lifetime: it's Set() once from
l_sphSrc (d_a_alink.cpp, shared with the sword-swing collider) and initKandelaarSwing()
(d_a_alink_kandelaar.inc) never touches this field afterwards, only AtType/AtMtrl/R/etc.

mCpsAttr's start/end/radius here are placeholders - both endpoints are degenerate (zero-length
capsule, i.e. effectively a point) until the first real per-frame Set() in
updateFireArrowEffect, which is harmless since the collider only ever gets registered with the
collision system (dComIfG_Ccsp()->Set()) once igniteActive is true and a real position has been
computed.

## Before/at: `{0x0, {{AT_TYPE_LANTERN_SWING, 0x0, 0x1A}, {0x0, 0x0}, 0x0}},`
*(originally `mod.cpp` lines 377-377)*

mObj

## Before/at: `{dCcD_SE_NONE, 0x0, 0x0, dCcD_MTRL_FIRE, 0x0},`
*(originally `mod.cpp` lines 378-378)*

mGObjAt

## Before/at: `{dCcD_SE_NONE, 0x0, 0x0, 0x0, 0x0},`
*(originally `mod.cpp` lines 379-379)*

mGObjTg

## Before/at: `{0x0},`
*(originally `mod.cpp` lines 380-380)*

mGObjCo

## Before/at: `},`
*(originally `mod.cpp` lines 381-381)*

mObjInf

## Before/at: `{{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, 50.0f}`
*(originally `mod.cpp` lines 383-385)*

mCps (same radius as the real lantern-
swing collider, daAlink_c::
initKandelaarSwing)

## Before/at: `}`
*(originally `mod.cpp` lines 386-386)*

mCpsAttr

## Before/at: `static constexpr GXColorS10 FIRE_ARROW_LIGHT_COLOR = {0xBC, 0x66, 0x42, 0xFF};`
*(originally `mod.cpp` lines 389-395)*

Color and flicker for the fire arrow's own point light (see TrackedFireArrow::light below),
matching the exact warm orange every stationary fire source in the game registers for itself -
e.g. daKtOnFire_c ("Lantern Fire", the wall-mounted dungeon lantern/torch fixture,
d_a_obj_ktOnFire.cpp lightInit()), daFireWood_c (d_a_obj_fireWood.cpp lightInit()) and
daLv1Cdl01_c (d_a_obj_lv1Candle01.cpp lightInit()) all set mColor to this same (0xBC, 0x66,
0x42) and mFluctuation to 1.0f, so reusing it here reads as the same kind of flame rather than
an unrelated new light color.

## Before/at: `static constexpr f32 FIRE_ARROW_LIGHT_POW = 250.0f;`
*(originally `mod.cpp` lines 398-402)*

Every stationary fire source above uses mPow = 500.0f regardless of how big its actual flame
is (a torch and a single candle both light up the same radius), so there's no existing "small
flame" constant to borrow. Half that keeps the arrow's own fire trail reading as the same kind
of light while still being noticeably smaller, matching the request for a reduced radius
compared to those fixtures (and, loosely, the player's own Lantern).

## Before/at: `static constexpr GXColor FIRE_ARROW_BOSSLIGHT_COLOR = {0xBC, 0x66, 0x42, 0xFF};`
*(originally `mod.cpp` lines 405-414)*

dKy_BossLight_set registers a real GX hardware spotlight/point light (daAlink_c::setLight's own
Lantern glow uses the same underlying mechanism via dKy_WolfEyeLight_set, which always claims
index 0 of the same slot array) that dScnKy_env_light_c::exeKankyo() actually turns into
per-vertex GX lighting for BG materials during the frame's real BG draw - unlike the
LIGHT_INFLUENCE/dKy_plight_set companion above, this is what makes the Lantern (and vanilla fire
fixtures below) visibly brighten ordinary terrain/walls, not just nearby actors. Every stationary
fire source that pairs a plight with a GLOW particle (daLv1Cdl00_c/01_c, daLv2Candle_c,
daLv3Candle_c, daFireWood2_c, daPoCandle_c, dT_myna_light_c) also calls dKy_BossLight_set every
single frame with this same (0xBC, 0x66, 0x42) color - same color as FIRE_ARROW_LIGHT_COLOR
above, just as a plain 8-bit-per-channel GXColor instead of LIGHT_INFLUENCE's GXColorS10.

## Before/at: `static constexpr f32 FIRE_ARROW_BOSSLIGHT_REF_DIST = 0.5f;`
*(originally `mod.cpp` lines 416-424)*

Unlike dKy_plight_set (register once, then just mutate mPosition forever), the BOSS_LIGHT slot
array is wiped every single frame by exeKankyo() before being repopulated by whichever
actors/effects call dKy_BossLight_set() that frame - so there's no "lazy register once" flag
here, it simply has to be called again every frame the flame is showing, same as every vanilla
caller above (most of which ramp this value in GXColor's "ref_dist" param via cLib_addCalc from
0.0f to 1.0f, used as a GX_DA_STEEP falloff reference distance rather than a literal unit
measurement). All of those stationary fixtures settle at exactly 1.0f once fully lit; half that,
consistent with FIRE_ARROW_LIGHT_POW's own halving above, keeps this noticeably smaller than
those fixtures (and, loosely, the player's own Lantern) rather than guessing at a new scale.

## Before/at: `struct TrackedFireArrow {`
*(originally `mod.cpp` lines 427-431)*

Tracks the fire trail particle and ignition collider for the player's own in-flight fire
arrows, so both can be re-anchored to each arrow's current position every frame (see
updateFireArrowEffect below). daArrow_c has no spare fields to stash these in, unlike the enemy
fire arrow actor's own mFireEMKeys/mCcFireEffSph, so they're tracked externally here instead,
keyed by the arrow's actor pointer (see arrowId below for why the pointer alone isn't enough).

## Before/at: `fpc_ProcID arrowId = fpcM_ERROR_PROCESS_ID_e;`
*(originally `mod.cpp` lines 434-444)*

The actor framework recycles daArrow_c instances out of a small fixed pool: once an arrow
is deleted (embedded in a wall, landed, etc.), its memory is free to be handed straight back
out to the very next arrow the player nocks. A raw daArrow_c* is therefore not a reliable
way to tell "the same arrow, still in flight" apart from "a brand new arrow that just
happens to have been allocated at the same address" - which, prior to tracking this,
resulted in a new arrow inheriting a stale slot's already-hit collision bookkeeping
(igniteStts) untouched, silently suppressing that new arrow's own ignition hits (works once
right after the pool's addresses are all still fresh, then increasingly fails as arrows
reuse addresses). fpc_ProcID (fpcM_GetID(), backed by fpcBs_MakeOfId()) is the actor
framework's own per-spawn-unique identifier, unaffected by address reuse, and is what
actually distinguishes these two cases - see trackFireArrow below.

## Before/at: `u32 glowParticleKey = 0;`
*(originally `mod.cpp` lines 447-449)*

The actual visible "glow" halo players perceive around any lit fire source in the game -
see glowParticleKey's own setup in updateFireArrowEffect below for why this, not the plight
in FIRE_ARROW_LIGHT_COLOR/LIGHT_INFLUENCE below, is what makes the effect actually visible.

## Before/at: `cXyz velocity = {0.0f, 0.0f, 0.0f};`
*(originally `mod.cpp` lines 451-452)*

Handed to the emitter via setUserWork() below; must outlive the emitter itself, so it's
stored here rather than as a stack temporary.

## Before/at: `dCcD_Stts igniteStts;`
*(originally `mod.cpp` lines 455-462)*

Its own dedicated Stts, rather than sharing the arrow's own field_0x64c. The collision
system tracks per-hit dedup/apid state (e.g. ChkAtNoConHit()'s "already hit this frame/
actor" bookkeeping) on the Stts itself, not on the individual At collider, so sharing
field_0x688's Stts let its AT_TYPE_ARROW hit against a target silently suppress
igniteCps's own AT_TYPE_LANTERN_SWING hit against that same target (or vice versa) -
which is what was actually blocking ignition, not the At-set bit. A dedicated Stts (still
Init'd with the arrow as its actor, same args field_0x64c itself uses) keeps the two
colliders' hit-registration fully independent.

## Before/at: `bool igniteActive = false;`
*(originally `mod.cpp` lines 464-467)*

Only turned on once the arrow is actually fired (see arrowShooting's hook below). The
cosmetic flame can start as soon as the arrow is nocked, but the collider that ignites
flammable objects stays off until then, so merely drawing the bow back near a torch doesn't
light it before the shot is actually released.

## Before/at: `bool hitActorHandled = false;`
*(originally `mod.cpp` lines 469-472)*

Set the first time field_0x688 (the arrow's own main collider, not igniteCps) reports a hit
on some actor, so updateFireArrowEffect only starts that target burning cosmetically
(see igniteBurningActor below) once per arrow, rather than every single frame it stays
stuck in whatever it hit (ChkAtHit() stays true the whole time the arrow remains lodged).

## Before/at: `LIGHT_INFLUENCE light;`
*(originally `mod.cpp` lines 474-487)*

Point light illuminating the arrow's surroundings (see updateFireArrowEffect below),
mirroring how every stationary fire source in the game registers its own LIGHT_INFLUENCE -
see FIRE_ARROW_LIGHT_COLOR/FIRE_ARROW_LIGHT_POW above for why these particular values. Only
mPosition changes frame to frame; dKy_plight_set() hands the engine this struct's address
once (see lightActive below) and it's read live off of that address from then on, the same
way e.g. daE_FM_c's own moving mPlight (d_a_e_fm.cpp) just mutates mPosition in place every
frame without ever calling dKy_plight_set() again. This only ever produces a visible effect
on nearby actors/Link's own model and the handful of specially-authored BG materials that
opt into point-light shading (dScnKy_env_light_c::settingTevStruct's TEV_TYPE_BG*_PLIGHT
path) - it never visibly brightens ordinary terrain on its own, which is what the
dKy_BossLight_set call alongside it in updateFireArrowEffect (see
FIRE_ARROW_BOSSLIGHT_COLOR/FIRE_ARROW_BOSSLIGHT_REF_DIST above) is for instead - so this
LIGHT_INFLUENCE remains a faithful-but-subtle actor/Link-only companion to glowParticleKey
and the real terrain-lighting BossLight call, not a substitute for either.

## Before/at: `bool lightActive = false;`
*(originally `mod.cpp` lines 489-493)*

True once dKy_plight_set(&light) has actually been called for this slot. Sentinels both
"this slot's light is currently registered with the engine and needs dKy_plight_cut() before
being repurposed or abandoned" (see initFireArrowSlot and updateFireArrows below) and
"light.mPosition/mColor/mPow haven't been initialized yet", so dKy_plight_set() is only ever
called the first frame this slot's flame actually shows, not every frame it's re-anchored.

## Before/at: `bool bossLightActive = false;`
*(originally `mod.cpp` lines 495-500)*

True once dKy_BossLight_set has actually been called for this slot at least once (see
fireArrowBossLightSlotIsFree's call site in updateFireArrowEffect below) - i.e. this arrow's
flame is currently one of the (at most 5) terrain lights actually brightening nearby BG, not
just the subtler actor/Link-only plight above. Used purely to pick an eviction candidate
when the pool is full (see findOldestFireArrowEffect below); cleared again once that slot's
BossLight claim is given up, so it's never picked a second time.

## Before/at: `u32 age = 0;`
*(originally `mod.cpp` lines 502-507)*

Monotonically increasing registration order, assigned once in initFireArrowSlot and never
touched again for the lifetime of this tracked arrow. g_fireArrows is a ring buffer indexed
by *when a slot was last (re)claimed*, not by how long its current occupant's effect has
actually been burning, so comparing raw array position wouldn't reliably tell "oldest active
effect" apart from "slot that merely happens to sit earliest in the array". A dedicated
counter does.

## Before/at: `bool bossLightGivenUp = false;`
*(originally `mod.cpp` lines 509-517)*

Set once this slot has deliberately given up its claim on a BossLight pool slot to make room
for a newer arrow (see findOldestFireArrowEffect and despawnFireArrowEffect below). Only
gates the BossLight re-acquisition attempt in updateFireArrowEffect below - the cosmetic
flame/glow particles and the actor/Link-only plight keep running completely normally once
this is set, the same as an arrow that never got a BossLight slot in the first place; only
this slot's own claim on the shared terrain-light pool is affected, permanently (this never
gets reset back to false for the same tracked arrow, so a once-evicted arrow's flame stays
lit but never competes for a BossLight slot again - consistent with how the pool's own
eviction-by-camera-distance never un-evicts a vanilla fixture mid-flight either).

## Before/at: `static constexpr int MAX_TRACKED_FIRE_ARROWS = 8;`
*(originally `mod.cpp` lines 521-523)*

The player only ever has a handful of arrows in flight at once; a small ring buffer is more
than enough, and simply evicts the oldest tracked arrow (long since landed/despawned by then)
if it somehow fills up.

## Before/at: `static u32 g_nextFireArrowAge = 0;`
*(originally `mod.cpp` lines 527-528)*

Feeds TrackedFireArrow::age below - see its own comment for why ring buffer position alone
isn't enough to determine "oldest active effect".

## Before/at: `static void initFireArrowSlot(TrackedFireArrow& slot, daArrow_c* arrow, fpc_ProcID id) {`
*(originally `mod.cpp` lines 531-534)*

(Re-)initializes a ring buffer slot for arrow/id, resetting every piece of per-arrow state -
this is the one and only place that's allowed to happen, so trackFireArrow (the only caller)
can't accidentally skip it for what's actually a brand new arrow instance (see arrowId's comment
on TrackedFireArrow above).

## Before/at: `if (slot.lightActive) {`
*(originally `mod.cpp` lines 536-541)*

Cuts whatever light this slot's previous occupant (a different arrow instance that just
happened to share this address, or this same arrow's own earlier shot - see arrowId's
comment above) left registered, so re-purposing the slot for a brand new arrow can't leave
a stray light frozen at the old occupant's last position. Mirrors the liveness check in
updateFireArrows below, which handles the other way a slot's light can go stale (the arrow
itself despawning without this slot ever being reused).

## Before/at: `slot.igniteStts.Init(10, 0xff, arrow);`
*(originally `mod.cpp` lines 556-558)*

Same Init() args daArrow_c itself uses for field_0x64c (d_a_arrow.cpp), so the ignition
collider's Stts still correctly identifies the arrow as its owning actor - it just doesn't
share field_0x688's hit-dedup/apid bookkeeping (see igniteStts's declaration above).

## Before/at: `static void trackFireArrow(daArrow_c* arrow) {`
*(originally `mod.cpp` lines 564-572)*

Called both when an arrow is nocked (drawn back) and again when it's actually fired, to start
tracking its fire trail particle and (re-)initialize its ignition collider. Idempotent for the
same arrow instance: calling it again for an arrow that's already tracked (e.g. the
shooting-time call, for an arrow that was already tracked at nock time) is a no-op, so the same
arrow never ends up straddling two ring buffer slots at once. Not idempotent across different
arrow instances that happen to share an address (see arrowId's comment on TrackedFireArrow
above): that case re-initializes the existing slot in place instead of treating it as already
tracked, so a reused address can never keep serving a new arrow stale collision state left over
from whatever previously occupied that memory.

## Before/at: `static void activateFireArrowIgnition(daArrow_c* arrow) {`
*(originally `mod.cpp` lines 590-592)*

Called once the arrow is actually fired, to turn on the ignition collider for an already
(or newly) tracked arrow. Split out from trackFireArrow() so nocking an arrow can start its
cosmetic flame without also making it able to ignite things before the shot is released.

## Before/at: `static bool isArrowStationary(daArrow_c* arrow) {`
*(originally `mod.cpp` lines 602-606)*

True while the arrow isn't actually flying (nocked and waiting to be released, or stopped/stuck
and waiting to be re-obtained), as opposed to genuinely moving through the air (procMove, or
procReturn/procSlingHit after deflecting off certain surfaces). Used to keep the fire trail's
particle trace callback from continuing to advance already-emitted trail particles along a
stale, no-longer-updated velocity once the arrow itself has stopped moving.

## Before/at: `(blank line)`
*(originally `mod.cpp` lines 614-640)*

--------------------------------------------------------------------------------------------
Cosmetic "on fire" effect for enemies hit by a fire-combo arrow.

This is deliberately cosmetic-only: it does not deal any extra damage, and does not attempt to
replicate the bulblin (bokoblin) archer's own fire arrows making *Link* take periodic burn
damage (daAlink_c's dCcD_MTRL_FIRE handling in its own damage code, entirely internal to that
one class). There's no equivalent generic "deal damage over time"/"apply a burn status" entry
point usable against an arbitrary enemy actor - every enemy actor class implements its own HP
field and its own reaction to being hit independently, with no shared base class or common
damage API to hook once for all of them. The only generic way to actually hurt an arbitrary
enemy from outside its own class is to make it believe it was hit by another arrow, which would
also replay that enemy's entire hit reaction (flinch animation, hit sound, etc.) on every tick,
not a quiet burn - and since most enemies that die to a single normal arrow hit anyway, extra
damage-over-time on top would rarely even matter before they're already dead. A purely visual
flame, needing nothing from the target beyond its position, avoids all of that.

To actually look like the bulblin archer's own fire arrows, this reuses the exact particle pair
(ID_ZI_J_LK_BURNS_A/B) and paired-emitter pattern daAlink_c::setFirePointDamageEffect() uses to
set *Link's own body* on fire when he's hit by one (d_a_alink_effect.inc) - the same pair his
own wooden (Ordon) shield effect (ID_ZI_J_LK_SH_BURN_A/B, setWoodShieldBurnEffect()) is styled
after too - rather than the plain open-flame particle (ID_ZF_J_FIRE02_FIRE) already used for the
arrow's own trail above, which reads as a torch/campfire, not a person on fire. Despite the "LK"
in the name, this is an ordinary resource out of the common (always-resident) particle archive,
not something tied to Link's own actor or model - the same way every other particle ID in this
file is just looked up and spawned at an arbitrary world position with dComIfGp_particle_set(),
it works identically for any target.
--------------------------------------------------------------------------------------------

## Before/at: `static constexpr s16 BURNING_ACTOR_DURATION = 150;`
*(originally `mod.cpp` lines 642-650)*

How long (in frames) the cosmetic flame keeps burning on a hit target before it fades out on
its own. dusklight's simulation tick runs at a fixed 30Hz (dusk::game_clock::kSimPeriod =
1/30, in game_clock.h) - matching the original game's native frame rate - and mod_update()
(and therefore updateBurningActors() below) is driven once per sim tick, not once per rendered
frame, so "frames" here means 1/30 second each, not 1/60. 150 frames = 5 seconds, long enough
to clearly read as "this thing is on fire" for a good while without lingering indefinitely.
(A previous build of this constant assumed a 60fps tick rate and used 300, which actually
produced a 10 second burn - confirmed via timestamped log evidence showing a refresh-to-expiry
gap of ~10s, not the intended 5s.)

## Before/at: `static const JGeometry::TVec3<f32> BURN_EFFECT_SCALE(3.0f, 3.0f, 3.0f);`
*(originally `mod.cpp` lines 653-657)*

Uniformly enlarges the cosmetic flame to 3x its native particle size via setGlobalScale() - the
same generic, per-emitter scaling knob the game's own code already uses whenever a particle
effect needs to read as bigger than its default size (e.g. d_a_boomerang.cpp's effScale0,
d_a_alink_effect.inc's many *Scale locals) - so it's clearly visible as "this enemy is on fire"
rather than a faint flicker.

## Before/at: `struct BurningActor {`
*(originally `mod.cpp` lines 660-664)*

Tracks a single actor set alight by a fire-combo arrow, so its flame particle can be re-anchored
to the target's current position every frame for BURNING_ACTOR_DURATION frames. Kept external
(rather than stashed on the target actor itself) for the same reason TrackedFireArrow is kept
external to daArrow_c above: there's no spare field on an arbitrary enemy actor to repurpose,
and every enemy actor class is laid out differently.

## Before/at: `fpc_ProcID actorId = fpcM_ERROR_PROCESS_ID_e;`
*(originally `mod.cpp` lines 666-674)*

Identifies which actor this flame belongs to. Deliberately not paired with a stored raw
fopAc_ac_c* like TrackedFireArrow's arrowId is (see its comment above): once the target is
deleted (e.g. defeated and its corpse despawned), fopAcM_delete() just frees its slot back to
the actor pool without clearing or overwriting it, so a stored pointer would keep reading
back the same (still-correct-looking) id for as long as nothing new happens to reuse that
exact address - i.e. the flame would keep burning on a dead target indefinitely, which is
exactly the bug this field's previous approach had. fopAcM_SearchByID() (see
updateBurningActors below) looks the id up in the actor manager's own live list instead,
which is the only way to actually know whether the target is still alive right now.

## Before/at: `u32 particleKeyA = 0;`
*(originally `mod.cpp` lines 676-677)*

Two emitters per burning actor, matching setFirePointDamageEffect's own A/B pair (one for
the base flame, one for the sparks/embers on top) rather than a single particle.

## Before/at: `cXyz velocity = {0.0f, 0.0f, 0.0f};`
*(originally `mod.cpp` lines 680-682)*

Handed to both emitters via setUserWork() below; must outlive the emitters themselves, so
it's stored here rather than as a stack temporary (same reasoning as
TrackedFireArrow::velocity).

## Before/at: `s16 timer = 0;`
*(originally `mod.cpp` lines 684-685)*

Frames remaining; 0 means the slot is free. Decremented once per updateBurningActors() call
(i.e. once per mod_update(), once per game frame).

## Before/at: `static constexpr int MAX_BURNING_ACTORS = 8;`
*(originally `mod.cpp` lines 689-692)*

The player can volley several fire arrows at different targets in quick succession; a small
ring buffer comfortably covers that without needing a dynamic container. Sized the same as
MAX_TRACKED_FIRE_ARROWS for consistency, since both bound "how many things this mod is actively
animating fire on at once".

## Before/at: `static constexpr f32 EYEPOS_MAX_PLAUSIBLE_DIST = 300.0f;`
*(originally `mod.cpp` lines 697-722)*

Returns the best available world-space anchor point for a target's cosmetic burn flame.

Earlier attempts anchored the flame at the arrow's impact point, expressed as a fixed local
offset from the target's current.pos and re-projected into world space every frame using only
the target's current.angle.y (the same generic transform daE_KK_c::executeWalk uses to re-
attach a fixed offset to a turning actor, d_a_e_kk.cpp). That tracked ordinary turning fine, but
fell apart the moment the hit itself triggered a knockback/stagger reaction: daE_BG_c's generic
damage reaction (d_a_e_bg.cpp's executeDamage()) spins shape_angle.x and shape_angle.y - a
separate field from current.angle, used only for rendering (see mtx_set(), which builds the
actual model matrix from shape_angle, not current.angle) - so the body visibly tumbles/recoils
in ways no single yaw-only offset can follow, leaving the flame floating wherever the body
would have ended up had it not reacted to the hit at all.

actor->eyePos sidesteps this entirely: it's a generic field on every fopAc_ac_c (not something
this mod adds), and the large majority of enemy actor classes recompute it every single frame
from their model's actual current animated joint matrix (e.g. daE_BG_c::cc_set(), d_a_e_bg.cpp)
rather than from current.pos/current.angle at all - so it already reflects wherever the body is
really posed, tumbling, falling or otherwise, with no extra rotation math needed here.

Not every enemy class bothers maintaining it past actor creation though, where it's defaulted to
the actor's spawn point (f_op_actor.cpp sets actor->eyePos = actor->home.pos once, generically,
for every actor). For those classes eyePos would stay pinned at home.pos forever, drifting
arbitrarily far from the actor as it moves around. Guard against that by falling back to
current.pos (plus a small fixed vertical nudge so the flame doesn't anchor at ground/feet level)
whenever eyePos has drifted implausibly far from current.pos - far further than eyePos's own
local offset from the body (a handful of model-size units) would ever actually be.

## Before/at: `static void igniteBurningActor(fopAc_ac_c* actor) {`
*(originally `mod.cpp` lines 733-740)*

Starts (or refreshes, if already burning) the cosmetic flame on a hit target. Reuses a free slot
(timer == 0) if one exists so an actor that's still burning never gets evicted by an unrelated
new ignition elsewhere; otherwise evicts the oldest slot in ring-buffer order, exactly like
trackFireArrow does for arrows. An evicted slot's particle keys are simply abandoned rather than
explicitly stopped: like the arrow's own trail particle (see updateFireArrowEffect below), this
effect is kept alive purely by being re-issued every frame, so no longer calling
dComIfGp_particle_set() for it is already enough for it to stop emitting and fade out on its own
- there's no separate "stop" call needed or used anywhere else in this file for the same reason.

## Before/at: `static void updateBurningActors() {`
*(originally `mod.cpp` lines 771-775)*

Called once per game frame (from mod_update, see below) to tick down and re-anchor every
currently-burning target's flame particles. Deliberately not tied to the arrow's own execute
hook (on_arrow_execute_post): the arrow itself is typically deleted (embedded, despawned) long
before BURNING_ACTOR_DURATION elapses, but the cosmetic flame on whatever it hit should keep
burning independently of the arrow's own lifetime.

## Before/at: `fopAc_ac_c* actor = fopAcM_SearchByID(slot.actorId);`
*(originally `mod.cpp` lines 782-785)*

The authoritative "is this actor still alive" check (see actorId's own comment above):
searches the actor manager's live actor list by id instead of dereferencing a stored
pointer, so a defeated/despawned target is noticed the moment it's actually deleted,
rather than only once its freed memory happens to get reused by some other actor.

## Before/at: `if (JPABaseEmitter* emitterA = dComIfGp_particle_getEmitter(slot.particleKeyA);`
*(originally `mod.cpp` lines 796-808)*

ID_ZI_J_LK_BURNS_A/B are the same particle pair daAlink_c::setFirePointDamageEffect()
uses for Link's own on-fire effect (d_a_alink_effect.inc), and that function explicitly
checks emitter->isEnableDeleteEmitter() every frame and stops using the emitter the
moment it fires - unlike the arrow's flight-trail particle elsewhere in this file, this
pair is a self-terminating JPA effect with its own short authored lifetime, baked into
the particle resource itself and unaffected by how often dComIfGp_particle_set() is
called with the same key. Re-issuing a since-died key here just keeps repositioning (or
silently no-op'ing on) an emitter that had already finished on its own, so the visual
flame would otherwise go out ~once that authored lifetime elapsed - a few seconds after
it was first created - regardless of how many times igniteBurningActor() had since
refreshed slot.timer back up to BURNING_ACTOR_DURATION on a later hit. Forcing a brand
new emitter (key 0) the moment the old one reports itself as finished keeps the cosmetic
flame alive for exactly as long as the timer above says it should be.

## Before/at: `static constexpr f32 NOCKED_FLAME_SCALE = 1.0f / 3.0f;`
*(originally `mod.cpp` lines 839-865)*

Called every frame for every live fire arrow (see on_arrow_execute_post). Re-issues the fire
trail particle emitter (by reusing its key) at the arrow's up-to-date position, exactly like the
Bulblin (Bokoblin) archers' own fire arrows keep their trail attached to the arrow in
fire_eff_set() (d_a_e_arrow.cpp) instead of spawning a new, stationary burst once. Also mirrors
that function's use of the particle "trace" callback: without it, each emitter only knows the
position it was (re)issued at and its own particles don't get interpolated towards the arrow's
position in between frames, so the trail can lag behind or, if re-issued too infrequently
relative to the arrow's speed, appear to not be there at all.

The particle resources used are ID_ZF_J_FIRE02_FIRE (the flame itself) and ID_ZF_J_FIRE00_GLOW
(the soft additive halo around it) - the common (always-resident, never behind dPa_RM's
per-room-pack bit) pair that every stationary fire source in the game (torches in
d_a_obj_ktOnFire.cpp, campfires in d_a_obj_fireWood[2].cpp, candles in
d_a_obj_lv1Candle0[01].cpp) spawns together as their actual visible flame outside of rooms with
a fancier room-specific variant loaded. The GLOW particle is the close-up "lit up" look the
player actually sees around the flame itself; the LIGHT_INFLUENCE point light below
(FIRE_ARROW_LIGHT_COLOR/FIRE_ARROW_LIGHT_POW) only ever tints nearby actors/Link's own model and
a handful of specially-authored BG materials - the dKy_BossLight_set call further below
(FIRE_ARROW_BOSSLIGHT_COLOR/FIRE_ARROW_BOSSLIGHT_REF_DIST) is what actually brightens ordinary
terrain/walls around the arrow, the same way every one of those vanilla fixtures (and the
player's own Lantern) does it. The previous attempt (ID_IT_JN_ARWFIR_FIRE00) is a dead resource
ID never referenced anywhere else in the game and never actually renders regardless of which
pack is loaded.

While the arrow is nocked (waiting to be released), the flame is drawn just in front of the
player's face and at the full "impact-sized" scale would cover most of the first-person aiming
view. Everywhere else (flying, or stuck after landing) keeps the normal, unscaled size.

## Before/at: `static constexpr f32 GLOW_SCALE_FACTOR = 0.5f;`
*(originally `mod.cpp` lines 867-870)*

The GLOW particle on its own authored scale reads as roughly as large as a torch's, which would
look oversized next to a single arrow's flame and overpower the "smaller than the Lantern"
radius the point light above already uses. Shrinking it relative to the FIRE particle keeps the
halo proportional to the arrow's own small flame instead.

## Before/at: `static bool fireArrowBossLightSlotIsFree() {`
*(originally `mod.cpp` lines 873-886)*

dKy_BossLight_set's shared pool (see FIRE_ARROW_BOSSLIGHT_COLOR above) only has 5 general-use
slots (kankyo->field_0x0c18 indices 1-5; index 0 is the Lantern/Wolf-eye's own reserved slot),
and when full it evicts whichever existing entry is currently farthest from the camera to make
room for a new one - with up to 5 fire arrows now also competing for those same slots, that
eviction could (and did, per playtesting) silently snuff out a real nearby lit candle/torch/
firewood's terrain light to make room for an arrow's. The engine has no per-caller reservation
system to fix this with more slots (field_0x0c18 is a fixed-size member of the already-compiled
dScnKy_env_light_c, not something this mod's build - which only compiles against Dusklight's sdk/
headers, not the engine itself - can resize), so instead of fighting over the existing 5 slots,
this mod simply never forces that eviction itself: it only calls dKy_BossLight_set for an arrow
when a slot is already free, leaving every other active light (vanilla or otherwise) alone. This
mirrors dKy_BossLight_set's own slot-counting (including the stage_light_info_num room reservation
and the F_SP118/no-room-data edge cases) closely enough to know in advance whether its call would
have to evict something, without actually calling it and finding out.

## Before/at: `return false;`
*(originally `mod.cpp` lines 895-896)*

dKy_BossLight_set itself bails out (returns 0) in this same situation, so there's
never a free slot to claim here either.

## Before/at: `static TrackedFireArrow* findOldestFireArrowEffect(const TrackedFireArrow* i_requester) {`
*(originally `mod.cpp` lines 913-919)*

Picks which currently-lit fire arrow should give up its BossLight slot when the pool is full
and i_requester needs one. Only ever considers other tracked arrows that are themselves
currently holding a slot (bossLightActive) and haven't already given that slot up - this can
never pick an unrelated vanilla fixture (those aren't tracked here at all, so they're never
touched) and can never pick the same slot twice. Returns NULL if i_requester is itself the only
(or oldest) claimant, which simply means this particular arrow doesn't get a slot yet either,
the same as before this eviction logic existed.

## Before/at: `static void despawnFireArrowEffect(TrackedFireArrow& slot) {`
*(originally `mod.cpp` lines 933-948)*

Gives up a tracked arrow's claim on the BossLight terrain-light pool, to make room for a newer
arrow's own claim once the shared pool is full - see findOldestFireArrowEffect and its call site
in updateFireArrowEffect below. Everything else about this slot's effect - the trail/glow
particles, the actor/Link-only plight, and the arrow itself (which keeps flying, embedding, and
igniting flammable objects/enemies exactly as before) - is left completely untouched; losing a
terrain light to the pool's hard 5-slot cap shouldn't also snuff out the arrow's own visible
flame, the same way a candle or torch that momentarily loses its own BossLight slot to something
closer to the camera doesn't stop looking lit.

The BossLight terrain light itself has no "cut now" API; it's simply never re-issued again from
here on (bossLightGivenUp short-circuits the re-acquisition attempt in updateFireArrowEffect),
which is already this file's established way of letting a continuously-reissued effect end on
its own (see igniteBurningActor's own comment above for the same pattern) - the BossLight slot
this frees up won't actually read as free until the engine's own next-frame clear in exeKankyo(),
so the requester that triggered this eviction still won't get a slot itself this exact frame,
only starting next frame.

## Before/at: `static void updateFireArrowEffect(daArrow_c* arrow) {`
*(originally `mod.cpp` lines 954-956)*

Also re-anchors the ignition collider (see l_igniteSphSrc above) to the arrow's current
position every frame, the same way the arrow's own "stuck in wall" collider (field_0x7cc) is
simply re-issued at current.pos each frame rather than swept.

## Before/at: `arrow->setSmokePos();`
*(originally `mod.cpp` lines 960-969)*

arrow->current.pos is the arrow's *held* origin - while nocked, that's the bow's
grip/nock matrix (daAlink_c::getLeftItemMatrix(), see setKeepMatrix()), well behind
the arrowhead, which is why the flame used to show at the bowstring instead. Bomb
Arrows already solve exactly this for their fuse effect (setSmokePos(), which
transforms a fixed near-tip offset by the arrow model's current base matrix rather
than using current.pos), so reuse that same helper and its output (field_0x9cc) to
put our own flame in the same visual spot, correctly tracking the tip whether the
arrow is nocked, flying, or stuck. field_0x9cc/mSmokePos are otherwise only written
by mArrowType==1 (Bomb Arrow) code, which the fire-arrow combo never applies to, so
repurposing them here for a plain arrow doesn't collide with anything.

## Before/at: `slot.velocity = isArrowStationary(arrow) ? cXyz(0.0f, 0.0f, 0.0f) : arrow->speed;`
*(originally `mod.cpp` lines 972-978)*

While the arrow is nocked/waiting or has come to rest (stuck in a wall or an
actor), it isn't actually moving frame-to-frame even though arrow->speed may still
hold a stale, nonzero pre-impact (or never-set, pre-shot) value. Feeding that stale
vector to the trace callback would otherwise keep dragging already-emitted trail
particles forward indefinitely, e.g. straight through and out the other side of
whatever the arrow just stuck into. Zero it out in those cases instead, so the
trail settles into a small, stationary flame at the arrowhead.

## Before/at: `emitter->setGlobalScale(JGeometry::TVec3<f32>(scale, scale, scale));`
*(originally `mod.cpp` lines 989-997)*

setGlobalParticleScale only shrinks each already-spawned particle's rendered
billboard size (mGlobalPScl) - it leaves the emitter's own spawn volume
(mGlobalScl, the dynamics scale baked into its world matrix in
JPAResource.cpp/calcEmitterGlobalPosition) untouched, so at NOCKED_FLAME_SCALE
particles still spawn spread across the resource's full torch-sized volume,
just rendered as smaller sprites - visually a diffuse cloud no longer centered
tightly on field_0x9cc/the point light below. setGlobalScale() scales both the
spawn volume and the particle size together, keeping the flame compact and
centered on the same point the light uses at any scale.

## Before/at: `slot.glowParticleKey =`
*(originally `mod.cpp` lines 1001-1003)*

The actual visible "illumination" - see GLOW_SCALE_FACTOR/NOCKED_FLAME_SCALE above
for why it's scaled down from its authored (torch-sized) look, and the comment on
ID_ZF_J_FIRE02_FIRE above for why the point light below isn't enough on its own.

## Before/at: `f32 glowScale = scale * GLOW_SCALE_FACTOR;`
*(originally `mod.cpp` lines 1012-1014)*

See the FIRE emitter's setGlobalScale comment above - same reasoning applies
here so the glow halo stays centered on field_0x9cc instead of spreading across
its full authored (torch-sized) volume while nocked.

## Before/at: `slot.light.mPosition = arrow->field_0x9cc;`
*(originally `mod.cpp` lines 1019-1029)*

Illuminates the arrow's surroundings the same way every other visible flame in the
game does (see FIRE_ARROW_LIGHT_COLOR/FIRE_ARROW_LIGHT_POW above), re-anchored to the
same tip position as the trail particle above. Registered once lazily (the first
frame the arrow's flame actually shows, whether nocked or already flying) rather
than at trackFireArrow() time, so a light never ends up registered for an arrow that
was tracked but never actually got this far (there's no such path today, but this
keeps the lazy-init pattern consistent with the particle key above, which is also
only ever actually assigned here). Kept running unconditionally, independent of
whether this slot still holds (or ever held, or has since given up) a BossLight
terrain-light slot below - the flame/glow/plight trio is this arrow's own cosmetic
effect and shouldn't disappear just because the shared BossLight pool is full.

## Before/at: `if (!slot.bossLightGivenUp) {`
*(originally `mod.cpp` lines 1039-1055)*

See FIRE_ARROW_BOSSLIGHT_COLOR/FIRE_ARROW_BOSSLIGHT_REF_DIST above - this is the
actual mechanism (shared with every vanilla fire fixture and the Lantern itself)
that visibly brightens nearby terrain, as a companion to the plight above. Must be
re-issued every frame (unlike dKy_plight_set's register-once pattern), since the
engine clears this slot array itself each frame before any of this frame's callers
get a chance to re-claim it. Only claimed when a slot is actually free (see
fireArrowBossLightSlotIsFree above) so an arrow's own terrain light never evicts an
already-lit vanilla fixture (or another arrow's) out of the shared, fixed-size pool.
If it isn't free, rather than simply giving up for this arrow, look for another fire
arrow's own BossLight claim to give up instead (see findOldestFireArrowEffect/
despawnFireArrowEffect above) - never a vanilla fixture's, since those were never
tracked here to begin with. That frees a slot starting next frame, not this one (see
despawnFireArrowEffect's own comment on why), so this arrow still doesn't get a
terrain light of its own this exact frame either way. Skipped entirely once this
slot has itself already given up its own BossLight claim (see bossLightGivenUp's own
comment above) - the flame/glow/plight above keeps running regardless, only this
slot's pursuit of a terrain light stops for good.

## Before/at: `static_cast<cM3dGCps*>(&slot.igniteCps)->Set(arrow->old.pos, arrow->current.pos, 50.0f);`
*(originally `mod.cpp` lines 1071-1080)*

A capsule (unlike a sphere) needs actual endpoints, not just a center + radius:
Set() the same way field_0x688 does every frame in setArrowAt() (d_a_arrow.cpp),
except spanning the arrow's real, already-travelled distance this frame
(old.pos -> current.pos) rather than a forward-projected guess - all that matters
here is that nothing the arrow visually passed through this frame gets skipped,
which a straight cast of the frame's actual movement guarantees regardless of
arrow speed. CalcAtVec() (as field_0x688's own Set() call is likewise always
followed by) derives the collider's direction vector from these same two points;
it needs to be recomputed any time the endpoints move, so it's not something that
can be set once up front and left alone.

## Before/at: `if (!slot.igniteCps.ChkAtSet()) {`
*(originally `mod.cpp` lines 1084-1091)*

Mirrors daAlink_c's own real lantern-swing collider (d_a_alink.cpp, the
checkKandelaarSwing(1) branch): setting the shape alone isn't enough for it to
actually register hits, since the collision system skips any At-side collider
whose "set" bit isn't on (see ChkAtSet() gating in d_cc_mass_s.cpp/d_cc_s.cpp).
l_igniteCpsSrc's mObjAt.mBase.mSPrm doesn't include that bit (see its comment for
the rest of that value), so it has to be turned on explicitly the first time this
collider becomes active, the same way daAlink_c's mAtSph starts off and gets
OnAtSetBit()'d only once an actual swing begins.

## Before/at: `if (!slot.hitActorHandled && arrow->field_0x688.ChkAtHit()) {`
*(originally `mod.cpp` lines 1098-1120)*

field_0x688 is the arrow's own main collider (AT_TYPE_ARROW, unaffected by
igniteActive above), so this fires for any confirmed hit as soon as the arrow is
actually released, independent of whether it also landed near anything ignitable.
ChkAtHit() stays true for as long as the arrow remains lodged in whatever it hit
(there's no per-frame reset, see field_0x688.ResetAtHit()'s own call sites in
d_a_arrow.cpp), so hitActorHandled latches this to a single reaction per arrow
rather than re-igniting the same target every subsequent frame.

GetAtHitAc() itself is a lazy-resolving cache (dCcD_GAtTgCoCommonBase::GetAc(),
d_cc_d.cpp): it turns the hit's recorded apid into an actual fopAc_ac_c* via
fopAcM_SearchByID() the first time it's asked, and can still legitimately read back
NULL on the very first frame ChkAtHit() goes true if that resolution hasn't
happened yet (e.g. an enemy that deflects/bounces the arrow off a shield-flagged
collider - see daE_KK_c/Chilfos's mCyl.ChkTgShield() toggling and d_a_arrow.cpp's
atHitCallBack field_0x93e branch - skips the embed-position lookup a stuck arrow's
own hit resolution otherwise forces to happen synchronously, so there's nothing
else guaranteeing the apid is already resolvable that same frame). Latching
hitActorHandled unconditionally on ChkAtHit() alone, like a previous version of
this code did, could therefore permanently give up on a hit whose actor pointer
simply wasn't resolvable yet - silently skipping ignition - and never get another
chance to look again on a later frame even if ChkAtHit() was still true then. Only
latching once GetAtHitAc() actually resolves to a real actor avoids that: a NULL
read just leaves hitActorHandled false and tries again next frame instead.

## Before/at: `if (fopAcM_GetGroup(hitActor) == fopAc_ENEMY_e) {`
*(originally `mod.cpp` lines 1126-1151)*

Restricted to enemies only, via the actor's own group tag (its profile's
"Group" field, fopAcM_GetGroup()) rather than dynamic_cast<fopEn_enemy_c*>:
fopEn_enemy_c is a class compiled separately into this mod and into the base
game binary, and a cross-module dynamic_cast's success depends on the two
sides' RTTI (typeinfo/vtable) agreeing exactly - unlike a plain data field read
through a pointer, that isn't guaranteed to hold here, and in practice the cast
against real enemy actors was never succeeding, so the effect never triggered.
fopAcM_GetGroup() == fopAc_ENEMY_e instead just reads a POD field set by each
actor's own static profile table (e.g. d_a_e_bg.cpp's "/* Group */
fopAc_ENEMY_e"), with no RTTI involved - it's the same test daArrow_c's own hit
callback already uses for its "100m headshot" achievement check
(atHitCallBack(), above in this same file) and daAlink_c's targeting code use for
their own "is this actor an enemy" checks, so it's proven to work reliably
across this exact mod/game boundary. This also implicitly excludes the player
(daAlink_c's own profile group is fopAc_PLAYER_e): a fire arrow striking Link
(e.g. an errant shot, or a reflected/deflected one) already makes him visibly
catch fire and take periodic burn damage through his own, unrelated vanilla
mechanism (dCcD_MTRL_FIRE handling in daAlink_c's own damage code) - layering
this purely cosmetic effect on top of that would just double up the flame
visuals on the same target. It equally excludes non-enemy actors a fire arrow
might still strike (NPCs, animals, carriable objects, etc.), which shouldn't
visibly catch fire at all. A small handful of enemy types are tagged with a
different group in their own profile (e.g. the Poison Mite swarm,
daE_Bug_HIO_c/e_bug_class in d_a_e_bug.h, uses fopAc_ACTOR_e) and won't get the
effect either - there's no fully generic way to catch those too without a
per-type exception.

## Before/at: `static void updateFireArrows() {`
*(originally `mod.cpp` lines 1163-1172)*

Called once per game frame (from mod_update, see below), independently of any particular
arrow's own execute hook, to cut the point light for any tracked slot whose arrow has actually
been deleted (embedded-forever arrows aside, eventually despawned/recycled by the actor pool)
without that slot ever being reused by a new fire arrow at the same address. Without this, such
a light would stay registered with the engine - and visibly lit - at its last position
indefinitely, since dKy_plight_set()'d lights (unlike the self-terminating trail particle) don't
expire on their own. Uses the same fopAcM_SearchByID()-based liveness check as
updateBurningActors above, for the same reason: fpcM_GetID() on a stored pointer can't tell "the
arrow is gone" from "the arrow is still here", since fopAcM_delete() never clears the freed
memory (see BurningActor::actorId's own comment above).

## Before/at: `slot.bossLightActive = false;`
*(originally `mod.cpp` lines 1178-1181)*

The arrow is gone, so updateFireArrowEffect will never run for this slot again
(and its BossLight call has therefore already stopped being re-issued) until it's
reclaimed by a new arrow - keep findOldestFireArrowEffect from treating a long-dead
slot as still actively holding a pool slot in the meantime.

## Before/at: `dComIfGp_setItemOilCount(-link->mpHIO->mItem.mLantern.m.mShakeOilLoss);`
*(originally `mod.cpp` lines 1197-1197)*

Same oil cost as a single lantern swing (daAlink_c::initKandelaarSwing).

## Before/at: `arrow->field_0x688.SetAtMtrl(dCcD_MTRL_FIRE);`
*(originally `mod.cpp` lines 1200-1204)*

Mark the arrow's attack collider as fire, same as how Light Arrows mark theirs as
dCcD_MTRL_LIGHT a few lines below in the original function. This keeps field_0x688's AtType
at its default AT_TYPE_ARROW (so normal arrow damage and enemy-specific arrow-hit reactions
still work), while still giving the arrow the elemental "fire" material property, exactly
like Light Arrows do for dCcD_MTRL_LIGHT without touching their own AtType either.

## Before/at: `trackFireArrow(arrow);`
*(originally `mod.cpp` lines 1207-1212)*

The actual AT_TYPE_LANTERN_SWING ignition effect is provided by a second, independent
collider tracked alongside the arrow (see trackFireArrow/l_igniteSphSrc below) rather than
by this collider, since ignitable objects require an exact AtType match and AT_TYPE_ARROW
can't be combined with AT_TYPE_LANTERN_SWING on one collider without losing one or the
other. Usually already tracked from nock time (see on_alink_make_arrow_post below), so this
is normally just a no-op that turns the ignition collider itself on.

## Before/at: `static void on_alink_make_arrow_post(ModContext*, void* args, void*, void*) {`
*(originally `mod.cpp` lines 1222-1227)*

Runs right after the Hero's Bow nocks a new arrow (drawing the bow back, or switching arrow
types mid-draw). Marks the arrow as a fire arrow immediately, purely so its cosmetic flame
effect (see updateFireArrowEffect) already shows on the arrowhead while the shot is being lined
up, matching how it looks once actually fired. Doesn't touch oil (that's only spent once the
arrow is actually released, in on_arrow_shooting_pre) or the ignition collider (see
activateFireArrowIgnition), so merely drawing the bow back near a torch can't ignite it.

