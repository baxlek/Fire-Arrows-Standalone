#include "mods/service.hpp"
#include "mods/svc/config.h"
#include "mods/svc/hook.hpp"
#include "mods/svc/log.hpp"
#include "mods/svc/ui.h"

#include <cstring>

#include "Z2AudioLib/Z2AudioMgr.h"
#include "Z2AudioLib/Z2SeMgr.h"
#include "d/actor/d_a_alink.h"
#include "d/actor/d_a_arrow.h"
#include "d/d_cc_d.h"
#include "d/d_com_inf_game.h"
#include "d/d_item_data.h"
#include "d/d_kankyo.h"
#include "d/d_menu_ring.h"
#include "d/d_meter2_draw.h"
#include "d/d_particle_name.h"
#include "d/d_save.h"
#include "f_op/f_op_actor_mng.h"
#include "f_pc/f_pc_manager.h"

DEFINE_MOD();

IMPORT_SERVICE(HookService, svc_hook);
IMPORT_SERVICE(LogService, svc_log);
IMPORT_SERVICE(ConfigService, svc_config);
IMPORT_SERVICE(UiService, svc_ui);

static ConfigVarHandle g_terrainLightingVar = 0;

static bool terrainLightingEnabled() {
    bool enabled = true;
    svc_config->get_bool(mod_ctx, g_terrainLightingVar, &enabled);
    return enabled;
}

static constexpr u8 NO_MIX_ITEM = 0xFF;

DEFINE_HOOK(&dMenu_Ring_c::setMixItem, SetMixItem);
DEFINE_HOOK(&dMenu_Ring_c::isMixItemOn, IsMixItemOn);
DEFINE_HOOK(&dMenu_Ring_c::isMixItemOff, IsMixItemOff);

DEFINE_HOOK(&dMenu_Ring_c::setActiveCursor, SetActiveCursor);

static constexpr HookOptions kDisguiseHookOptions = {
    sizeof(HookOptions), 1'000'000, HOOK_REPLACE_CONFLICT, nullptr};

DEFINE_HOOK(&daArrow_c::arrowShooting, ArrowShooting);

DEFINE_HOOK(&daAlink_c::makeArrow, AlinkMakeArrow);

DEFINE_HOOK(&daArrow_c::execute, ArrowExecute);

DEFINE_HOOK(&dMeter2Draw_c::drawKanteraMeter, DrawKanteraMeter);

DEFINE_HOOK(&daAlink_c::changeArrowType, ChangeArrowType);

DEFINE_HOOK(&daAlink_c::setBowReadyAnime, SetBowReadyAnime);

static u8 g_disguisedItemSlot = NO_MIX_ITEM;

static void disguiseLanternPre(dMenu_Ring_c* menu) {
    u8 slot = menu->mItemSlots[menu->mCurrentSlot];

    if (dComIfGs_getItem(slot, false) == dItemNo_KANTERA_e) {
        dComIfGs_setItem(slot, dItemNo_HAWK_EYE_e);
        g_disguisedItemSlot = slot;
    }
}

static void restoreLanternPost() {
    if (g_disguisedItemSlot != NO_MIX_ITEM) {
        dComIfGs_setItem(g_disguisedItemSlot, dItemNo_KANTERA_e);
        g_disguisedItemSlot = NO_MIX_ITEM;
    }
}

static HookAction on_set_mix_item_pre(ModContext*, void* args, void*, void*) {
    disguiseLanternPre(mods::arg<dMenu_Ring_c*>(args, 0));
    return HOOK_CONTINUE;
}

static void on_set_mix_item_post(ModContext*, void* args, void*, void*) {
    restoreLanternPost();

    mods::arg<dMenu_Ring_c*>(args, 0)->setJumpItem(false);
}

static HookAction on_is_mix_item_on_pre(ModContext*, void* args, void*, void*) {
    disguiseLanternPre(mods::arg<dMenu_Ring_c*>(args, 0));
    return HOOK_CONTINUE;
}

static void on_is_mix_item_on_post(ModContext*, void*, void*, void*) {
    restoreLanternPost();
}

static HookAction on_is_mix_item_off_pre(ModContext*, void* args, void*, void*) {
    disguiseLanternPre(mods::arg<dMenu_Ring_c*>(args, 0));
    return HOOK_CONTINUE;
}

static void on_is_mix_item_off_post(ModContext*, void*, void*, void*) {
    restoreLanternPost();
}

static HookAction on_set_active_cursor_pre(ModContext*, void* args, void*, void*) {
    disguiseLanternPre(mods::arg<dMenu_Ring_c*>(args, 0));
    return HOOK_CONTINUE;
}

static void on_set_active_cursor_post(ModContext*, void*, void*, void*) {
    restoreLanternPost();
}

static bool checkBowLanternComboInSlot(int selectItemIdx) {
    if (dComIfGp_getSelectItem(selectItemIdx) != dItemNo_BOW_e) {
        return false;
    }

    u8 partnerSlot = dComIfGs_getSelectItemIndex(selectItemIdx);
    return dComIfGs_getItem(partnerSlot, false) == dItemNo_KANTERA_e;
}

static bool checkBowLanternComboInExtraSlot(int selectItemIdx) {
    if (dComIfGs_getMixItemIndex(selectItemIdx) != SLOT_4) {
        return false;
    }

    u8 partnerSlot = dComIfGs_getSelectItemIndex(selectItemIdx);
    return dComIfGs_getItem(partnerSlot, false) == dItemNo_KANTERA_e;
}

static bool checkBowLanternCombo() {
    for (int selectItemIdx = 0; selectItemIdx < SELECT_ITEM_NUM; selectItemIdx++) {
        if (checkBowLanternComboInSlot(selectItemIdx)) {
            return true;
        }
    }

    for (int selectItemIdx = SELECT_ITEM_NUM; selectItemIdx < MAX_SELECT_ITEM; selectItemIdx++) {
        if (checkBowLanternComboInExtraSlot(selectItemIdx)) {
            return true;
        }
    }

    return false;
}

static bool g_fireArrowModeOn = true;

static HookAction on_change_arrow_type_pre(ModContext*, void* args, void*, void*) {
    daAlink_c* link = mods::arg<daAlink_c*>(args, 0);

    if (!checkBowLanternCombo()) {
        return HOOK_CONTINUE;
    }

    if (link->checkCanoeSlider() || (!g_fireArrowModeOn && dComIfGs_getOil() == 0)) {
        return HOOK_SKIP_ORIGINAL;
    }

    link->setItemActionButtonStatus(BUTTON_STATUS_SWITCH);

    if (link->arrowChangeTrigger()) {
        g_fireArrowModeOn = !g_fireArrowModeOn;

        if (link->mItemAcKeep.getActor() != NULL) {
            link->deleteArrow();
            link->makeArrow();
            link->setBowReloadAnime();
        }
    }

    return HOOK_SKIP_ORIGINAL;
}

static HookAction on_set_bow_ready_anime_pre(ModContext*, void* args, void*, void*) {
    daAlink_c* link = mods::arg<daAlink_c*>(args, 0);

    if (checkBowLanternCombo() && !link->checkBowAnime()) {
        g_fireArrowModeOn = dComIfGs_getOil() != 0;
    }

    return HOOK_CONTINUE;
}

static HookAction on_draw_kantera_meter_pre(ModContext*, void* args, void*, void*) {
    u8 button = mods::arg<u8>(args, 1);
    f32& alpha = mods::arg_ref<f32>(args, 2);

    if (alpha == 0.0f && button < SELECT_ITEM_NUM && checkBowLanternComboInSlot(button)) {
        alpha = 1.0f;
    }

    return HOOK_CONTINUE;
}

static const dCcD_SrcCps l_igniteCpsSrc = {
    {
        {0x0, {{AT_TYPE_LANTERN_SWING, 0x0, 0x1A}, {0x0, 0x0}, 0x0}},
        {dCcD_SE_NONE, 0x0, 0x0, dCcD_MTRL_FIRE, 0x0},
        {dCcD_SE_NONE, 0x0, 0x0, 0x0, 0x0},
        {0x0},
    },
    {
        {{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, 50.0f}
    }
};

static constexpr GXColorS10 FIRE_ARROW_LIGHT_COLOR = {0xBC, 0x66, 0x42, 0xFF};
static constexpr f32 FIRE_ARROW_LIGHT_FLUCTUATION = 1.0f;
static constexpr f32 FIRE_ARROW_LIGHT_POW = 250.0f;

static constexpr GXColor FIRE_ARROW_BOSSLIGHT_COLOR = {0xBC, 0x66, 0x42, 0xFF};
static constexpr f32 FIRE_ARROW_BOSSLIGHT_REF_DIST = 0.5f;

struct TrackedFireArrow {
    daArrow_c* arrow = nullptr;
    fpc_ProcID arrowId = fpcM_ERROR_PROCESS_ID_e;
    u32 particleKey = 0;
    u32 glowParticleKey = 0;
    cXyz velocity = {0.0f, 0.0f, 0.0f};
    dCcD_Cps igniteCps;
    dCcD_Stts igniteStts;
    bool igniteActive = false;
    bool hitActorHandled = false;
    LIGHT_INFLUENCE light;
    bool lightActive = false;
    bool bossLightActive = false;
    u32 age = 0;
    bool bossLightGivenUp = false;
};

static constexpr int MAX_TRACKED_FIRE_ARROWS = 8;
static TrackedFireArrow g_fireArrows[MAX_TRACKED_FIRE_ARROWS];
static int g_nextFireArrowSlot = 0;
static u32 g_nextFireArrowAge = 0;

static void initFireArrowSlot(TrackedFireArrow& slot, daArrow_c* arrow, fpc_ProcID id) {
    if (slot.lightActive) {
        dKy_plight_cut(&slot.light);
        slot.lightActive = false;
    }

    slot.arrow = arrow;
    slot.arrowId = id;
    slot.particleKey = 0;
    slot.glowParticleKey = 0;
    slot.igniteActive = false;
    slot.hitActorHandled = false;
    slot.bossLightActive = false;
    slot.age = g_nextFireArrowAge++;
    slot.bossLightGivenUp = false;
    slot.igniteStts.Init(10, 0xff, arrow);
    slot.igniteCps.Set(l_igniteCpsSrc);
    slot.igniteCps.SetStts(&slot.igniteStts);
}

static void trackFireArrow(daArrow_c* arrow) {
    fpc_ProcID id = fpcM_GetID(arrow);

    for (TrackedFireArrow& slot : g_fireArrows) {
        if (slot.arrow == arrow) {
            if (slot.arrowId != id) {
                initFireArrowSlot(slot, arrow, id);
            }
            return;
        }
    }

    TrackedFireArrow& slot = g_fireArrows[g_nextFireArrowSlot];
    g_nextFireArrowSlot = (g_nextFireArrowSlot + 1) % MAX_TRACKED_FIRE_ARROWS;
    initFireArrowSlot(slot, arrow, id);
}

static void activateFireArrowIgnition(daArrow_c* arrow) {
    for (TrackedFireArrow& slot : g_fireArrows) {
        if (slot.arrow == arrow) {
            slot.igniteActive = true;
            return;
        }
    }
}

static bool isArrowStationary(daArrow_c* arrow) {
    return arrow->mProcFunc == &daArrow_c::procWait ||
           arrow->mProcFunc == &daArrow_c::procBGStop ||
           arrow->mProcFunc == &daArrow_c::procActorStop ||
           arrow->mProcFunc == &daArrow_c::procActorControllStop;
}

static constexpr s16 BURNING_ACTOR_DURATION = 150;

static const JGeometry::TVec3<f32> BURN_EFFECT_SCALE(3.0f, 3.0f, 3.0f);

struct BurningActor {
    fpc_ProcID actorId = fpcM_ERROR_PROCESS_ID_e;
    u32 particleKeyA = 0;
    u32 particleKeyB = 0;
    cXyz velocity = {0.0f, 0.0f, 0.0f};
    s16 timer = 0;
};

static constexpr int MAX_BURNING_ACTORS = 8;
static BurningActor g_burningActors[MAX_BURNING_ACTORS];
static int g_nextBurningActorSlot = 0;

static constexpr f32 EYEPOS_MAX_PLAUSIBLE_DIST = 300.0f;
static constexpr f32 FALLBACK_ANCHOR_HEIGHT = 40.0f;

static cXyz getBurnAnchorPos(fopAc_ac_c* actor) {
    if (actor->eyePos.abs(actor->current.pos) <= EYEPOS_MAX_PLAUSIBLE_DIST) {
        return actor->eyePos;
    }
    return actor->current.pos + cXyz(0.0f, FALLBACK_ANCHOR_HEIGHT, 0.0f);
}

static void igniteBurningActor(fopAc_ac_c* actor) {
    fpc_ProcID id = fpcM_GetID(actor);

    for (int i = 0; i < MAX_BURNING_ACTORS; i++) {
        BurningActor& slot = g_burningActors[i];
        if (slot.timer != 0 && slot.actorId == id) {
            slot.timer = BURNING_ACTOR_DURATION;
            return;
        }
    }

    for (int i = 0; i < MAX_BURNING_ACTORS; i++) {
        BurningActor& slot = g_burningActors[i];
        if (slot.timer == 0) {
            slot.actorId = id;
            slot.particleKeyA = 0;
            slot.particleKeyB = 0;
            slot.timer = BURNING_ACTOR_DURATION;
            return;
        }
    }

    BurningActor& slot = g_burningActors[g_nextBurningActorSlot];
    g_nextBurningActorSlot = (g_nextBurningActorSlot + 1) % MAX_BURNING_ACTORS;
    slot.actorId = id;
    slot.particleKeyA = 0;
    slot.particleKeyB = 0;
    slot.timer = BURNING_ACTOR_DURATION;
}

static void updateBurningActors() {
    for (BurningActor& slot : g_burningActors) {
        if (slot.timer == 0) {
            continue;
        }

        fopAc_ac_c* actor = fopAcM_SearchByID(slot.actorId);
        if (actor == NULL) {
            slot.actorId = fpcM_ERROR_PROCESS_ID_e;
            slot.timer = 0;
            continue;
        }

        slot.timer--;
        slot.velocity = actor->speed;

        if (JPABaseEmitter* emitterA = dComIfGp_particle_getEmitter(slot.particleKeyA);
            emitterA != NULL && emitterA->isEnableDeleteEmitter()) {
            slot.particleKeyA = 0;
        }
        if (JPABaseEmitter* emitterB = dComIfGp_particle_getEmitter(slot.particleKeyB);
            emitterB != NULL && emitterB->isEnableDeleteEmitter()) {
            slot.particleKeyB = 0;
        }

        cXyz pos = getBurnAnchorPos(actor);
        slot.particleKeyA =
            dComIfGp_particle_set(slot.particleKeyA, ID_ZI_J_LK_BURNS_A, &pos, NULL, NULL);
        slot.particleKeyB =
            dComIfGp_particle_set(slot.particleKeyB, ID_ZI_J_LK_BURNS_B, &pos, NULL, NULL);

        for (u32 key : {slot.particleKeyA, slot.particleKeyB}) {
            JPABaseEmitter* emitter = dComIfGp_particle_getEmitter(key);
            if (emitter != NULL) {
                emitter->setParticleCallBackPtr(dPa_control_c::getParticleTracePCB());
                emitter->setUserWork((uintptr_t)&slot.velocity);
                emitter->setGlobalScale(BURN_EFFECT_SCALE);
            }
        }

        if (slot.timer == 0) {
            slot.actorId = fpcM_ERROR_PROCESS_ID_e;
        }
    }
}

static constexpr f32 NOCKED_FLAME_SCALE = 1.0f / 3.0f;
static constexpr f32 GLOW_SCALE_FACTOR = 0.5f;

static bool fireArrowBossLightSlotIsFree() {
    dScnKy_env_light_c* kankyo = dKy_getEnvlight();

    int stageLightInfoNum = 0;
    if (strcmp(dComIfGp_getStartStageName(), "F_SP118") != 0) {
        dStage_roomDt_c* roomDt =
            dComIfGp_roomControl_getStatusRoomDt(dComIfGp_roomControl_getStayNo());
        if (roomDt == NULL) {
            return false;
        }
        stageLightInfoNum = roomDt->getLightVecInfoNum();
        if (stageLightInfoNum < 0 || stageLightInfoNum > 6) {
            stageLightInfoNum = 6;
        }
    }

    for (int i = 1; i < 6 - stageLightInfoNum; i++) {
        if (kankyo->field_0x0c18[i].field_0x26 == 0) {
            return true;
        }
    }
    return false;
}

static TrackedFireArrow* findOldestFireArrowEffect(const TrackedFireArrow* i_requester) {
    TrackedFireArrow* oldest = NULL;
    for (TrackedFireArrow& slot : g_fireArrows) {
        if (&slot == i_requester || !slot.bossLightActive || slot.bossLightGivenUp) {
            continue;
        }
        if (oldest == NULL || slot.age < oldest->age) {
            oldest = &slot;
        }
    }
    return oldest;
}

static void despawnFireArrowEffect(TrackedFireArrow& slot) {
    slot.bossLightActive = false;
    slot.bossLightGivenUp = true;
}

static void updateFireArrowEffect(daArrow_c* arrow) {
    for (TrackedFireArrow& slot : g_fireArrows) {
        if (slot.arrow == arrow) {
            arrow->setSmokePos();

            slot.velocity = isArrowStationary(arrow) ? cXyz(0.0f, 0.0f, 0.0f) : arrow->speed;
            slot.particleKey =
                dComIfGp_particle_set(slot.particleKey, ID_ZF_J_FIRE02_FIRE,
                                      &arrow->field_0x9cc, &arrow->shape_angle, NULL);

            JPABaseEmitter* emitter = dComIfGp_particle_getEmitter(slot.particleKey);
            f32 scale = arrow->mProcFunc == &daArrow_c::procWait ? NOCKED_FLAME_SCALE : 1.0f;
            if (emitter != NULL) {
                emitter->setParticleCallBackPtr(dPa_control_c::getParticleTracePCB());
                emitter->setUserWork((uintptr_t)&slot.velocity);
                emitter->setGlobalScale(JGeometry::TVec3<f32>(scale, scale, scale));
            }

            slot.glowParticleKey =
                dComIfGp_particle_set(slot.glowParticleKey, ID_ZF_J_FIRE00_GLOW,
                                      &arrow->field_0x9cc, &arrow->shape_angle, NULL);

            JPABaseEmitter* glowEmitter = dComIfGp_particle_getEmitter(slot.glowParticleKey);
            if (glowEmitter != NULL) {
                glowEmitter->setParticleCallBackPtr(dPa_control_c::getParticleTracePCB());
                glowEmitter->setUserWork((uintptr_t)&slot.velocity);
                f32 glowScale = scale * GLOW_SCALE_FACTOR;
                glowEmitter->setGlobalScale(JGeometry::TVec3<f32>(glowScale, glowScale, glowScale));
            }

            slot.light.mPosition = arrow->field_0x9cc;
            if (!slot.lightActive) {
                slot.light.mColor = FIRE_ARROW_LIGHT_COLOR;
                slot.light.mPow = FIRE_ARROW_LIGHT_POW;
                slot.light.mFluctuation = FIRE_ARROW_LIGHT_FLUCTUATION;
                dKy_plight_set(&slot.light);
                slot.lightActive = true;
            }

            if (!terrainLightingEnabled()) {
                slot.bossLightActive = false;
            } else if (!slot.bossLightGivenUp) {
                if (fireArrowBossLightSlotIsFree()) {
                    GXColor bossLightColor = FIRE_ARROW_BOSSLIGHT_COLOR;
                    dKy_BossLight_set(&arrow->field_0x9cc, &bossLightColor,
                                       FIRE_ARROW_BOSSLIGHT_REF_DIST, 0);
                    slot.bossLightActive = true;
                } else {
                    TrackedFireArrow* oldest = findOldestFireArrowEffect(&slot);
                    if (oldest != NULL) {
                        despawnFireArrowEffect(*oldest);
                    }
                }
            }

            if (slot.igniteActive) {
                static_cast<cM3dGCps*>(&slot.igniteCps)->Set(arrow->old.pos, arrow->current.pos, 50.0f);
                slot.igniteCps.CalcAtVec();

                if (!slot.igniteCps.ChkAtSet()) {
                    slot.igniteCps.OnAtSetBit();
                }
                dComIfG_Ccsp()->Set(&slot.igniteCps);
            }

            if (!slot.hitActorHandled && arrow->field_0x688.ChkAtHit()) {
                fopAc_ac_c* hitActor = arrow->field_0x688.GetAtHitAc();
                if (hitActor != NULL) {
                    slot.hitActorHandled = true;

                    if (fopAcM_GetGroup(hitActor) == fopAc_ENEMY_e) {
                        igniteBurningActor(hitActor);
                    }
                }
            }

            return;
        }
    }
}

static void updateFireArrows() {
    for (TrackedFireArrow& slot : g_fireArrows) {
        if (slot.lightActive && fopAcM_SearchByID(slot.arrowId) == NULL) {
            dKy_plight_cut(&slot.light);
            slot.lightActive = false;
            slot.bossLightActive = false;
        }
    }
}

static HookAction on_arrow_shooting_pre(ModContext*, void* args, void*, void*) {
    daArrow_c* arrow = mods::arg<daArrow_c*>(args, 0);

    daAlink_c* link = daAlink_getAlinkActorClass();
    if (link == nullptr || !checkBowLanternCombo() || !g_fireArrowModeOn ||
        dComIfGs_getOil() == 0)
    {
        return HOOK_CONTINUE;
    }

    dComIfGp_setItemOilCount(-link->mpHIO->mItem.mLantern.m.mShakeOilLoss);

    arrow->field_0x688.SetAtMtrl(dCcD_MTRL_FIRE);

    trackFireArrow(arrow);
    activateFireArrowIgnition(arrow);

    Z2GetAudioMgr()->seStart(Z2SE_OBJ_ARROW_SHOT_FIRE, &arrow->current.pos, 0, 0, 1.0f, 1.0f, -1.0f,
                             -1.0f, 0);

    return HOOK_CONTINUE;
}

static void on_alink_make_arrow_post(ModContext*, void* args, void*, void*) {
    daAlink_c* link = mods::arg<daAlink_c*>(args, 0);
    if (!checkBowLanternCombo() || !g_fireArrowModeOn || dComIfGs_getOil() == 0) {
        return;
    }

    fopAc_ac_c* actor = link->mItemAcKeep.getActor();
    if (actor == NULL || fopAcM_GetName(actor) != fpcNm_ARROW_e) {
        return;
    }

    daArrow_c* arrow = static_cast<daArrow_c*>(actor);
    arrow->field_0x688.SetAtMtrl(dCcD_MTRL_FIRE);
    trackFireArrow(arrow);
}

static void on_arrow_execute_post(ModContext*, void* args, void*, void*) {
    daArrow_c* arrow = mods::arg<daArrow_c*>(args, 0);
    if (arrow->field_0x688.GetAtMtrl() == dCcD_MTRL_FIRE) {
        updateFireArrowEffect(arrow);
    }
}

static ModResult buildFireArrowsModsPanel(
    ModContext*, UiElementHandle panel, void*, ModError*) {
    UiControlDesc terrainLightingControl = UI_CONTROL_DESC_INIT;
    terrainLightingControl.kind = UI_CONTROL_TOGGLE;
    terrainLightingControl.label = "Terrain Lighting";
    terrainLightingControl.help_rml =
        "Whether fire arrows cast dynamic light onto nearby terrain.";
    terrainLightingControl.binding = UI_BINDING_CONFIG_VAR;
    terrainLightingControl.config_var = g_terrainLightingVar;
    return svc_ui->pane_add_control(mod_ctx, panel, &terrainLightingControl, nullptr);
}

extern "C" {
MOD_EXPORT ModResult mod_initialize(ModError*) {
    ConfigVarDesc terrainLightingDesc = CONFIG_VAR_DESC_INIT;
    terrainLightingDesc.name = "terrainLighting";
    terrainLightingDesc.type = CONFIG_VAR_BOOL;
    terrainLightingDesc.default_bool = true;
    ModResult configResult =
        svc_config->register_var(mod_ctx, &terrainLightingDesc, &g_terrainLightingVar);
    if (configResult != MOD_OK) {
        mods::log::error("failed to register terrainLighting config var");
        return configResult;
    }

    UiModsPanelDesc panelDesc = UI_MODS_PANEL_DESC_INIT;
    panelDesc.build = buildFireArrowsModsPanel;
    configResult = svc_ui->register_mods_panel(mod_ctx, &panelDesc);
    if (configResult != MOD_OK) {
        mods::log::error("failed to register fire_arrows mods panel");
        return configResult;
    }

    ModResult result = mods::hook::add_pre<SetMixItem>(on_set_mix_item_pre, &kDisguiseHookOptions);
    if (result != MOD_OK) {
        mods::log::error("failed to install pre hook on_set_mix_item_pre");
        return result;
    }

    result = mods::hook::add_post<SetMixItem>(on_set_mix_item_post, &kDisguiseHookOptions);
    if (result != MOD_OK) {
        mods::log::error("failed to install post hook on_set_mix_item_post");
        return result;
    }

    result = mods::hook::add_pre<IsMixItemOn>(on_is_mix_item_on_pre, &kDisguiseHookOptions);
    if (result != MOD_OK) {
        mods::log::error("failed to install pre hook on_is_mix_item_on_pre");
        return result;
    }

    result = mods::hook::add_post<IsMixItemOn>(on_is_mix_item_on_post, &kDisguiseHookOptions);
    if (result != MOD_OK) {
        mods::log::error("failed to install post hook on_is_mix_item_on_post");
        return result;
    }

    result = mods::hook::add_pre<IsMixItemOff>(on_is_mix_item_off_pre, &kDisguiseHookOptions);
    if (result != MOD_OK) {
        mods::log::error("failed to install pre hook on_is_mix_item_off_pre");
        return result;
    }

    result = mods::hook::add_post<IsMixItemOff>(on_is_mix_item_off_post, &kDisguiseHookOptions);
    if (result != MOD_OK) {
        mods::log::error("failed to install post hook on_is_mix_item_off_post");
        return result;
    }

    result = mods::hook::add_pre<SetActiveCursor>(on_set_active_cursor_pre, &kDisguiseHookOptions);
    if (result != MOD_OK) {
        mods::log::error("failed to install pre hook on_set_active_cursor_pre");
        return result;
    }

    result =
        mods::hook::add_post<SetActiveCursor>(on_set_active_cursor_post, &kDisguiseHookOptions);
    if (result != MOD_OK) {
        mods::log::error("failed to install post hook on_set_active_cursor_post");
        return result;
    }

    result = mods::hook::add_pre<ArrowShooting>(on_arrow_shooting_pre);
    if (result != MOD_OK) {
        mods::log::error("failed to install pre hook on_arrow_shooting_pre");
        return result;
    }

    result = mods::hook::add_post<ArrowExecute>(on_arrow_execute_post);
    if (result != MOD_OK) {
        mods::log::error("failed to install post hook on_arrow_execute_post");
        return result;
    }

    result = mods::hook::add_post<AlinkMakeArrow>(on_alink_make_arrow_post);
    if (result != MOD_OK) {
        mods::log::error("failed to install post hook on_alink_make_arrow_post");
        return result;
    }

    result = mods::hook::add_pre<DrawKanteraMeter>(on_draw_kantera_meter_pre);
    if (result != MOD_OK) {
        mods::log::error("failed to install pre hook on_draw_kantera_meter_pre");
        return result;
    }

    result = mods::hook::add_pre<ChangeArrowType>(on_change_arrow_type_pre);
    if (result != MOD_OK) {
        mods::log::error("failed to install pre hook on_change_arrow_type_pre");
        return result;
    }

    result = mods::hook::add_pre<SetBowReadyAnime>(on_set_bow_ready_anime_pre);
    if (result != MOD_OK) {
        mods::log::error("failed to install pre hook on_set_bow_ready_anime_pre");
        return result;
    }

    mods::log::info("fire_arrows initialized");
    return MOD_OK;
}

MOD_EXPORT ModResult mod_update(ModError*) {
    updateBurningActors();
    updateFireArrows();
    return MOD_OK;
}

MOD_EXPORT ModResult mod_shutdown(ModError*) {
    return MOD_OK;
}
}
