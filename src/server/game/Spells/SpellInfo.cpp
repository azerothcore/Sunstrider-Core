#include "SpellInfo.h"
#include "DBCStores.h"
#include "SpellAuraDefines.h"
#include "Spell.h"
#include "SpellAuraEffects.h"
#include "SpellMgr.h"

uint32 GetTargetFlagMask(SpellTargetObjectTypes objType)
{
    switch (objType)
    {
    case TARGET_OBJECT_TYPE_DEST:
        return TARGET_FLAG_DEST_LOCATION;
    case TARGET_OBJECT_TYPE_UNIT_AND_DEST:
        return TARGET_FLAG_DEST_LOCATION | TARGET_FLAG_UNIT;
    case TARGET_OBJECT_TYPE_CORPSE_ALLY:
        return TARGET_FLAG_CORPSE_ALLY;
    case TARGET_OBJECT_TYPE_CORPSE_ENEMY:
        return TARGET_FLAG_CORPSE_ENEMY;
    case TARGET_OBJECT_TYPE_CORPSE:
        return TARGET_FLAG_CORPSE_ALLY | TARGET_FLAG_CORPSE_ENEMY;
    case TARGET_OBJECT_TYPE_UNIT:
        return TARGET_FLAG_UNIT;
    case TARGET_OBJECT_TYPE_GOBJ:
        return TARGET_FLAG_GAMEOBJECT;
    case TARGET_OBJECT_TYPE_GOBJ_ITEM:
        return TARGET_FLAG_GAMEOBJECT_ITEM;
    case TARGET_OBJECT_TYPE_ITEM:
        return TARGET_FLAG_ITEM;
    case TARGET_OBJECT_TYPE_SRC:
        return TARGET_FLAG_SOURCE_LOCATION;
    default:
        return TARGET_FLAG_NONE;
    }
}

SpellImplicitTargetInfo::SpellImplicitTargetInfo(uint32 target)
{
    _target = Targets(target);
}

bool SpellImplicitTargetInfo::IsArea() const
{
    return GetSelectionCategory() == TARGET_SELECT_CATEGORY_AREA || GetSelectionCategory() == TARGET_SELECT_CATEGORY_CONE;
}

bool SpellImplicitTargetInfo::IsProximityBasedAoe() const
{
    switch (_target)
    {
    case TARGET_UNIT_SRC_AREA_ENTRY:
    case TARGET_UNIT_SRC_AREA_ENEMY:
    case TARGET_UNIT_CASTER_AREA_PARTY:
    case TARGET_UNIT_SRC_AREA_ALLY:
    case TARGET_UNIT_SRC_AREA_PARTY:
#ifdef LICH_KING
    case TARGET_UNIT_LASTTARGET_AREA_PARTY:
#else
    case TARGET_UNIT_AREA_PARTY:
#endif
    case TARGET_GAMEOBJECT_SRC_AREA:
    case TARGET_UNIT_CASTER_AREA_RAID:
    case TARGET_CORPSE_SRC_AREA_ENEMY:
        return true;

    case TARGET_UNIT_DEST_AREA_ENTRY:
    case TARGET_UNIT_DEST_AREA_ENEMY:
    case TARGET_UNIT_DEST_AREA_ALLY:
    case TARGET_UNIT_DEST_AREA_PARTY:
    case TARGET_GAMEOBJECT_DEST_AREA:
    case TARGET_UNIT_TARGET_AREA_RAID_CLASS:
        return false;

    default:
        TC_LOG_WARN("spells", "SpellImplicitTargetInfo::IsProximityBasedAoe called a non-aoe spell");
        return false;
    }
}
SpellTargetSelectionCategories SpellImplicitTargetInfo::GetSelectionCategory() const
{
    return _data[_target].SelectionCategory;
}

SpellTargetReferenceTypes SpellImplicitTargetInfo::GetReferenceType() const
{
    return _data[_target].ReferenceType;
}

SpellTargetObjectTypes SpellImplicitTargetInfo::GetObjectType() const
{
    return _data[_target].ObjectType;
}

SpellTargetCheckTypes SpellImplicitTargetInfo::GetCheckType() const
{
    return _data[_target].SelectionCheckType;
}

SpellTargetDirectionTypes SpellImplicitTargetInfo::GetDirectionType() const
{
    return _data[_target].DirectionType;
}

float SpellImplicitTargetInfo::CalcDirectionAngle() const
{
    switch (GetDirectionType())
    {
        case TARGET_DIR_FRONT:
            return 0.0f;
        case TARGET_DIR_BACK:
            return static_cast<float>(M_PI);
        case TARGET_DIR_RIGHT:
            return static_cast<float>(-M_PI/2);
        case TARGET_DIR_LEFT:
            return static_cast<float>(M_PI/2);
        case TARGET_DIR_FRONT_RIGHT:
            return static_cast<float>(-M_PI/4);
        case TARGET_DIR_BACK_RIGHT:
            return static_cast<float>(-3*M_PI/4);
        case TARGET_DIR_BACK_LEFT:
            return static_cast<float>(3*M_PI/4);
        case TARGET_DIR_FRONT_LEFT:
            return static_cast<float>(M_PI/4);
        case TARGET_DIR_RANDOM:
            return float(rand_norm())*static_cast<float>(2*M_PI);
        default:
            return 0.0f;
    }
}

Targets SpellImplicitTargetInfo::GetTarget() const
{
    return _target;
}

uint32 SpellImplicitTargetInfo::GetExplicitTargetMask(bool& srcSet, bool& dstSet) const
{
    uint32 targetMask = 0;
    if (GetTarget() == TARGET_DEST_TRAJ)
    {
        if (!srcSet)
            targetMask = TARGET_FLAG_SOURCE_LOCATION;
        if (!dstSet)
            targetMask |= TARGET_FLAG_DEST_LOCATION;
    }
    else
    {
        switch (GetReferenceType())
        {
            case TARGET_REFERENCE_TYPE_SRC:
                if (srcSet)
                    break;
                targetMask = TARGET_FLAG_SOURCE_LOCATION;
                break;
            case TARGET_REFERENCE_TYPE_DEST:
                if (dstSet)
                    break;
                targetMask = TARGET_FLAG_DEST_LOCATION;
                break;
            case TARGET_REFERENCE_TYPE_TARGET:
                switch (GetObjectType())
                {
                    case TARGET_OBJECT_TYPE_GOBJ:
                        targetMask = TARGET_FLAG_GAMEOBJECT;
                        break;
                    case TARGET_OBJECT_TYPE_GOBJ_ITEM:
                        targetMask = TARGET_FLAG_GAMEOBJECT_ITEM;
                        break;
                    case TARGET_OBJECT_TYPE_UNIT_AND_DEST:
                    case TARGET_OBJECT_TYPE_UNIT:
                    case TARGET_OBJECT_TYPE_DEST:
                        switch (GetCheckType())
                        {
                            case TARGET_CHECK_ENEMY:
                                targetMask = TARGET_FLAG_UNIT_ENEMY;
                                break;
                            case TARGET_CHECK_ALLY:
                                targetMask = TARGET_FLAG_UNIT_ALLY;
                                break;
                            case TARGET_CHECK_PARTY:
                                targetMask = TARGET_FLAG_UNIT_PARTY;
                                break;
                            case TARGET_CHECK_RAID:
                                targetMask = TARGET_FLAG_UNIT_RAID;
                                break;
#ifdef LICH_KING
                            case TARGET_CHECK_PASSENGER:
                                targetMask = TARGET_FLAG_UNIT_PASSENGER;
                                break;
#endif
                            case TARGET_CHECK_RAID_CLASS:
                                // nobreak;
                            default:
                                targetMask = TARGET_FLAG_UNIT;
                                break;
                        }
                        break;
                    default:
                        break;
                }
                break;
            default:
                break;
        }
    }

    switch (GetObjectType())
    {
        case TARGET_OBJECT_TYPE_SRC:
            srcSet = true;
            break;
        case TARGET_OBJECT_TYPE_DEST:
        case TARGET_OBJECT_TYPE_UNIT_AND_DEST:
            dstSet = true;
            break;
        default:
            break;
    }
    return targetMask;
}

SpellImplicitTargetInfo::StaticData  SpellImplicitTargetInfo::_data[TOTAL_SPELL_TARGETS] =
{
    {TARGET_OBJECT_TYPE_NONE, TARGET_REFERENCE_TYPE_NONE,   TARGET_SELECT_CATEGORY_NYI,     TARGET_CHECK_DEFAULT,  TARGET_DIR_NONE},        //
    {TARGET_OBJECT_TYPE_UNIT, TARGET_REFERENCE_TYPE_CASTER, TARGET_SELECT_CATEGORY_DEFAULT, TARGET_CHECK_DEFAULT,  TARGET_DIR_NONE},        // 1 TARGET_UNIT_CASTER
    {TARGET_OBJECT_TYPE_UNIT, TARGET_REFERENCE_TYPE_CASTER, TARGET_SELECT_CATEGORY_NEARBY,  TARGET_CHECK_ENEMY,    TARGET_DIR_NONE},        // 2 TARGET_UNIT_NEARBY_ENEMY
    {TARGET_OBJECT_TYPE_UNIT, TARGET_REFERENCE_TYPE_CASTER, TARGET_SELECT_CATEGORY_NEARBY,  TARGET_CHECK_PARTY,    TARGET_DIR_NONE},        // 3 TARGET_UNIT_NEARBY_PARTY
    {TARGET_OBJECT_TYPE_UNIT, TARGET_REFERENCE_TYPE_CASTER, TARGET_SELECT_CATEGORY_NEARBY,  TARGET_CHECK_ALLY,     TARGET_DIR_NONE},        // 4 TARGET_UNIT_NEARBY_ALLY
    {TARGET_OBJECT_TYPE_UNIT, TARGET_REFERENCE_TYPE_CASTER, TARGET_SELECT_CATEGORY_DEFAULT, TARGET_CHECK_DEFAULT,  TARGET_DIR_NONE},        // 5 TARGET_UNIT_PET
    {TARGET_OBJECT_TYPE_UNIT, TARGET_REFERENCE_TYPE_TARGET, TARGET_SELECT_CATEGORY_DEFAULT, TARGET_CHECK_ENEMY,    TARGET_DIR_NONE},        // 6 TARGET_UNIT_TARGET_ENEMY
    {TARGET_OBJECT_TYPE_UNIT, TARGET_REFERENCE_TYPE_SRC,    TARGET_SELECT_CATEGORY_AREA,    TARGET_CHECK_ENTRY,    TARGET_DIR_NONE},        // 7 TARGET_UNIT_SRC_AREA_ENTRY
    {TARGET_OBJECT_TYPE_UNIT, TARGET_REFERENCE_TYPE_DEST,   TARGET_SELECT_CATEGORY_AREA,    TARGET_CHECK_ENTRY,    TARGET_DIR_NONE},        // 8 TARGET_UNIT_DEST_AREA_ENTRY
    {TARGET_OBJECT_TYPE_DEST, TARGET_REFERENCE_TYPE_CASTER, TARGET_SELECT_CATEGORY_DEFAULT, TARGET_CHECK_DEFAULT,  TARGET_DIR_NONE},        // 9 TARGET_DEST_HOME
    {TARGET_OBJECT_TYPE_NONE, TARGET_REFERENCE_TYPE_NONE,   TARGET_SELECT_CATEGORY_NYI,     TARGET_CHECK_DEFAULT,  TARGET_DIR_NONE},        // 10
    {TARGET_OBJECT_TYPE_UNIT, TARGET_REFERENCE_TYPE_SRC,    TARGET_SELECT_CATEGORY_NYI,     TARGET_CHECK_DEFAULT,  TARGET_DIR_NONE},        // 11 TARGET_UNIT_SRC_AREA_UNK_11
    {TARGET_OBJECT_TYPE_NONE, TARGET_REFERENCE_TYPE_NONE,   TARGET_SELECT_CATEGORY_NYI,     TARGET_CHECK_DEFAULT,  TARGET_DIR_NONE},        // 12
    {TARGET_OBJECT_TYPE_NONE, TARGET_REFERENCE_TYPE_NONE,   TARGET_SELECT_CATEGORY_NYI,     TARGET_CHECK_DEFAULT,  TARGET_DIR_NONE},        // 13
    {TARGET_OBJECT_TYPE_NONE, TARGET_REFERENCE_TYPE_NONE,   TARGET_SELECT_CATEGORY_NYI,     TARGET_CHECK_DEFAULT,  TARGET_DIR_NONE},        // 14
    {TARGET_OBJECT_TYPE_UNIT, TARGET_REFERENCE_TYPE_SRC,    TARGET_SELECT_CATEGORY_AREA,    TARGET_CHECK_ENEMY,    TARGET_DIR_NONE},        // 15 TARGET_UNIT_SRC_AREA_ENEMY
    {TARGET_OBJECT_TYPE_UNIT, TARGET_REFERENCE_TYPE_DEST,   TARGET_SELECT_CATEGORY_AREA,    TARGET_CHECK_ENEMY,    TARGET_DIR_NONE},        // 16 TARGET_UNIT_DEST_AREA_ENEMY
    {TARGET_OBJECT_TYPE_DEST, TARGET_REFERENCE_TYPE_CASTER, TARGET_SELECT_CATEGORY_DEFAULT, TARGET_CHECK_DEFAULT,  TARGET_DIR_NONE},        // 17 TARGET_DEST_DB
    {TARGET_OBJECT_TYPE_DEST, TARGET_REFERENCE_TYPE_CASTER, TARGET_SELECT_CATEGORY_DEFAULT, TARGET_CHECK_DEFAULT,  TARGET_DIR_NONE},        // 18 TARGET_DEST_CASTER
    {TARGET_OBJECT_TYPE_NONE, TARGET_REFERENCE_TYPE_NONE,   TARGET_SELECT_CATEGORY_NYI,     TARGET_CHECK_DEFAULT,  TARGET_DIR_NONE},        // 19
    {TARGET_OBJECT_TYPE_UNIT, TARGET_REFERENCE_TYPE_CASTER, TARGET_SELECT_CATEGORY_AREA,    TARGET_CHECK_PARTY,    TARGET_DIR_NONE},        // 20 TARGET_UNIT_CASTER_AREA_PARTY
    {TARGET_OBJECT_TYPE_UNIT, TARGET_REFERENCE_TYPE_TARGET, TARGET_SELECT_CATEGORY_DEFAULT, TARGET_CHECK_ALLY,     TARGET_DIR_NONE},        // 21 TARGET_UNIT_TARGET_ALLY
    {TARGET_OBJECT_TYPE_SRC,  TARGET_REFERENCE_TYPE_CASTER, TARGET_SELECT_CATEGORY_DEFAULT, TARGET_CHECK_DEFAULT,  TARGET_DIR_NONE},        // 22 TARGET_SRC_CASTER
    {TARGET_OBJECT_TYPE_GOBJ, TARGET_REFERENCE_TYPE_TARGET, TARGET_SELECT_CATEGORY_DEFAULT, TARGET_CHECK_DEFAULT,  TARGET_DIR_NONE},        // 23 TARGET_GAMEOBJECT_TARGET
    {TARGET_OBJECT_TYPE_UNIT, TARGET_REFERENCE_TYPE_CASTER, TARGET_SELECT_CATEGORY_CONE,    TARGET_CHECK_ENEMY,    TARGET_DIR_FRONT},       // 24 TARGET_UNIT_CONE_ENEMY_24
    {TARGET_OBJECT_TYPE_UNIT, TARGET_REFERENCE_TYPE_TARGET, TARGET_SELECT_CATEGORY_DEFAULT, TARGET_CHECK_DEFAULT,  TARGET_DIR_NONE},        // 25 TARGET_UNIT_TARGET_ANY
    {TARGET_OBJECT_TYPE_GOBJ_ITEM, TARGET_REFERENCE_TYPE_TARGET, TARGET_SELECT_CATEGORY_DEFAULT, TARGET_CHECK_DEFAULT, TARGET_DIR_NONE},    // 26 TARGET_GAMEOBJECT_ITEM_TARGET
    {TARGET_OBJECT_TYPE_UNIT, TARGET_REFERENCE_TYPE_CASTER, TARGET_SELECT_CATEGORY_DEFAULT, TARGET_CHECK_DEFAULT,  TARGET_DIR_NONE},        // 27 TARGET_UNIT_MASTER
    {TARGET_OBJECT_TYPE_DEST, TARGET_REFERENCE_TYPE_DEST,   TARGET_SELECT_CATEGORY_DEFAULT, TARGET_CHECK_ENEMY,    TARGET_DIR_NONE},        // 28 TARGET_DEST_DYNOBJ_ENEMY
    {TARGET_OBJECT_TYPE_DEST, TARGET_REFERENCE_TYPE_DEST,   TARGET_SELECT_CATEGORY_DEFAULT, TARGET_CHECK_ALLY,     TARGET_DIR_NONE},        // 29 TARGET_DEST_DYNOBJ_ALLY
    {TARGET_OBJECT_TYPE_UNIT, TARGET_REFERENCE_TYPE_SRC,    TARGET_SELECT_CATEGORY_AREA,    TARGET_CHECK_ALLY,     TARGET_DIR_NONE},        // 30 TARGET_UNIT_SRC_AREA_ALLY
    {TARGET_OBJECT_TYPE_UNIT, TARGET_REFERENCE_TYPE_DEST,   TARGET_SELECT_CATEGORY_AREA,    TARGET_CHECK_ALLY,     TARGET_DIR_NONE},        // 31 TARGET_UNIT_DEST_AREA_ALLY
    {TARGET_OBJECT_TYPE_DEST, TARGET_REFERENCE_TYPE_CASTER, TARGET_SELECT_CATEGORY_DEFAULT, TARGET_CHECK_DEFAULT,  TARGET_DIR_FRONT_LEFT},  // 32 TARGET_DEST_CASTER_SUMMON
    {TARGET_OBJECT_TYPE_UNIT, TARGET_REFERENCE_TYPE_SRC,    TARGET_SELECT_CATEGORY_AREA,    TARGET_CHECK_PARTY,    TARGET_DIR_NONE},        // 33 TARGET_UNIT_SRC_AREA_PARTY
    {TARGET_OBJECT_TYPE_UNIT, TARGET_REFERENCE_TYPE_DEST,   TARGET_SELECT_CATEGORY_AREA,    TARGET_CHECK_PARTY,    TARGET_DIR_NONE},        // 34 TARGET_UNIT_DEST_AREA_PARTY
    {TARGET_OBJECT_TYPE_UNIT, TARGET_REFERENCE_TYPE_TARGET, TARGET_SELECT_CATEGORY_DEFAULT, TARGET_CHECK_PARTY,    TARGET_DIR_NONE},        // 35 TARGET_UNIT_TARGET_PARTY
    {TARGET_OBJECT_TYPE_DEST, TARGET_REFERENCE_TYPE_CASTER, TARGET_SELECT_CATEGORY_NYI,     TARGET_CHECK_DEFAULT,  TARGET_DIR_NONE},        // 36 TARGET_DEST_CASTER_UNK_36
#ifdef LICH_KING
    {TARGET_OBJECT_TYPE_UNIT, TARGET_REFERENCE_TYPE_LAST,   TARGET_SELECT_CATEGORY_AREA,    TARGET_CHECK_PARTY,    TARGET_DIR_NONE},        // 37 TARGET_UNIT_LASTTARGET_AREA_PARTY
#else
    {TARGET_OBJECT_TYPE_UNIT, TARGET_REFERENCE_TYPE_TARGET, TARGET_SELECT_CATEGORY_AREA,    TARGET_CHECK_PARTY,    TARGET_DIR_NONE },       // 37 TARGET_UNIT_AREA_PARTY
#endif
    {TARGET_OBJECT_TYPE_UNIT, TARGET_REFERENCE_TYPE_CASTER, TARGET_SELECT_CATEGORY_NEARBY,  TARGET_CHECK_ENTRY,    TARGET_DIR_NONE},        // 38 TARGET_UNIT_NEARBY_ENTRY
    {TARGET_OBJECT_TYPE_DEST, TARGET_REFERENCE_TYPE_CASTER, TARGET_SELECT_CATEGORY_DEFAULT, TARGET_CHECK_DEFAULT,  TARGET_DIR_NONE},        // 39 TARGET_DEST_CASTER_FISHING
    {TARGET_OBJECT_TYPE_GOBJ, TARGET_REFERENCE_TYPE_CASTER, TARGET_SELECT_CATEGORY_NEARBY,  TARGET_CHECK_ENTRY,    TARGET_DIR_NONE},        // 40 TARGET_GAMEOBJECT_NEARBY_ENTRY
    {TARGET_OBJECT_TYPE_DEST, TARGET_REFERENCE_TYPE_CASTER, TARGET_SELECT_CATEGORY_DEFAULT, TARGET_CHECK_DEFAULT,  TARGET_DIR_FRONT_RIGHT}, // 41 TARGET_DEST_CASTER_FRONT_RIGHT
    {TARGET_OBJECT_TYPE_DEST, TARGET_REFERENCE_TYPE_CASTER, TARGET_SELECT_CATEGORY_DEFAULT, TARGET_CHECK_DEFAULT,  TARGET_DIR_BACK_RIGHT},  // 42 TARGET_DEST_CASTER_BACK_RIGHT
    {TARGET_OBJECT_TYPE_DEST, TARGET_REFERENCE_TYPE_CASTER, TARGET_SELECT_CATEGORY_DEFAULT, TARGET_CHECK_DEFAULT,  TARGET_DIR_BACK_LEFT},   // 43 TARGET_DEST_CASTER_BACK_LEFT
    {TARGET_OBJECT_TYPE_DEST, TARGET_REFERENCE_TYPE_CASTER, TARGET_SELECT_CATEGORY_DEFAULT, TARGET_CHECK_DEFAULT,  TARGET_DIR_FRONT_LEFT},  // 44 TARGET_DEST_CASTER_FRONT_LEFT
    {TARGET_OBJECT_TYPE_UNIT, TARGET_REFERENCE_TYPE_TARGET, TARGET_SELECT_CATEGORY_DEFAULT, TARGET_CHECK_ALLY,     TARGET_DIR_NONE},        // 45 TARGET_UNIT_TARGET_CHAINHEAL_ALLY
    {TARGET_OBJECT_TYPE_DEST, TARGET_REFERENCE_TYPE_CASTER, TARGET_SELECT_CATEGORY_NEARBY,  TARGET_CHECK_ENTRY,    TARGET_DIR_NONE},        // 46 TARGET_DEST_NEARBY_ENTRY
    {TARGET_OBJECT_TYPE_DEST, TARGET_REFERENCE_TYPE_CASTER, TARGET_SELECT_CATEGORY_DEFAULT, TARGET_CHECK_DEFAULT,  TARGET_DIR_FRONT},       // 47 TARGET_DEST_CASTER_FRONT
    {TARGET_OBJECT_TYPE_DEST, TARGET_REFERENCE_TYPE_CASTER, TARGET_SELECT_CATEGORY_DEFAULT, TARGET_CHECK_DEFAULT,  TARGET_DIR_BACK},        // 48 TARGET_DEST_CASTER_BACK
    {TARGET_OBJECT_TYPE_DEST, TARGET_REFERENCE_TYPE_CASTER, TARGET_SELECT_CATEGORY_DEFAULT, TARGET_CHECK_DEFAULT,  TARGET_DIR_RIGHT},       // 49 TARGET_DEST_CASTER_RIGHT
    {TARGET_OBJECT_TYPE_DEST, TARGET_REFERENCE_TYPE_CASTER, TARGET_SELECT_CATEGORY_DEFAULT, TARGET_CHECK_DEFAULT,  TARGET_DIR_LEFT},        // 50 TARGET_DEST_CASTER_LEFT
    {TARGET_OBJECT_TYPE_GOBJ, TARGET_REFERENCE_TYPE_SRC,    TARGET_SELECT_CATEGORY_AREA,    TARGET_CHECK_DEFAULT,  TARGET_DIR_NONE},        // 51 TARGET_GAMEOBJECT_SRC_AREA
    {TARGET_OBJECT_TYPE_GOBJ, TARGET_REFERENCE_TYPE_DEST,   TARGET_SELECT_CATEGORY_AREA,    TARGET_CHECK_DEFAULT,  TARGET_DIR_NONE},        // 52 TARGET_GAMEOBJECT_DEST_AREA
    {TARGET_OBJECT_TYPE_DEST, TARGET_REFERENCE_TYPE_TARGET, TARGET_SELECT_CATEGORY_DEFAULT, TARGET_CHECK_ENEMY,    TARGET_DIR_NONE},        // 53 TARGET_DEST_TARGET_ENEMY
    {TARGET_OBJECT_TYPE_UNIT, TARGET_REFERENCE_TYPE_CASTER, TARGET_SELECT_CATEGORY_CONE,    TARGET_CHECK_ENEMY,    TARGET_DIR_FRONT},       // 54 TARGET_UNIT_CONE_ENEMY_54
    {TARGET_OBJECT_TYPE_DEST, TARGET_REFERENCE_TYPE_CASTER, TARGET_SELECT_CATEGORY_DEFAULT, TARGET_CHECK_DEFAULT,  TARGET_DIR_NONE},        // 55 TARGET_DEST_CASTER_FRONT_LEAP
    {TARGET_OBJECT_TYPE_UNIT, TARGET_REFERENCE_TYPE_CASTER, TARGET_SELECT_CATEGORY_AREA,    TARGET_CHECK_RAID,     TARGET_DIR_NONE},        // 56 TARGET_UNIT_CASTER_AREA_RAID
    {TARGET_OBJECT_TYPE_UNIT, TARGET_REFERENCE_TYPE_TARGET, TARGET_SELECT_CATEGORY_DEFAULT, TARGET_CHECK_RAID,     TARGET_DIR_NONE},        // 57 TARGET_UNIT_TARGET_RAID
    {TARGET_OBJECT_TYPE_UNIT, TARGET_REFERENCE_TYPE_CASTER, TARGET_SELECT_CATEGORY_NEARBY,  TARGET_CHECK_RAID,     TARGET_DIR_NONE},        // 58 TARGET_UNIT_NEARBY_RAID
    {TARGET_OBJECT_TYPE_UNIT, TARGET_REFERENCE_TYPE_CASTER, TARGET_SELECT_CATEGORY_CONE,    TARGET_CHECK_ALLY,     TARGET_DIR_FRONT},       // 59 TARGET_UNIT_CONE_ALLY
    {TARGET_OBJECT_TYPE_UNIT, TARGET_REFERENCE_TYPE_CASTER, TARGET_SELECT_CATEGORY_CONE,    TARGET_CHECK_ENTRY,    TARGET_DIR_FRONT},       // 60 TARGET_UNIT_CONE_ENTRY
    {TARGET_OBJECT_TYPE_UNIT, TARGET_REFERENCE_TYPE_TARGET, TARGET_SELECT_CATEGORY_AREA,    TARGET_CHECK_RAID_CLASS, TARGET_DIR_NONE},      // 61 TARGET_UNIT_TARGET_AREA_RAID_CLASS
    {TARGET_OBJECT_TYPE_NONE, TARGET_REFERENCE_TYPE_NONE,   TARGET_SELECT_CATEGORY_NYI,     TARGET_CHECK_DEFAULT,  TARGET_DIR_NONE},        // 62 TARGET_UNK_62
    {TARGET_OBJECT_TYPE_DEST, TARGET_REFERENCE_TYPE_TARGET, TARGET_SELECT_CATEGORY_DEFAULT, TARGET_CHECK_DEFAULT,  TARGET_DIR_NONE},        // 63 TARGET_DEST_TARGET_ANY
    {TARGET_OBJECT_TYPE_DEST, TARGET_REFERENCE_TYPE_TARGET, TARGET_SELECT_CATEGORY_DEFAULT, TARGET_CHECK_DEFAULT,  TARGET_DIR_FRONT},       // 64 TARGET_DEST_TARGET_FRONT
    {TARGET_OBJECT_TYPE_DEST, TARGET_REFERENCE_TYPE_TARGET, TARGET_SELECT_CATEGORY_DEFAULT, TARGET_CHECK_DEFAULT,  TARGET_DIR_BACK},        // 65 TARGET_DEST_TARGET_BACK
    {TARGET_OBJECT_TYPE_DEST, TARGET_REFERENCE_TYPE_TARGET, TARGET_SELECT_CATEGORY_DEFAULT, TARGET_CHECK_DEFAULT,  TARGET_DIR_RIGHT},       // 66 TARGET_DEST_TARGET_RIGHT
    {TARGET_OBJECT_TYPE_DEST, TARGET_REFERENCE_TYPE_TARGET, TARGET_SELECT_CATEGORY_DEFAULT, TARGET_CHECK_DEFAULT,  TARGET_DIR_LEFT},        // 67 TARGET_DEST_TARGET_LEFT
    {TARGET_OBJECT_TYPE_DEST, TARGET_REFERENCE_TYPE_TARGET, TARGET_SELECT_CATEGORY_DEFAULT, TARGET_CHECK_DEFAULT,  TARGET_DIR_FRONT_RIGHT}, // 68 TARGET_DEST_TARGET_FRONT_RIGHT
    {TARGET_OBJECT_TYPE_DEST, TARGET_REFERENCE_TYPE_TARGET, TARGET_SELECT_CATEGORY_DEFAULT, TARGET_CHECK_DEFAULT,  TARGET_DIR_BACK_RIGHT},  // 69 TARGET_DEST_TARGET_BACK_RIGHT
    {TARGET_OBJECT_TYPE_DEST, TARGET_REFERENCE_TYPE_TARGET, TARGET_SELECT_CATEGORY_DEFAULT, TARGET_CHECK_DEFAULT,  TARGET_DIR_BACK_LEFT},   // 70 TARGET_DEST_TARGET_BACK_LEFT
    {TARGET_OBJECT_TYPE_DEST, TARGET_REFERENCE_TYPE_TARGET, TARGET_SELECT_CATEGORY_DEFAULT, TARGET_CHECK_DEFAULT,  TARGET_DIR_FRONT_LEFT},  // 71 TARGET_DEST_TARGET_FRONT_LEFT
    {TARGET_OBJECT_TYPE_DEST, TARGET_REFERENCE_TYPE_CASTER, TARGET_SELECT_CATEGORY_DEFAULT, TARGET_CHECK_DEFAULT,  TARGET_DIR_RANDOM},      // 72 TARGET_DEST_CASTER_RANDOM
    {TARGET_OBJECT_TYPE_DEST, TARGET_REFERENCE_TYPE_CASTER, TARGET_SELECT_CATEGORY_DEFAULT, TARGET_CHECK_DEFAULT,  TARGET_DIR_RANDOM},      // 73 TARGET_DEST_CASTER_RADIUS
    {TARGET_OBJECT_TYPE_DEST, TARGET_REFERENCE_TYPE_TARGET, TARGET_SELECT_CATEGORY_DEFAULT, TARGET_CHECK_DEFAULT,  TARGET_DIR_RANDOM},      // 74 TARGET_DEST_TARGET_RANDOM
    {TARGET_OBJECT_TYPE_DEST, TARGET_REFERENCE_TYPE_TARGET, TARGET_SELECT_CATEGORY_DEFAULT, TARGET_CHECK_DEFAULT,  TARGET_DIR_RANDOM},      // 75 TARGET_DEST_TARGET_RADIUS
    {TARGET_OBJECT_TYPE_DEST, TARGET_REFERENCE_TYPE_CASTER, TARGET_SELECT_CATEGORY_CHANNEL, TARGET_CHECK_DEFAULT,  TARGET_DIR_NONE},        // 76 TARGET_DEST_CHANNEL_TARGET
    {TARGET_OBJECT_TYPE_UNIT, TARGET_REFERENCE_TYPE_CASTER, TARGET_SELECT_CATEGORY_CHANNEL, TARGET_CHECK_DEFAULT,  TARGET_DIR_NONE},        // 77 TARGET_UNIT_CHANNEL_TARGET
    {TARGET_OBJECT_TYPE_DEST, TARGET_REFERENCE_TYPE_DEST,   TARGET_SELECT_CATEGORY_DEFAULT, TARGET_CHECK_DEFAULT,  TARGET_DIR_FRONT},       // 78 TARGET_DEST_DEST_FRONT
    {TARGET_OBJECT_TYPE_DEST, TARGET_REFERENCE_TYPE_DEST,   TARGET_SELECT_CATEGORY_DEFAULT, TARGET_CHECK_DEFAULT,  TARGET_DIR_BACK},        // 79 TARGET_DEST_DEST_BACK
    {TARGET_OBJECT_TYPE_DEST, TARGET_REFERENCE_TYPE_DEST,   TARGET_SELECT_CATEGORY_DEFAULT, TARGET_CHECK_DEFAULT,  TARGET_DIR_RIGHT},       // 80 TARGET_DEST_DEST_RIGHT
    {TARGET_OBJECT_TYPE_DEST, TARGET_REFERENCE_TYPE_DEST,   TARGET_SELECT_CATEGORY_DEFAULT, TARGET_CHECK_DEFAULT,  TARGET_DIR_LEFT},        // 81 TARGET_DEST_DEST_LEFT
    {TARGET_OBJECT_TYPE_DEST, TARGET_REFERENCE_TYPE_DEST,   TARGET_SELECT_CATEGORY_DEFAULT, TARGET_CHECK_DEFAULT,  TARGET_DIR_FRONT_RIGHT}, // 82 TARGET_DEST_DEST_FRONT_RIGHT
    {TARGET_OBJECT_TYPE_DEST, TARGET_REFERENCE_TYPE_DEST,   TARGET_SELECT_CATEGORY_DEFAULT, TARGET_CHECK_DEFAULT,  TARGET_DIR_BACK_RIGHT},  // 83 TARGET_DEST_DEST_BACK_RIGHT
    {TARGET_OBJECT_TYPE_DEST, TARGET_REFERENCE_TYPE_DEST,   TARGET_SELECT_CATEGORY_DEFAULT, TARGET_CHECK_DEFAULT,  TARGET_DIR_BACK_LEFT},   // 84 TARGET_DEST_DEST_BACK_LEFT
    {TARGET_OBJECT_TYPE_DEST, TARGET_REFERENCE_TYPE_DEST,   TARGET_SELECT_CATEGORY_DEFAULT, TARGET_CHECK_DEFAULT,  TARGET_DIR_FRONT_LEFT},  // 85 TARGET_DEST_DEST_FRONT_LEFT
    {TARGET_OBJECT_TYPE_DEST, TARGET_REFERENCE_TYPE_DEST,   TARGET_SELECT_CATEGORY_DEFAULT, TARGET_CHECK_DEFAULT,  TARGET_DIR_RANDOM},      // 86 TARGET_DEST_DEST_RANDOM
    {TARGET_OBJECT_TYPE_DEST, TARGET_REFERENCE_TYPE_DEST,   TARGET_SELECT_CATEGORY_DEFAULT, TARGET_CHECK_DEFAULT,  TARGET_DIR_NONE},        // 87 TARGET_DEST_DEST
    {TARGET_OBJECT_TYPE_DEST, TARGET_REFERENCE_TYPE_DEST,   TARGET_SELECT_CATEGORY_DEFAULT, TARGET_CHECK_DEFAULT,  TARGET_DIR_NONE},        // 88 TARGET_DEST_DYNOBJ_NONE
    {TARGET_OBJECT_TYPE_DEST, TARGET_REFERENCE_TYPE_DEST,   TARGET_SELECT_CATEGORY_TRAJ,    TARGET_CHECK_DEFAULT,  TARGET_DIR_NONE},        // 89 TARGET_DEST_TRAJ
    {TARGET_OBJECT_TYPE_UNIT, TARGET_REFERENCE_TYPE_TARGET, TARGET_SELECT_CATEGORY_DEFAULT, TARGET_CHECK_DEFAULT,  TARGET_DIR_NONE},        // 90 TARGET_UNIT_TARGET_MINIPET
    {TARGET_OBJECT_TYPE_DEST, TARGET_REFERENCE_TYPE_DEST,   TARGET_SELECT_CATEGORY_DEFAULT, TARGET_CHECK_DEFAULT,  TARGET_DIR_RANDOM},      // 91 TARGET_DEST_DEST_RADIUS
    {TARGET_OBJECT_TYPE_UNIT, TARGET_REFERENCE_TYPE_CASTER, TARGET_SELECT_CATEGORY_DEFAULT, TARGET_CHECK_DEFAULT,  TARGET_DIR_NONE},        // 92 TARGET_UNIT_SUMMONER
    {TARGET_OBJECT_TYPE_CORPSE, TARGET_REFERENCE_TYPE_SRC,   TARGET_SELECT_CATEGORY_NYI,     TARGET_CHECK_ENEMY,    TARGET_DIR_NONE},       // 93 TARGET_CORPSE_SRC_AREA_ENEMY
#ifdef LICH_KING
    {TARGET_OBJECT_TYPE_UNIT, TARGET_REFERENCE_TYPE_CASTER, TARGET_SELECT_CATEGORY_DEFAULT, TARGET_CHECK_DEFAULT,  TARGET_DIR_NONE},        // 94 TARGET_UNIT_VEHICLE
    {TARGET_OBJECT_TYPE_UNIT, TARGET_REFERENCE_TYPE_TARGET, TARGET_SELECT_CATEGORY_DEFAULT, TARGET_CHECK_PASSENGER, TARGET_DIR_NONE},       // 95 TARGET_UNIT_TARGET_PASSENGER
    {TARGET_OBJECT_TYPE_UNIT, TARGET_REFERENCE_TYPE_CASTER, TARGET_SELECT_CATEGORY_DEFAULT, TARGET_CHECK_DEFAULT,  TARGET_DIR_NONE},        // 96 TARGET_UNIT_PASSENGER_0
    {TARGET_OBJECT_TYPE_UNIT, TARGET_REFERENCE_TYPE_CASTER, TARGET_SELECT_CATEGORY_DEFAULT, TARGET_CHECK_DEFAULT,  TARGET_DIR_NONE},        // 97 TARGET_UNIT_PASSENGER_1
    {TARGET_OBJECT_TYPE_UNIT, TARGET_REFERENCE_TYPE_CASTER, TARGET_SELECT_CATEGORY_DEFAULT, TARGET_CHECK_DEFAULT,  TARGET_DIR_NONE},        // 98 TARGET_UNIT_PASSENGER_2
    {TARGET_OBJECT_TYPE_UNIT, TARGET_REFERENCE_TYPE_CASTER, TARGET_SELECT_CATEGORY_DEFAULT, TARGET_CHECK_DEFAULT,  TARGET_DIR_NONE},        // 99 TARGET_UNIT_PASSENGER_3
    {TARGET_OBJECT_TYPE_UNIT, TARGET_REFERENCE_TYPE_CASTER, TARGET_SELECT_CATEGORY_DEFAULT, TARGET_CHECK_DEFAULT,  TARGET_DIR_NONE},        // 100 TARGET_UNIT_PASSENGER_4
    {TARGET_OBJECT_TYPE_UNIT, TARGET_REFERENCE_TYPE_CASTER, TARGET_SELECT_CATEGORY_DEFAULT, TARGET_CHECK_DEFAULT,  TARGET_DIR_NONE},        // 101 TARGET_UNIT_PASSENGER_5
    {TARGET_OBJECT_TYPE_UNIT, TARGET_REFERENCE_TYPE_CASTER, TARGET_SELECT_CATEGORY_DEFAULT, TARGET_CHECK_DEFAULT,  TARGET_DIR_NONE},        // 102 TARGET_UNIT_PASSENGER_6
    {TARGET_OBJECT_TYPE_UNIT, TARGET_REFERENCE_TYPE_CASTER, TARGET_SELECT_CATEGORY_DEFAULT, TARGET_CHECK_DEFAULT,  TARGET_DIR_NONE},        // 103 TARGET_UNIT_PASSENGER_7
    {TARGET_OBJECT_TYPE_UNIT, TARGET_REFERENCE_TYPE_CASTER, TARGET_SELECT_CATEGORY_CONE,    TARGET_CHECK_ENEMY,    TARGET_DIR_FRONT},       // 104 TARGET_UNIT_CONE_ENEMY_104
    {TARGET_OBJECT_TYPE_UNIT, TARGET_REFERENCE_TYPE_NONE,   TARGET_SELECT_CATEGORY_NYI,     TARGET_CHECK_DEFAULT,  TARGET_DIR_NONE},        // 105 TARGET_UNIT_UNK_105
    {TARGET_OBJECT_TYPE_DEST, TARGET_REFERENCE_TYPE_CASTER, TARGET_SELECT_CATEGORY_CHANNEL, TARGET_CHECK_DEFAULT,  TARGET_DIR_NONE},        // 106 TARGET_DEST_CHANNEL_CASTER
    {TARGET_OBJECT_TYPE_NONE, TARGET_REFERENCE_TYPE_DEST,   TARGET_SELECT_CATEGORY_NYI,     TARGET_CHECK_DEFAULT,  TARGET_DIR_NONE},        // 107 TARGET_UNK_DEST_AREA_UNK_107
    {TARGET_OBJECT_TYPE_GOBJ, TARGET_REFERENCE_TYPE_CASTER, TARGET_SELECT_CATEGORY_CONE,    TARGET_CHECK_DEFAULT,  TARGET_DIR_FRONT},       // 108 TARGET_GAMEOBJECT_CONE
    {TARGET_OBJECT_TYPE_NONE, TARGET_REFERENCE_TYPE_NONE,   TARGET_SELECT_CATEGORY_NYI,     TARGET_CHECK_DEFAULT,  TARGET_DIR_NONE},        // 109
    {TARGET_OBJECT_TYPE_DEST, TARGET_REFERENCE_TYPE_NONE,   TARGET_SELECT_CATEGORY_NYI,     TARGET_CHECK_DEFAULT,  TARGET_DIR_NONE},        // 110 TARGET_DEST_UNK_110
#endif
};

SpellEffectInfo::SpellEffectInfo(SpellEntry const* spellEntry, SpellInfo const* spellInfo, uint8 effIndex)
{
    _spellInfo = spellInfo;
    _effIndex = effIndex;
    Effect = spellEntry->Effect[effIndex];
    ApplyAuraName = spellEntry->EffectApplyAuraName[effIndex];
    Amplitude = spellEntry->EffectAmplitude[effIndex];
    DieSides = spellEntry->EffectDieSides[effIndex];
#ifndef LICH_KING
    BaseDice = spellEntry->EffectBaseDice[effIndex];
    DicePerLevel = spellEntry->EffectDicePerLevel[effIndex];
#endif
    RealPointsPerLevel = spellEntry->EffectRealPointsPerLevel[effIndex];
    BasePoints = spellEntry->EffectBasePoints[effIndex];
    PointsPerComboPoint = spellEntry->EffectPointsPerComboPoint[effIndex];
    ValueMultiplier = spellEntry->EffectValueMultiplier[effIndex];
    DamageMultiplier = spellEntry->EffectDamageMultiplier[effIndex];
#ifdef LICH_KING
    BonusMultiplier = spellEntry->Effects[effIndex].BonusMultiplier;
#endif
    MiscValue = spellEntry->EffectMiscValue[effIndex];
    MiscValueB = spellEntry->EffectMiscValueB[effIndex];
    Mechanic = Mechanics(spellEntry->EffectMechanic[effIndex]);
    TargetA = SpellImplicitTargetInfo(spellEntry->EffectImplicitTargetA[effIndex]);
    TargetB = SpellImplicitTargetInfo(spellEntry->EffectImplicitTargetB[effIndex]);
    RadiusEntry = spellEntry->EffectRadiusIndex[effIndex] ? sSpellRadiusStore.LookupEntry(spellEntry->EffectRadiusIndex[effIndex]) : nullptr;
    ChainTarget = spellEntry->EffectChainTarget[effIndex];
    ItemType = spellEntry->EffectItemType[effIndex];
    TriggerSpell = spellEntry->EffectTriggerSpell[effIndex];
#ifdef LICH_KING
    SpellClassMask = spellEntry->EffectSpellClassMask[effIndex];
#else
    SpellClassMask = 0; //will be loaded with spell_affect table
#endif
    ImplicitTargetConditions = nullptr;
}

bool SpellEffectInfo::IsEffect() const
{
    return Effect != 0;
}

bool SpellEffectInfo::IsEffect(SpellEffects effectName) const
{
    return Effect == uint32(effectName);
}

bool SpellEffectInfo::IsAura() const
{
    return (IsUnitOwnedAuraEffect() || Effect == SPELL_EFFECT_PERSISTENT_AREA_AURA) && ApplyAuraName != 0;
}

bool SpellEffectInfo::IsAura(AuraType aura) const
{
    return IsAura() && ApplyAuraName == uint32(aura);
}

bool SpellEffectInfo::IsTargetingArea() const
{
    return TargetA.IsArea() || TargetB.IsArea();
}

bool SpellEffectInfo::IsAreaAuraEffect() const
{
    if (Effect == SPELL_EFFECT_APPLY_AREA_AURA_PARTY ||
#ifdef LICH_KING
        Effect == SPELL_EFFECT_APPLY_AREA_AURA_RAID ||
#endif
        Effect == SPELL_EFFECT_APPLY_AREA_AURA_FRIEND ||
        Effect == SPELL_EFFECT_APPLY_AREA_AURA_ENEMY ||
        Effect == SPELL_EFFECT_APPLY_AREA_AURA_PET ||
        Effect == SPELL_EFFECT_APPLY_AREA_AURA_OWNER)
        return true;

    return false;
}

bool SpellEffectInfo::IsFarUnitTargetEffect() const
{
    return (Effect == SPELL_EFFECT_SUMMON_PLAYER)
#ifdef LICH_KING
        || (Effect == SPELL_EFFECT_SUMMON_RAF_FRIEND)
#endif
        || (Effect == SPELL_EFFECT_RESURRECT)
        || (Effect == SPELL_EFFECT_RESURRECT_NEW)
        || (Effect == SPELL_EFFECT_SKIN_PLAYER_CORPSE);
}

bool SpellEffectInfo::IsFarDestTargetEffect() const
{
    return Effect == SPELL_EFFECT_TELEPORT_UNITS;
}

bool SpellEffectInfo::IsUnitOwnedAuraEffect() const
{
    return IsAreaAuraEffect() || Effect == SPELL_EFFECT_APPLY_AURA;
}

SpellSpecificType SpellInfo::GetSpellSpecific() const
{
    switch(SpellFamilyName)
    {
        case SPELLFAMILY_GENERIC:
        {
            //food/drink
            if (AuraInterruptFlags & AURA_INTERRUPT_FLAG_NOT_SEATED)
            {
                bool food = false;
                bool drink = false;
                for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
                {
                    if (!Effects[i].IsAura())
                        continue;
                    switch (Effects[i].ApplyAuraName)
                    {
                        // Food
                    case SPELL_AURA_MOD_REGEN:
                    case SPELL_AURA_OBS_MOD_HEALTH:
                        food = true;
                        break;
                        // Drink
                    case SPELL_AURA_MOD_POWER_REGEN:
                    case SPELL_AURA_OBS_MOD_POWER:
                        drink = true;
                        break;
                    default:
                        break;
                    }
                }

                if (food && drink)
                    return SPELL_SPECIFIC_FOOD_AND_DRINK;
                else if (food)
                    return SPELL_SPECIFIC_FOOD;
                else if (drink)
                    return SPELL_SPECIFIC_DRINK;
            }
            else {
                // this may be a hack
                if ((HasAttribute(SPELL_ATTR2_FOOD_BUFF))
                    && !Category)
                    return SPELL_SPECIFIC_WELL_FED;

                SpellInfo const* firstRankSpellInfo = GetFirstRankSpell();
                switch (firstRankSpellInfo->Id)
                {
                case 8118: // Strength
                case 8099: // Stamina
                case 8112: // Spirit
                case 8096: // Intellect
                case 8115: // Agility
                case 8091: // Armor
                    return SPELL_SPECIFIC_SCROLL;
                default:
                    break;
                }

                switch (Id)
                {
                case 12880: // warrior's Enrage rank 1
                case 14201: //           Enrage rank 2
                case 14202: //           Enrage rank 3
                case 14203: //           Enrage rank 4
                case 14204: //           Enrage rank 5
                case 12292: //             Death Wish
                    return SPELL_SPECIFIC_WARRIOR_ENRAGE;
                    break;
                default: break;
                }
            }
            break;
        }
        case SPELLFAMILY_MAGE:
        {
            // family flags 18(Molten), 25(Frost/Ice), 28(Mage)
            if (SpellFamilyFlags & 0x12040000)
                return SPELL_SPECIFIC_MAGE_ARMOR;

            // Arcane brillance and Arcane intelect (normal check fails because of flags difference)
            if (SpellFamilyFlags & 0x400)
                return SPELL_SPECIFIC_MAGE_ARCANE_BRILLANCE;

            if ((SpellFamilyFlags & 0x1000000) && Effects[0].ApplyAuraName==SPELL_AURA_MOD_CONFUSE)
                return SPELL_SPECIFIC_MAGE_POLYMORPH;

            break;
        }
        case SPELLFAMILY_WARRIOR:
        {
            if (SpellFamilyFlags & 0x00008000010000LL)
                return SPELL_SPECIFIC_POSITIVE_SHOUT;
            break;
        }
        case SPELLFAMILY_WARLOCK:
        {
            // only warlock curses have this
            if (Dispel == DISPEL_CURSE)
                return SPELL_SPECIFIC_CURSE;

            // family flag 37 (only part spells have family name)
            if (SpellFamilyFlags & 0x2000000000LL)
                return SPELL_SPECIFIC_WARLOCK_ARMOR;

            //seed of corruption and corruption
            if (SpellFamilyFlags & 0x1000000002LL)
                return SPELL_SPECIFIC_WARLOCK_CORRUPTION;
            break;
        }  
        case SPELLFAMILY_PRIEST:
        {
            // Divine Spirit and Prayer of Spirit
            if (SpellFamilyFlags & 0x20)
                return SPELL_SPECIFIC_PRIEST_DIVINE_SPIRIT;

            break;
        }
        case SPELLFAMILY_HUNTER:
        {
            // only hunter stings have this
            if (Dispel == DISPEL_POISON)
                return SPELL_SPECIFIC_STING;

            break;
        }
        case SPELLFAMILY_PALADIN:
        {
            //Collection of all the seal family flags. No other paladin spell has any of those.
            if( SpellFamilyFlags & 0x4000A000200LL )
                return SPELL_SPECIFIC_SEAL;

            if (SpellFamilyFlags & 0x10000100LL)
                return SPELL_SPECIFIC_BLESSING;

            if ((SpellFamilyFlags & 0x00000820180400LL) && (HasAttribute(SPELL_ATTR3_TRIGGERED_CAN_TRIGGER_PROC_2)))
                return SPELL_SPECIFIC_JUDGEMENT;

            for (const auto & Effect : Effects)
            {
                // only paladin auras have this
                if (Effect.Effect == SPELL_EFFECT_APPLY_AREA_AURA_PARTY)
                    return SPELL_SPECIFIC_AURA;
            }
            break;
        }
        case SPELLFAMILY_SHAMAN:
        {
            if (SpellFamilyFlags & 0x02000000400LL || Id == 23552) //Lightning Shield & Water Shield (Earth shield is handled by spell_group)
                return SPELL_SPECIFIC_ELEMENTAL_SHIELD;

            break;
        }
        case SPELLFAMILY_DRUID:
        {
            // Mangle
            if (SpellFamilyFlags & 0x0000044000000000LL)
                return SPELL_SPECIFIC_DRUID_MANGLE;
        
            break;
        }
        case SPELLFAMILY_POTION:
            return GetSpellElixirSpecific();
    }

    // only warlock armor/skin have this (in additional to family cases)
    if (HasVisual(130) && SpellIconID == 89)
    {
        return SPELL_SPECIFIC_WARLOCK_ARMOR;
    }

    // only hunter aspects have this (but not all aspects in hunter family)
    if( ActiveIconID == 122 && (GetSchoolMask() & SPELL_SCHOOL_MASK_NATURE) &&
        (Attributes & 0x50000) != 0 && (Attributes & 0x9000010) == 0)
    {
        return SPELL_SPECIFIC_ASPECT;
    }

    for(const auto & Effect : Effects)
    {
        if(Effect.Effect == SPELL_EFFECT_APPLY_AURA)
        {
            switch(Effect.ApplyAuraName)
            {
                case SPELL_AURA_TRACK_CREATURES:
                case SPELL_AURA_TRACK_RESOURCES:
                case SPELL_AURA_TRACK_STEALTHED:
                    return SPELL_SPECIFIC_TRACKER;
                case SPELL_AURA_MOD_CHARM:
                case SPELL_AURA_MOD_POSSESS_PET:
                case SPELL_AURA_MOD_POSSESS:
                    return SPELL_SPECIFIC_CHARM;
            }
        }
    }

    // elixirs can have different families, but potion most ofc.
    if(SpellSpecificType sp = GetSpellElixirSpecific())
        return sp;

    return SPELL_SPECIFIC_NORMAL;
}

SpellSpecificType SpellInfo::GetSpellElixirSpecific() const
{
    uint32 mask = sSpellMgr->GetSpellElixirMask(Id);
    if (mask & ELIXIR_SHATTRATH_MASK)
        return SPELL_SPECIFIC_FLASK_ELIXIR;
    else if((mask & ELIXIR_FLASK_MASK)==ELIXIR_FLASK_MASK)
        return SPELL_SPECIFIC_FLASK_ELIXIR;
    else if(mask & ELIXIR_BATTLE_MASK)
        return SPELL_SPECIFIC_BATTLE_ELIXIR;
    else if(mask & ELIXIR_GUARDIAN_MASK)
        return SPELL_SPECIFIC_GUARDIAN_ELIXIR;
    else
        return SPELL_SPECIFIC_NORMAL;
}

SpellInfo::SpellInfo(SpellEntry const* spellEntry)
{
    Id = spellEntry->Id;
    Category = spellEntry->Category ? sSpellCategoryStore.LookupEntry(spellEntry->Category) : nullptr;
    Dispel = spellEntry->Dispel;
    Mechanic = spellEntry->Mechanic;
    Attributes = spellEntry->Attributes;
    AttributesEx = spellEntry->AttributesEx;
    AttributesEx2 = spellEntry->AttributesEx2;
    AttributesEx3 = spellEntry->AttributesEx3;
    AttributesEx4 = spellEntry->AttributesEx4;
    AttributesEx5 = spellEntry->AttributesEx5;
    AttributesEx6 = spellEntry->AttributesEx6;
#ifdef LICH_KING
    AttributesEx7 = spellEntry->AttributesEx7;
#else
    AttributesEx7 = 0;
#endif
    AttributesCu = 0;
    Stances = spellEntry->Stances;
    StancesNot = spellEntry->StancesNot;
    Targets = spellEntry->Targets;
    TargetCreatureType = spellEntry->TargetCreatureType;
    RequiresSpellFocus = spellEntry->RequiresSpellFocus;
    FacingCasterFlags = spellEntry->FacingCasterFlags;
    CasterAuraState = spellEntry->CasterAuraState;
    TargetAuraState = spellEntry->TargetAuraState;
    CasterAuraStateNot = spellEntry->CasterAuraStateNot;
    TargetAuraStateNot = spellEntry->TargetAuraStateNot;
#ifdef LICH_KING
    CasterAuraSpell = spellEntry->casterAuraSpell;
    TargetAuraSpell = spellEntry->targetAuraSpell;
    ExcludeCasterAuraSpell = spellEntry->excludeCasterAuraSpell;
    ExcludeTargetAuraSpell = spellEntry->excludeTargetAuraSpell;
#else
#endif
    CastTimeEntry = spellEntry->CastingTimeIndex ? sSpellCastTimesStore.LookupEntry(spellEntry->CastingTimeIndex) : nullptr;
    RecoveryTime = spellEntry->RecoveryTime;
    CategoryRecoveryTime = spellEntry->CategoryRecoveryTime;
    StartRecoveryCategory = spellEntry->StartRecoveryCategory;
    StartRecoveryTime = spellEntry->StartRecoveryTime;
    InterruptFlags = spellEntry->InterruptFlags;
    AuraInterruptFlags = spellEntry->AuraInterruptFlags;
    ChannelInterruptFlags = spellEntry->ChannelInterruptFlags;
    ProcFlags = spellEntry->ProcFlags;
    ProcChance = spellEntry->procChance;
    ProcCharges = spellEntry->procCharges;
    MaxLevel = spellEntry->MaxLevel;
    BaseLevel = spellEntry->BaseLevel;
    SpellLevel = spellEntry->SpellLevel;
    DurationEntry = spellEntry->DurationIndex ? sSpellDurationStore.LookupEntry(spellEntry->DurationIndex) : nullptr;
    PowerType = spellEntry->PowerType;
    ManaCost = spellEntry->ManaCost;
    ManaCostPerlevel = spellEntry->ManaCostPerlevel;
    ManaPerSecond = spellEntry->manaPerSecond;
    ManaPerSecondPerLevel = spellEntry->ManaPerSecondPerLevel;
    ManaCostPercentage = spellEntry->ManaCostPercentage;
#ifdef LICH_KING
    RuneCostID = spellEntry->runeCostID;
#endif
    RangeEntry = spellEntry->rangeIndex ? sSpellRangeStore.LookupEntry(spellEntry->rangeIndex) : nullptr;
    Speed = spellEntry->speed;
    StackAmount = spellEntry->StackAmount;
    for (uint8 i = 0; i < 2; ++i)
        Totem[i] = spellEntry->Totem[i];

    for (uint8 i = 0; i < MAX_SPELL_REAGENTS; ++i)
    {
        Reagent[i] = spellEntry->Reagent[i];
        ReagentCount[i] = spellEntry->ReagentCount[i];
    }

    EquippedItemClass = spellEntry->EquippedItemClass;
    EquippedItemSubClassMask = spellEntry->EquippedItemSubClassMask;
    EquippedItemInventoryTypeMask = spellEntry->EquippedItemInventoryTypeMask;
    for (uint8 i = 0; i < 2; ++i)
        TotemCategory[i] = spellEntry->TotemCategory[i];

    for (uint8 i = 0; i < 2; ++i)
#ifdef LICH_KING
        SpellVisual[i] = spellEntry->SpellVisual[i];
#else
        SpellVisual = spellEntry->SpellVisual;
#endif

    SpellIconID = spellEntry->SpellIconID;
    ActiveIconID = spellEntry->activeIconID;
    Priority = spellEntry->spellPriority;
    for (uint8 i = 0; i < 16; ++i)
        SpellName[i] = spellEntry->SpellName[i];

    for (uint8 i = 0; i < 16; ++i)
        Rank[i] = spellEntry->Rank[i];

    MaxTargetLevel = spellEntry->MaxTargetLevel;
    MaxAffectedTargets = spellEntry->MaxAffectedTargets;
    SpellFamilyName = spellEntry->SpellFamilyName;
    SpellFamilyFlags = spellEntry->SpellFamilyFlags;
    DmgClass = spellEntry->DmgClass;
    PreventionType = spellEntry->PreventionType;
#ifdef LICH_KING
    AreaGroupId = spellEntry->AreaGroupId;
#else
    AreaId = spellEntry->AreaId;
#endif
    SchoolMask = spellEntry->SchoolMask;
    for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
        Effects[i] = SpellEffectInfo(spellEntry, this, i);

    //custom attributes from database. More custom attributes to be loaded 
    AttributesCu = spellEntry->CustomAttributesFlags;
    ChainEntry = nullptr;

    ExplicitTargetMask = _GetExplicitTargetMask();

    _allowedMechanicMask = 0;
}


bool SpellInfo::IsRanked() const
{
    return ChainEntry != nullptr;
}

uint8 SpellInfo::GetRank() const
{
    if (!ChainEntry)
        return 1;
    return ChainEntry->rank;
}

SpellInfo const* SpellInfo::GetFirstRankSpell() const
{
    if (!ChainEntry)
        return this;
    return ChainEntry->first;
}

SpellInfo const* SpellInfo::GetLastRankSpell() const
{
    if (!ChainEntry)
        return nullptr;
    return ChainEntry->last;
}

SpellInfo const* SpellInfo::GetNextRankSpell() const
{
    if (!ChainEntry)
        return nullptr;
    return ChainEntry->next;
}

SpellInfo const* SpellInfo::GetPrevRankSpell() const
{
    if (!ChainEntry)
        return nullptr;
    return ChainEntry->prev;
}

SpellInfo const* SpellInfo::GetAuraRankForLevel(uint8 level) const
{
    // ignore passive spells
    if (IsPassive())
        return this;

    bool needRankSelection = false;
    for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
    {
        if (IsPositiveEffect(i) &&
            (   Effects[i].Effect == SPELL_EFFECT_APPLY_AURA
             || Effects[i].Effect == SPELL_EFFECT_APPLY_AREA_AURA_PARTY 
#ifdef LICH_KING
             || Effects[i].Effect == SPELL_EFFECT_APPLY_AREA_AURA_RAID))
#endif
            )
           )
        {
            needRankSelection = true;
            break;
        }
    }

    // not required
    if (!needRankSelection)
        return this;

    for (SpellInfo const* nextSpellInfo = this; nextSpellInfo != nullptr; nextSpellInfo = nextSpellInfo->GetPrevRankSpell())
    {
        // if found appropriate level
        if (uint32(level + 10) >= nextSpellInfo->SpellLevel)
            return nextSpellInfo;

        // one rank less then
    }

    // not found
    return nullptr;
}

bool SpellInfo::IsRankOf(SpellInfo const* spellInfo) const
{
    return GetFirstRankSpell() == spellInfo->GetFirstRankSpell();
}

bool SpellInfo::IsDifferentRankOf(SpellInfo const* spellInfo) const
{
    if (Id == spellInfo->Id)
        return false;
    return IsRankOf(spellInfo);
}

bool SpellInfo::IsHighRankOf(SpellInfo const* spellInfo) const
{
    if (ChainEntry && spellInfo->ChainEntry)
    {
        if (ChainEntry->first == spellInfo->ChainEntry->first)
            if (ChainEntry->rank > spellInfo->ChainEntry->rank)
                return true;
    }
    return false;
}

void SpellInfo::_InitializeExplicitTargetMask()
{
    bool srcSet = false;
    bool dstSet = false;
    uint32 targetMask = Targets;
    // prepare target mask using effect target entries
    for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
    {
        if (!Effects[i].IsEffect())
            continue;

        targetMask |= Effects[i].TargetA.GetExplicitTargetMask(srcSet, dstSet);
        targetMask |= Effects[i].TargetB.GetExplicitTargetMask(srcSet, dstSet);

        // add explicit target flags based on spell effects which have EFFECT_IMPLICIT_TARGET_EXPLICIT and no valid target provided
        if (Effects[i].GetImplicitTargetType() != EFFECT_IMPLICIT_TARGET_EXPLICIT)
            continue;

        // extend explicit target mask only if valid targets for effect could not be provided by target types
        uint32 effectTargetMask = Effects[i].GetMissingTargetMask(srcSet, dstSet, targetMask);

        // don't add explicit object/dest flags when spell has no max range
        if (GetMaxRange(true) == 0.0f && GetMaxRange(false) == 0.0f)
            effectTargetMask &= ~(TARGET_FLAG_UNIT_MASK | TARGET_FLAG_GAMEOBJECT | TARGET_FLAG_CORPSE_MASK | TARGET_FLAG_DEST_LOCATION);

        targetMask |= effectTargetMask;
    }

    ExplicitTargetMask = targetMask;
}

void SpellInfo::_LoadSpellDiminishInfo()
{
    auto diminishingGroupCompute = [this](bool triggered) -> DiminishingGroup
    {
        // Explicit Diminishing Groups
        switch (SpellFamilyName)
        {
        case SPELLFAMILY_GENERIC:
        {
            switch (Id) {
            case 30529: // Recently In Game - Karazhan Chess Event
            case 44799: // Frost Breath (Kalecgos)
            case 46562: // Mind Flay
            case 6945: // Chest Pains
                return DIMINISHING_NONE;
            case 12494: // Frostbite
                return DIMINISHING_TRIGGER_ROOT;
            }
        }
        case SPELLFAMILY_MAGE:
        {
            // Polymorph
            if ((SpellFamilyFlags & 0x00001000000LL) && Effects[0].ApplyAuraName == SPELL_AURA_MOD_CONFUSE)
                return DIMINISHING_POLYMORPH;
            if (Id == 33395) // Elemental's freeze
                return DIMINISHING_CONTROLLED_ROOT;
            break;
        }
        case SPELLFAMILY_ROGUE:
        {
            // Kidney Shot
            if (SpellFamilyFlags & 0x00000200000LL)
                return DIMINISHING_KIDNEYSHOT;
            // Sap
            else if (SpellFamilyFlags & 0x00000000080LL)
                return DIMINISHING_POLYMORPH;
            // Gouge
            else if (SpellFamilyFlags & 0x00000000008LL)
                return DIMINISHING_POLYMORPH;
            // Blind
            else if (SpellFamilyFlags & 0x00001000000LL)
                return DIMINISHING_BLIND_CYCLONE;
            break;
        }
        case SPELLFAMILY_HUNTER:
        {
            // Freezing trap
            if (SpellFamilyFlags & 0x00000000008LL)
                return DIMINISHING_FREEZE;
            break;
        }
        case SPELLFAMILY_WARLOCK:
        {
            // Death Coil
            if (SpellFamilyFlags & 0x00000080000LL)
                return DIMINISHING_DEATHCOIL;
            // Seduction
            if (SpellFamilyFlags & 0x00040000000LL)
                return DIMINISHING_FEAR;
            // Fear
            //else if (SpellFamilyFlags & 0x40840000000LL)
            //    return DIMINISHING_WARLOCK_FEAR;
            // Curses/etc
            if (SpellVisual == 339 && SpellIconID == 692) // Curse of Languages
                return DIMINISHING_LIMITONLY;
            else if (SpellFamilyFlags & 0x00080000000LL) {
                if (SpellVisual == 1265 && SpellIconID == 93)   // Curse of Recklessness
                    return DIMINISHING_NONE;
                else
                    return DIMINISHING_LIMITONLY;
            }
            break;
        }
        case SPELLFAMILY_DRUID:
        {
            // Cyclone
            if (SpellFamilyFlags & 0x02000000000LL)
                return DIMINISHING_BLIND_CYCLONE;
            // Nature's Grasp trigger
            if (SpellFamilyFlags & 0x00000000200LL && Attributes == 0x49010000)
                return DIMINISHING_CONTROLLED_ROOT;
            break;
        }
        case SPELLFAMILY_WARRIOR:
        {
            // Hamstring - limit duration to 10s in PvP
            if (SpellFamilyFlags & 0x00000000002LL)
                return DIMINISHING_LIMITONLY;
            break;
        }
        default:
        {
            break;
        }
        }

        // Get by mechanic
        for (const auto & Effect : Effects)
        {
            if (Mechanic == MECHANIC_STUN || Effect.Mechanic == MECHANIC_STUN)
                return triggered ? DIMINISHING_TRIGGER_STUN : DIMINISHING_CONTROLLED_STUN;
            else if (Mechanic == MECHANIC_SLEEP || Effect.Mechanic == MECHANIC_SLEEP)
                return DIMINISHING_SLEEP;
            else if (Mechanic == MECHANIC_ROOT || Effect.Mechanic == MECHANIC_ROOT)
                return triggered ? DIMINISHING_TRIGGER_ROOT : DIMINISHING_CONTROLLED_ROOT;
            else if (Mechanic == MECHANIC_FEAR || Effect.Mechanic == MECHANIC_FEAR)
                return DIMINISHING_FEAR;
            else if (Mechanic == MECHANIC_CHARM || Effect.Mechanic == MECHANIC_CHARM)
                return DIMINISHING_CHARM;
            /*   else if (Mechanic == MECHANIC_SILENCE || Effects[i].Mechanic == MECHANIC_SILENCE)
            return DIMINISHING_SILENCE; */
            else if (Mechanic == MECHANIC_DISARM || Effect.Mechanic == MECHANIC_DISARM)
                return DIMINISHING_DISARM;
            else if (Mechanic == MECHANIC_FREEZE || Effect.Mechanic == MECHANIC_FREEZE)
                return DIMINISHING_FREEZE;
            else if (Mechanic == MECHANIC_KNOCKOUT || Effect.Mechanic == MECHANIC_KNOCKOUT ||
                Mechanic == MECHANIC_SAPPED || Effect.Mechanic == MECHANIC_SAPPED)
                return DIMINISHING_KNOCKOUT;
            else if (Mechanic == MECHANIC_BANISH || Effect.Mechanic == MECHANIC_BANISH)
                return DIMINISHING_BANISH;
        }

        return DIMINISHING_NONE;
    };

    auto diminishingTypeCompute = [](DiminishingGroup group) -> DiminishingReturnsType
    {
        switch (group)
        {
        case DIMINISHING_BLIND_CYCLONE:
        case DIMINISHING_CONTROLLED_STUN:
        case DIMINISHING_TRIGGER_STUN:
        case DIMINISHING_KIDNEYSHOT:
            return DRTYPE_ALL;
        case DIMINISHING_SLEEP:
        case DIMINISHING_CONTROLLED_ROOT:
        case DIMINISHING_TRIGGER_ROOT:
        case DIMINISHING_FEAR:
        case DIMINISHING_CHARM:
        case DIMINISHING_POLYMORPH:
        case DIMINISHING_SILENCE:
        case DIMINISHING_DISARM:
        case DIMINISHING_DEATHCOIL:
        case DIMINISHING_FREEZE:
        case DIMINISHING_BANISH:
        case DIMINISHING_WARLOCK_FEAR:
        case DIMINISHING_KNOCKOUT:
            return DRTYPE_PLAYER;
        }

        return DRTYPE_NONE;
    };

#ifdef LICH_KING
    auto diminishingMaxLevelCompute = [](DiminishingGroup group) -> DiminishingLevels
    {
        switch (group)
        {
        case DIMINISHING_TAUNT:
            return DIMINISHING_LEVEL_TAUNT_IMMUNE;
        default:
            return DIMINISHING_LEVEL_IMMUNE;
        }
    };
#endif

    auto diminishingLimitDurationCompute = [this](DiminishingGroup group) -> int32
    {
        auto isGroupDurationLimited = [group]() -> bool
        {
            switch (group)
            {
            //groups are different on LK... not done here
            case DIMINISHING_CONTROLLED_STUN:
            case DIMINISHING_TRIGGER_STUN:
            case DIMINISHING_KIDNEYSHOT:
            case DIMINISHING_SLEEP:
            case DIMINISHING_CONTROLLED_ROOT:
            case DIMINISHING_TRIGGER_ROOT:
            case DIMINISHING_FEAR:
            case DIMINISHING_WARLOCK_FEAR:
            case DIMINISHING_CHARM:
            case DIMINISHING_POLYMORPH:
            case DIMINISHING_FREEZE:
            case DIMINISHING_KNOCKOUT:
            case DIMINISHING_BLIND_CYCLONE:
            case DIMINISHING_BANISH:
            case DIMINISHING_LIMITONLY:
                return true;
            default:
                return false;
            }
        };

        if (!isGroupDurationLimited())
            return 0;

#ifdef LICH_KING
        //not sure none of these are BC but I see no mention of this
        // Explicit diminishing duration
        switch (SpellFamilyName)
        {
        case SPELLFAMILY_DRUID:
        {
            // Faerie Fire - limit to 40 seconds in PvP (3.1)
            if (SpellFamilyFlags[0] & 0x400)
                return 40 * IN_MILLISECONDS;
            break;
        }
        case SPELLFAMILY_HUNTER:
        {
            // Wyvern Sting
            if (SpellFamilyFlags[1] & 0x1000)
                return 6 * IN_MILLISECONDS;
            // Hunter's Mark
            if (SpellFamilyFlags[0] & 0x400)
                return 120 * IN_MILLISECONDS;
            break;
        }
        case SPELLFAMILY_PALADIN:
        {
            // Repentance - limit to 6 seconds in PvP
            if (SpellFamilyFlags[0] & 0x4)
                return 6 * IN_MILLISECONDS;
            break;
        }
        case SPELLFAMILY_WARLOCK:
        {
            // Banish - limit to 6 seconds in PvP
            if (SpellFamilyFlags[1] & 0x8000000)
                return 6 * IN_MILLISECONDS;
            // Curse of Tongues - limit to 12 seconds in PvP
            else if (SpellFamilyFlags[2] & 0x800)
                return 12 * IN_MILLISECONDS;
            // Curse of Elements - limit to 120 seconds in PvP
            else if (SpellFamilyFlags[1] & 0x200)
                return 120 * IN_MILLISECONDS;
            break;
        }
        default:
            break;
        }

#endif
        return 10 * IN_MILLISECONDS;
    };

    SpellDiminishInfo triggeredInfo, normalInfo;
    triggeredInfo.DiminishGroup = diminishingGroupCompute(true);
    triggeredInfo.DiminishReturnType = diminishingTypeCompute(triggeredInfo.DiminishGroup);
#ifdef LICH_KING
    triggeredInfo.DiminishMaxLevel = diminishingMaxLevelCompute(triggeredInfo.DiminishGroup);
#endif
    triggeredInfo.DiminishDurationLimit = diminishingLimitDurationCompute(triggeredInfo.DiminishGroup);

    normalInfo.DiminishGroup = diminishingGroupCompute(false);
    normalInfo.DiminishReturnType = diminishingTypeCompute(normalInfo.DiminishGroup);
#ifdef LICH_KING
    normalInfo.DiminishMaxLevel = diminishingMaxLevelCompute(normalInfo.DiminishGroup);
#endif
    normalInfo.DiminishDurationLimit = diminishingLimitDurationCompute(normalInfo.DiminishGroup);

    _diminishInfoTriggered = triggeredInfo;
    _diminishInfoNonTriggered = normalInfo;
}

DiminishingGroup SpellInfo::GetDiminishingReturnsGroupForSpell(bool triggered) const
{
    return triggered ? _diminishInfoTriggered.DiminishGroup : _diminishInfoNonTriggered.DiminishGroup;
}

DiminishingReturnsType SpellInfo::GetDiminishingReturnsGroupType(bool triggered) const
{
    return triggered ? _diminishInfoTriggered.DiminishReturnType : _diminishInfoNonTriggered.DiminishReturnType;
}

DiminishingLevels SpellInfo::GetDiminishingReturnsMaxLevel(bool triggered) const
{
#ifdef LICH_KING
    return triggered ? _diminishInfoTriggered.DiminishMaxLevel : _diminishInfoNonTriggered.DiminishMaxLevel;
#else
    return DIMINISHING_LEVEL_IMMUNE;
#endif
}

int32 SpellInfo::GetDiminishingReturnsLimitDuration(bool triggered) const
{
    return triggered ? _diminishInfoTriggered.DiminishDurationLimit : _diminishInfoNonTriggered.DiminishDurationLimit;
}

int32 SpellInfo::GetDuration() const
{
    if (!DurationEntry)
        return 0;
    return (DurationEntry->Duration[0] == -1) ? -1 : abs(DurationEntry->Duration[0]);
}

int32 SpellInfo::GetMaxDuration() const
{
    if (!DurationEntry)
        return 0;
    return (DurationEntry->Duration[2] == -1) ? -1 : abs(DurationEntry->Duration[2]);
}


uint32 SpellInfo::GetMaxTicks() const
{
    int32 DotDuration = GetDuration();
    if (DotDuration == 0)
        return 1;

    // 200% limit
    if (DotDuration > 30000)
        DotDuration = 30000;

    for (const auto & Effect : Effects)
    {
        if (Effect.Effect == SPELL_EFFECT_APPLY_AURA)
            switch (Effect.ApplyAuraName)
            {
            case SPELL_AURA_PERIODIC_DAMAGE:
            case SPELL_AURA_PERIODIC_HEAL:
            case SPELL_AURA_PERIODIC_LEECH:
                if (Effect.Amplitude != 0)
                    return DotDuration / Effect.Amplitude;
                break;
            }
    }

    return 6;
}

int32 SpellInfo::CalcPowerCost(Unit const* caster, SpellSchoolMask schoolMask, Spell* spell) const
{
    // Spell drain all exist power on cast (Only paladin lay of Hands)
    if (HasAttribute(SPELL_ATTR1_DRAIN_ALL_POWER))
    {
        // If power type - health drain all
        if (PowerType == POWER_HEALTH)
            return caster->GetHealth();
        // Else drain all power
        if (PowerType < MAX_POWERS)
            return caster->GetPower(Powers(PowerType));
        TC_LOG_ERROR("spells", "Spell::CalculateManaCost: Unknown power type '%d' in spell %d", PowerType, Id);
        return 0;
    }

    // Base powerCost
    int32 powerCost = ManaCost;
    // PCT cost from total amount
    if (ManaCostPercentage)
    {
        switch (PowerType)
        {
            // health as power used
        case POWER_HEALTH:
            powerCost += int32(CalculatePct(caster->GetCreateHealth(), ManaCostPercentage));
            break;
        case POWER_MANA:
            powerCost += int32(CalculatePct(caster->GetCreateMana(), ManaCostPercentage));
            break;
        case POWER_RAGE:
        case POWER_FOCUS:
        case POWER_ENERGY:
        case POWER_HAPPINESS:
            powerCost += int32(CalculatePct(caster->GetMaxPower(Powers(PowerType)), ManaCostPercentage));
            break;
        default:
            TC_LOG_ERROR("spells", "CalculateManaCost: Unknown power type '%d' in spell %d", PowerType, Id);
            return 0;
        }
    }
    SpellSchools school = GetFirstSchoolInMask(schoolMask);
    // Flat mod from caster auras by spell school
    powerCost += caster->GetInt32Value(UNIT_FIELD_POWER_COST_MODIFIER + school);

    // Shiv - costs 20 + weaponSpeed*10 energy (apply only to non-triggered spell with energy cost)
    if (HasAttribute(SPELL_ATTR4_SPELL_VS_EXTEND_COST))
        powerCost += caster->GetAttackTime(OFF_ATTACK) / 100;

    // Apply cost mod by spell
    if (Player* modOwner = caster->GetSpellModOwner())
        modOwner->ApplySpellMod(Id, SPELLMOD_COST, powerCost, spell);

    if (!caster->IsControlledByPlayer())
    {
        if (HasAttribute(SPELL_ATTR0_LEVEL_DAMAGE_CALCULATION))
        {
            GtNPCManaCostScalerEntry const* spellScaler = sGtNPCManaCostScalerStore.LookupEntry(SpellLevel - 1);
            GtNPCManaCostScalerEntry const* casterScaler = sGtNPCManaCostScalerStore.LookupEntry(caster->GetLevel() - 1);
            if (spellScaler && casterScaler)
                powerCost *= casterScaler->ratio / spellScaler->ratio;
        }
        /* OLD CALC
        if (Attributes & SPELL_ATTR0_LEVEL_DAMAGE_CALCULATION)
        powerCost = int32(powerCost / (1.117f* SpellLevel / caster->GetLevel() - 0.1327f));
        */
    }

    // PCT mod from user auras by school
    powerCost = int32(powerCost * (1.0f + caster->GetFloatValue(UNIT_FIELD_POWER_COST_MULTIPLIER + school)));
    if (powerCost < 0)
        powerCost = 0;
    return powerCost;
}

uint32 SpellInfo::GetRecoveryTime() const
{
    return RecoveryTime > CategoryRecoveryTime ? RecoveryTime : CategoryRecoveryTime;
}

uint32 SpellInfo::CalcCastTime(Spell* spell) const
{
    // not all spells have cast time index and this is all is pasiive abilities
    if (!CastTimeEntry)
        return 0;

    int32 castTime = CastTimeEntry->CastTime;

    //todo, remove hack !
    if (spell && spell->m_spellInfo->Id == 8690) //heartstone
        return castTime;

    if (spell)
        spell->GetCaster()->ModSpellDurationTime(this, castTime, spell);
    
    if (HasAttribute(SPELL_ATTR0_RANGED) && (!IsAutoRepeatRangedSpell()))
        castTime += 500;

    return (castTime > 0) ? uint32(castTime) : 0;
}


bool SpellInfo::HasInitialAggro() const
{
    return !(HasAttribute(SPELL_ATTR1_NO_THREAT) || HasAttribute(SPELL_ATTR3_NO_INITIAL_AGGRO));
}

bool SpellInfo::IsItemFitToSpellRequirements(Item const* item) const
{
    // item neutral spell
    if (EquippedItemClass == -1)
        return true;

    // item dependent spell
    if (item && item->IsFitToSpellRequirements(this))
        return true;

    return false;
}

WeaponAttackType SpellInfo::GetAttackType() const
{
    WeaponAttackType result;
    switch (DmgClass)
    {
    case SPELL_DAMAGE_CLASS_MELEE:
        if (HasAttribute(SPELL_ATTR3_REQ_OFFHAND))
            result = OFF_ATTACK;
        else
            result = BASE_ATTACK;
        break;
    case SPELL_DAMAGE_CLASS_RANGED:
        result = IsRangedWeaponSpell() ? RANGED_ATTACK : BASE_ATTACK;
        break;
    default:
        // Wands
        if (HasAttribute(SPELL_ATTR3_REQ_WAND))
            result = RANGED_ATTACK;
        else
            result = BASE_ATTACK;
        break;
    }
    return result;
}

bool SpellInfo::IsAffected(uint32 familyName, uint64 const& familyFlags) const
{
    if (familyName == SPELLFAMILY_GENERIC)
        return true;

    if (familyName != SpellFamilyName)
        return false;

    if (familyFlags && !(familyFlags & SpellFamilyFlags))
        return false;

    return true;
}

bool SpellInfo::IsAffectedBySpellMods() const
{
    return !HasAttribute(SPELL_ATTR3_NO_DONE_BONUS);
}

bool SpellInfo::IsAffectedBySpellMod(SpellModifier const* mod) const
{
    if (!IsAffectedBySpellMods())
        return false;

    SpellInfo const* affectSpell = sSpellMgr->GetSpellInfo(mod->spellId);
    if (!affectSpell)
        return false;

    return IsAffected(affectSpell->SpellFamilyName, mod->mask);
}

bool SpellInfo::IsRangedWeaponSpell() const
{
    return ((EquippedItemSubClassMask & ITEM_SUBCLASS_MASK_WEAPON_RANGED)
        || (Attributes & SPELL_ATTR0_RANGED)); // sunwell
}

bool SpellInfo::IsAutoRepeatRangedSpell() const
{
    return HasAttribute(SPELL_ATTR2_AUTOREPEAT_FLAG);
}

SpellInfo::~SpellInfo()
{
    _UnloadImplicitTargetConditionLists();
}

// checks if spell targets are selected from area, doesn't include spell effects in check (like area wide auras for example)
bool SpellInfo::IsTargetingArea() const
{
    for (const auto & Effect : Effects)
        if (Effect.IsEffect() && Effect.IsTargetingArea())
            return true;
    return false;
}

bool SpellInfo::IsAffectingArea() const
{
    for (const auto & Effect : Effects)
        if (Effect.IsEffect() && (Effect.IsTargetingArea() || Effect.IsEffect(SPELL_EFFECT_PERSISTENT_AREA_AURA) || Effect.IsAreaAuraEffect()))
            return true;
    return false;
}

bool SpellInfo::NeedsExplicitUnitTarget() const
{
    return GetExplicitTargetMask() & TARGET_FLAG_UNIT_MASK;
}

bool SpellInfo::NeedsToBeTriggeredByCaster(SpellInfo const* triggeringSpell, uint8 effIndex) const
{
    if (NeedsExplicitUnitTarget())
        return true;

    // sunwell:
    if (effIndex < MAX_SPELL_EFFECTS && (triggeringSpell->Effects[effIndex].TargetA.GetCheckType() == TARGET_CHECK_ENTRY || triggeringSpell->Effects[effIndex].TargetB.GetCheckType() == TARGET_CHECK_ENTRY))
    {
#ifdef LICH_KING
        if (Id == 60563)
            return true;
#endif
        for (const auto & Effect : Effects)
            if (Effect.IsEffect() && (Effect.TargetA.GetCheckType() == TARGET_CHECK_ENTRY || Effect.TargetB.GetCheckType() == TARGET_CHECK_ENTRY))
                return true;
    }

    if (triggeringSpell->IsChanneled())
    {
        uint32 mask = 0;
        for (const auto & Effect : Effects)
        {
            if (Effect.TargetA.GetTarget() != TARGET_UNIT_CASTER && Effect.TargetA.GetTarget() != TARGET_DEST_CASTER)
                mask |= Effect.GetProvidedTargetMask();
        }

        if (mask & TARGET_FLAG_UNIT_MASK)
            return true;
    }

    return false;
}

bool SpellInfo::CanBeUsedInCombat() const
{
    return !HasAttribute(SPELL_ATTR0_CANT_USED_IN_COMBAT);
}

bool SpellInfo::HasAreaAuraEffect() const
{
    for (const auto & Effect : Effects)
        if (Effect.IsAreaAuraEffect())
            return true;
    return false;
}

bool SpellInfo::HasOnlyDamageEffects() const
{
    for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
    {
        if (Effects[i].IsEffect())
        {
            switch (Effects[i].Effect)
            {
            case SPELL_EFFECT_WEAPON_DAMAGE:
            case SPELL_EFFECT_WEAPON_DAMAGE_NOSCHOOL:
            case SPELL_EFFECT_NORMALIZED_WEAPON_DMG:
            case SPELL_EFFECT_WEAPON_PERCENT_DAMAGE:
            case SPELL_EFFECT_SCHOOL_DAMAGE:
            case SPELL_EFFECT_ENVIRONMENTAL_DAMAGE:
            case SPELL_EFFECT_HEALTH_LEECH:
                continue;
            default:
                return false;
            }
        }
    }

    return true;
}

uint32 SpellInfo::GetCategory() const
{
    return Category ? Category->Id : 0;
}

bool SpellInfo::HasEffectByEffectMask(SpellEffects effect, SpellEffectMask effectMask) const
{
    assert(effectMask <= SPELL_EFFECT_MASK_ALL);

    for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
    {
        if( (effectMask & (1 << i)) && Effects[i].IsEffect(effect))
            return true;
    }

    return false;
}

bool SpellInfo::HasEffect(SpellEffects effect, uint8 effectIndex) const
{
    return HasEffectByEffectMask(effect, SpellEffectMask(1 << effectIndex));
}

bool SpellInfo::HasAura(AuraType aura) const
{
    for (const auto & Effect : Effects)
        if (Effect.IsAura(aura))
            return true;
    return false;
}

bool SpellInfo::HasAuraEffect(AuraType aura) const
{
    for (const auto & Effect : Effects)
        if (Effect.IsAura(aura))
            return true;
    return false;
}

bool SpellInfo::IsBreakingStealth() const
{
    return !(AttributesEx & SPELL_ATTR1_NOT_BREAK_STEALTH);
}

bool SpellInfo::IsValidDeadOrAliveTarget(Unit const* target) const
{
    if (target->IsAlive())
        return !IsRequiringDeadTarget();

    return IsAllowingDeadTarget();
}

bool SpellInfo::IsRequiringDeadTarget() const
{
    return AttributesEx3 & SPELL_ATTR3_ONLY_TARGET_GHOSTS;
}

bool SpellInfo::IsAllowingDeadTarget() const
{
    return AttributesEx2 & (SPELL_ATTR2_CAN_TARGET_DEAD) || Attributes & (SPELL_ATTR0_CASTABLE_WHILE_DEAD) || Targets & (TARGET_FLAG_CORPSE_ALLY | TARGET_FLAG_CORPSE_ENEMY | TARGET_FLAG_UNIT_DEAD);
}

bool SpellInfo::IsGroupBuff() const
{
    for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
    {
        switch (Effects[i].TargetA.GetCheckType())
        {
        case TARGET_CHECK_PARTY:
        case TARGET_CHECK_RAID:
        case TARGET_CHECK_RAID_CLASS:
            return true;
        default:
            break;
        }
    }

    return false;
}

bool SpellInfo::IsChanneled() const
{
    return (AttributesEx & (SPELL_ATTR1_CHANNELED_1 | SPELL_ATTR1_CHANNELED_2)) != 0;
}

bool SpellInfo::IsMoveAllowedChannel() const
{
    return IsChanneled() && HasAttribute(SPELL_ATTR5_CAN_CHANNEL_WHEN_MOVING);
}

bool SpellInfo::IsNextMeleeSwingSpell() const
{
    return HasAttribute(SpellAttr0(SPELL_ATTR0_ON_NEXT_SWING | SPELL_ATTR0_ON_NEXT_SWING_2));
}

bool SpellInfo::NeedsComboPoints() const
{
    return (AttributesEx & (SPELL_ATTR1_REQ_COMBO_POINTS1 | SPELL_ATTR1_REQ_COMBO_POINTS2)) != 0;
}

bool SpellInfo::IsDeathPersistent() const
{
    switch(Id)
    {
        case 40214:                                     // Dragonmaw Illusion
        case 35480: case 35481: case 35482:             // Human Illusion
        case 35483: case 39824:                         // Human Illusion
        case 17619:
        case 37128:                                     // Doomwalker's Mark of Death
        case 37800: case 37801:                         // GM transparency
            return true;
    }

    return (AttributesEx3 & SPELL_ATTR3_DEATH_PERSISTENT) != 0;
}

bool SpellInfo::HasVisual(uint32 visual) const
{
#ifdef LICH_KING
    for (uint8 i = 0; i < 2; i++)
        if (SpellVisual[i] == visual)
            return true;

    return false;
#else
    return SpellVisual == visual;
#endif
}

SpellCastResult SpellInfo::CheckTarget(Unit const* caster, WorldObject const* target, bool implicit) const
{
    if (AttributesEx & SPELL_ATTR1_CANT_TARGET_SELF && caster == target)
        return SPELL_FAILED_BAD_TARGETS;

    // check visibility - ignore stealth for implicit (area) targets
    if (!(AttributesEx6 & SPELL_ATTR6_CAN_TARGET_INVISIBLE) && !caster->CanSeeOrDetect(target, implicit))
        return SPELL_FAILED_BAD_TARGETS;
    
    Unit const* unitTarget = target->ToUnit();

    // creature/player specific target checks
    if (unitTarget)
    {
        // spells cannot be cast if target has a pet in combat either
        if (HasAttribute(SPELL_ATTR1_CANT_TARGET_IN_COMBAT) && (unitTarget->IsInCombat() || unitTarget->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PET_IN_COMBAT)))
            return SPELL_FAILED_TARGET_AFFECTING_COMBAT;

        // only spells with SPELL_ATTR3_ONLY_TARGET_GHOSTS can target ghosts
        if (((IsRequiringDeadTarget() != 0) != unitTarget->HasAuraType(SPELL_AURA_GHOST)) && !(IsDeathPersistent() && IsAllowingDeadTarget()))
        {
            if (AttributesEx3 & SPELL_ATTR3_ONLY_TARGET_GHOSTS)
                return SPELL_FAILED_TARGET_NOT_GHOST;
            else
                return SPELL_FAILED_BAD_TARGETS;
        }

        if (caster != unitTarget)
        {
            if (caster->GetTypeId() == TYPEID_PLAYER)
            {
                // Do not allow these spells to target creatures not tapped by us (Banish, Polymorph, many quest spells)
                if (AttributesEx2 & SPELL_ATTR2_CANT_TARGET_TAPPED)
                    if (Creature const* targetCreature = unitTarget->ToCreature())
                        if (targetCreature->hasLootRecipient() && !targetCreature->isTappedBy(caster->ToPlayer()))
                            return SPELL_FAILED_CANT_CAST_ON_TAPPED;

                if (AttributesCu & SPELL_ATTR0_CU_PICKPOCKET)
                {
                    if (unitTarget->GetTypeId() == TYPEID_PLAYER)
                        return SPELL_FAILED_BAD_TARGETS;
                    else if ((unitTarget->GetCreatureTypeMask() & CREATURE_TYPEMASK_HUMANOID_OR_UNDEAD) == 0)
                        return SPELL_FAILED_TARGET_NO_POCKETS;
                }
            }
            // Not allow disarm unarmed target (sun: moved out of player check, creatures can be disarmed too)
            if (Mechanic == MECHANIC_DISARM)
            {
                if (!unitTarget->HasMainWeapon())
                    return SPELL_FAILED_TARGET_NO_WEAPONS;
            }
        }
    }
    // corpse specific target checks
    else if (Corpse const* corpseTarget = target->ToCorpse())
    {
        // cannot target bare bones
        if (corpseTarget->GetType() == CORPSE_BONES)
            return SPELL_FAILED_BAD_TARGETS;
        // we have to use owner for some checks (aura preventing resurrection for example)
        if (Player* owner = ObjectAccessor::FindPlayer(corpseTarget->GetOwnerGUID()))
            unitTarget = owner;
        // we're not interested in corpses without owner
        else
            return SPELL_FAILED_BAD_TARGETS;
    }
    // other types of objects - always valid
    else return SPELL_CAST_OK;

    // corpseOwner and unit specific target checks
    if (AttributesEx3 & SPELL_ATTR3_ONLY_TARGET_PLAYERS && !unitTarget->ToPlayer())
        return SPELL_FAILED_TARGET_NOT_PLAYER;

    if (!IsAllowingDeadTarget() && !unitTarget->IsAlive())
        return SPELL_FAILED_TARGETS_DEAD;

    // check this flag only for implicit targets (chain and area), allow to explicitly target units for spells like Shield of Righteousness
    if (implicit && AttributesEx6 & SPELL_ATTR6_CANT_TARGET_CROWD_CONTROLLED && !unitTarget->CanFreeMove())
        return SPELL_FAILED_BAD_TARGETS;

    // checked in Unit::IsValidAttack/AssistTarget, shouldn't be checked for ENTRY targets
    //if (!(AttributesEx6 & SPELL_ATTR6_CAN_TARGET_UNTARGETABLE) && target->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE))
    //    return SPELL_FAILED_BAD_TARGETS;

    //if (!(AttributesEx6 & SPELL_ATTR6_CAN_TARGET_POSSESSED_FRIENDS)

    if (!CheckTargetCreatureType(unitTarget))
    {
        if (target->GetTypeId() == TYPEID_PLAYER)
            return SPELL_FAILED_TARGET_IS_PLAYER;
        else
            return SPELL_FAILED_BAD_TARGETS;
    }

    // check GM mode and GM invisibility - only for player casts (npc casts are controlled by AI) and negative spells
    if (unitTarget != caster && (caster->IsControlledByPlayer() || !IsPositive()) && unitTarget->GetTypeId() == TYPEID_PLAYER)
    {
        if (!unitTarget->ToPlayer()->IsVisible())
            return SPELL_FAILED_BM_OR_INVISGOD;

        if (!IsPositive() && unitTarget->ToPlayer()->IsGameMaster())
            return SPELL_FAILED_BM_OR_INVISGOD;
    }

    // not allow casting on flying player
    if (unitTarget->IsInFlight() /* sunwell && !HasAttribute(SPELL_ATTR0_CU_ALLOW_INFLIGHT_TARGET) */)
        return SPELL_FAILED_BAD_TARGETS;

    /* TARGET_UNIT_MASTER gets blocked here for passengers, because the whole idea of this check is to
    not allow passengers to be implicitly hit by spells, however this target type should be an exception,
    if this is left it kills spells that award kill credit from vehicle to master (few spells),
    the use of these 2 covers passenger target check, logically, if vehicle cast this to master it should always hit
    him, because it would be it's passenger, there's no such case where this gets to fail legitimacy, this problem
    cannot be solved from within the check in other way since target type cannot be called for the spell currently
    Spell examples: [ID - 52864 Devour Water, ID - 52862 Devour Wind, ID - 49370 Wyrmrest Defender: Destabilize Azure Dragonshrine Effect] */
    if (
#ifdef LICH_KING
        !caster->IsVehicle() &&
#endif
        !(caster->GetCharmerOrOwner() == target))
    {
        if (TargetAuraState && !unitTarget->HasAuraState(AuraStateType(TargetAuraState), this, caster))
            return SPELL_FAILED_TARGET_AURASTATE;

        if (TargetAuraStateNot && unitTarget->HasAuraState(AuraStateType(TargetAuraStateNot), this, caster))
            return SPELL_FAILED_TARGET_AURASTATE;
    }

#ifdef LICH_KING
    if (TargetAuraSpell && !unitTarget->HasAura(sSpellMgr->GetSpellIdForDifficulty(TargetAuraSpell, caster)))
        return SPELL_FAILED_TARGET_AURASTATE;

    if (ExcludeTargetAuraSpell && unitTarget->HasAura(sSpellMgr->GetSpellIdForDifficulty(ExcludeTargetAuraSpell, caster)))
        return SPELL_FAILED_TARGET_AURASTATE;

    if (unitTarget->HasAuraType(SPELL_AURA_PREVENT_RESURRECTION))
        if (HasEffect(SPELL_EFFECT_SELF_RESURRECT) || HasEffect(SPELL_EFFECT_RESURRECT) || HasEffect(SPELL_EFFECT_RESURRECT_NEW))
            return SPELL_FAILED_TARGET_CANNOT_BE_RESURRECTED;
#endif

    return SPELL_CAST_OK;
}

SpellCastResult SpellInfo::CheckExplicitTarget(Unit const* caster, WorldObject const* target, Item const* itemTarget) const
{
    uint32 neededTargets = GetExplicitTargetMask();
    if (!target)
    {
        if (neededTargets & (TARGET_FLAG_UNIT_MASK | TARGET_FLAG_GAMEOBJECT_MASK | TARGET_FLAG_CORPSE_MASK))
            if (!(neededTargets & TARGET_FLAG_GAMEOBJECT_ITEM) || !itemTarget)
                return SPELL_FAILED_BAD_TARGETS;
        return SPELL_CAST_OK;
    }

    if (Unit const* unitTarget = target->ToUnit())
    {
        if (neededTargets & (TARGET_FLAG_UNIT_ENEMY | TARGET_FLAG_UNIT_ALLY | TARGET_FLAG_UNIT_RAID | TARGET_FLAG_UNIT_PARTY | TARGET_FLAG_UNIT_MINIPET 
#ifdef LICH_KING
            | TARGET_FLAG_UNIT_PASSENGER
#endif
            ))
        {
            if (neededTargets & TARGET_FLAG_UNIT_ENEMY)
                if (caster->_IsValidAttackTarget(unitTarget, this))
                    return SPELL_CAST_OK;
            if (neededTargets & TARGET_FLAG_UNIT_ALLY
                || (neededTargets & TARGET_FLAG_UNIT_PARTY && caster->IsInPartyWith(unitTarget))
                || (neededTargets & TARGET_FLAG_UNIT_RAID && caster->IsInRaidWith(unitTarget)))
                if (caster->_IsValidAssistTarget(unitTarget, this))
                    return SPELL_CAST_OK;
            if (neededTargets & TARGET_FLAG_UNIT_MINIPET)
                if (unitTarget->GetGUID() == caster->GetCritterGUID())
                    return SPELL_CAST_OK;
#ifdef LICH_KING
            if (neededTargets & TARGET_FLAG_UNIT_PASSENGER)
                if (unitTarget->IsOnVehicle(caster))
                    return SPELL_CAST_OK;
#endif
            return SPELL_FAILED_BAD_TARGETS;
        }
    }
    return SPELL_CAST_OK;
}

bool SpellInfo::CheckTargetCreatureType(Unit const* target) const
{
    // Curse of Doom & Exorcism: not find another way to fix spell target check :/
    if (SpellFamilyName == SPELLFAMILY_WARLOCK && GetCategory() == 1179)
    {
        // not allow cast at player
        if (target->GetTypeId() == TYPEID_PLAYER)
            return false;
        else
            return true;
    }
    uint32 creatureType = target->GetCreatureTypeMask();
    return !TargetCreatureType || !creatureType || (creatureType & TargetCreatureType);
}

SpellSchoolMask SpellInfo::GetSchoolMask() const
{
    return SpellSchoolMask(SchoolMask);
}

uint32 SpellInfo::GetAllEffectsMechanicMask() const
{
    uint32 mask = 0;
    if (Mechanic)
        mask |= 1 << Mechanic;
    for (const auto & Effect : Effects)
        if (Effect.IsEffect() && Effect.Mechanic)
            mask |= 1 << Effect.Mechanic;
    return mask;
}

uint32 SpellInfo::GetEffectMechanicMask(uint8 effIndex) const
{
    uint32 mask = 0;
    if (Mechanic)
        mask |= 1 << Mechanic;
    if (Effects[effIndex].IsEffect() && Effects[effIndex].Mechanic)
        mask |= 1 << Effects[effIndex].Mechanic;
    return mask;
}

uint32 SpellInfo::GetSpellMechanicMaskByEffectMask(SpellEffectMask effectMask) const
{
    uint32 mask = 0;
    if (Mechanic)
        mask |= 1 << Mechanic;
    for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
        if ((effectMask & (1 << i)) && Effects[i].Mechanic)
            mask |= 1 << Effects[i].Mechanic;
    return mask;
}

Mechanics SpellInfo::GetEffectMechanic(uint8 effIndex) const
{
    if (Effects[effIndex].IsEffect() && Effects[effIndex].Mechanic)
        return Mechanics(Effects[effIndex].Mechanic);
    if (Mechanic)
        return Mechanics(Mechanic);
    return MECHANIC_NONE;
}

bool SpellInfo::HasAnyEffectMechanic() const
{
    for (const auto & Effect : Effects)
        if (Effect.Mechanic)
            return true;
    return false;
}

float SpellInfo::GetMinRange(bool /* positive */) const
{
    if (!RangeEntry)
        return 0.0f;
#ifdef LICH_KING
    if (positive)
        return RangeEntry->minRangeFriend;
    else
        return RangeEntry->minRangeHostile;
#endif
    return RangeEntry->minRange;
}

float SpellInfo::GetMaxRange(bool positive, Unit* caster, Spell* spell) const
{
    if (!RangeEntry)
        return 0.0f;
    float range;
#ifdef LICH_KING
    if (positive)
        range = RangeEntry->maxRangeFriend;
    else
        range = RangeEntry->maxRangeHostile;
#endif
    range = RangeEntry->maxRange;
    if (caster)
        if (Player* modOwner = caster->GetSpellModOwner())
            modOwner->ApplySpellMod(Id, SPELLMOD_RANGE, range, spell);
    return range;
}

bool SpellInfo::IsCooldownStartedOnEvent() const
{
    if (HasAttribute(SPELL_ATTR0_DISABLED_WHILE_ACTIVE))
        return true;

    return Category && Category->Flags & SPELL_CATEGORY_FLAG_COOLDOWN_STARTS_ON_EVENT;
}

bool SpellInfo::IsChannelCategorySpell() const
{
    for (const auto & Effect : Effects)
        if (Effect.TargetA.GetSelectionCategory() == TARGET_SELECT_CATEGORY_CHANNEL || Effect.TargetB.GetSelectionCategory() == TARGET_SELECT_CATEGORY_CHANNEL)
            return true;
    return false;
}

bool SpellInfo::IsAutocastable() const
{
    if (IsPassive())
        return false;
    if (HasAttribute(SPELL_ATTR1_UNAUTOCASTABLE_BY_PET))
        return false;
    return true;
}

bool SpellInfo::IsPassiveStackableWithRanks() const
{
    return IsPassive() 
        && !HasEffect(SPELL_EFFECT_APPLY_AURA) && !HasEffect(SPELL_EFFECT_APPLY_AREA_AURA_PARTY) && !HasEffect(SPELL_EFFECT_APPLY_AREA_AURA_FRIEND); //sun, added SPELL_EFFECT_APPLY_AREA_AURA_PARTY && SPELL_EFFECT_APPLY_AREA_AURA_FRIEND, else this would make mana totem stacks
}

bool SpellInfo::IsPassive() const
{
    return HasAttribute(SPELL_ATTR0_PASSIVE);
}

bool SpellInfo::IsMultiSlotAura() const
{
    return IsPassive() 
#ifdef LICH_KING
        || Id == 55849 || Id == 40075 || Id == 44413
#endif
        ; // Power Spark, Fel Flak Fire, Incanter's Absorption
}

bool SpellInfo::IsStackableOnOneSlotWithDifferentCasters() const
{
    /// TODO: Re-verify meaning of SPELL_ATTR3_STACK_FOR_DIFF_CASTERS and update conditions here
    return StackAmount > 1 && !IsChanneled() && !HasAttribute(SPELL_ATTR3_STACK_FOR_DIFF_CASTERS);
}

bool SpellInfo::IsPositive(bool hostileTarget /* = false */) const
{
    if(HasEffect(SPELL_EFFECT_DISPEL) || HasEffect(SPELL_EFFECT_DISPEL_MECHANIC))
        return !hostileTarget;  // positive on friendly, negative on hostile

    return !HasAttribute(SPELL_ATTR0_CU_NEGATIVE);
}

bool SpellInfo::IsPositiveEffect(uint8 effIndex, bool hostileTarget /* = false */) const
{
    if(HasEffect(SPELL_EFFECT_DISPEL, effIndex) || HasEffect(SPELL_EFFECT_DISPEL_MECHANIC, effIndex))
        return !hostileTarget;  // positive on friendly, negative on hostile

    switch (effIndex)
    {
        default:
        case 0:
            return !HasAttribute(SPELL_ATTR0_CU_NEGATIVE_EFF0);
        case 1:
            return !HasAttribute(SPELL_ATTR0_CU_NEGATIVE_EFF1);
        case 2:
            return !HasAttribute(SPELL_ATTR0_CU_NEGATIVE_EFF2);
    }
}

uint32 SpellInfo::GetDispelMask() const
{
    return GetDispelMask(DispelType(Dispel));
}

uint32 SpellInfo::GetDispelMask(DispelType type)
{
    // If dispel all
    if (type == DISPEL_ALL)
        return DISPEL_ALL_MASK;
    else
        return uint32(1 << type);
}

uint32 SpellInfo::GetExplicitTargetMask() const
{
    return ExplicitTargetMask;
}

AuraStateType SpellInfo::GetAuraState() const
{
    if (GetSpellSpecific() == SPELL_SPECIFIC_SEAL)
        return AURA_STATE_JUDGEMENT;

    // Conflagrate aura state
    if (SpellFamilyName == SPELLFAMILY_WARLOCK && (SpellFamilyFlags & SPELLFAMILYFLAG_WARLOCK_IMMOLATE))
        return AURA_STATE_CONFLAGRATE;

    // Swiftmend state on Regrowth & Rejuvenation
    if (SpellFamilyName == SPELLFAMILY_DRUID
        && (SpellFamilyFlags & (SPELLFAMILYFLAG_DRUID_REJUVENATION | SPELLFAMILYFLAG_DRUID_REGROWTH)))
        return AURA_STATE_SWIFTMEND;

    // Deadly poison
    if (SpellFamilyName == SPELLFAMILY_ROGUE && (SpellFamilyFlags & SPELLFAMILYFLAG_ROGUE_DEADLYPOISON))
        return AURA_STATE_DEADLY_POISON;

    // Faerie Fire (druid versions)
    if ((SpellFamilyName == SPELLFAMILY_DRUID &&
        SpellFamilyFlags & SPELLFAMILYFLAG_DRUID_FAERIEFIRE)
        || Id == 35325) //glowing blood (why ?)
        return AURA_STATE_FAERIE_FIRE;

    if (GetSchoolMask() & SPELL_SCHOOL_MASK_FROST)
        for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
            if (Effects[i].IsAura() && (Effects[i].ApplyAuraName == SPELL_AURA_MOD_STUN
                || Effects[i].ApplyAuraName == SPELL_AURA_MOD_ROOT))
                return AURA_STATE_FROZEN;

    // Forbearance and Weakened Soul
    for (const auto & Effect : Effects)
    {
        if (Effect.ApplyAuraName == SPELL_AURA_MECHANIC_IMMUNITY)
        {
            if (Effect.MiscValue == MECHANIC_INVULNERABILITY)
                return AURA_STATE_FORBEARANCE;
            if (Effect.MiscValue == MECHANIC_SHIELD)
                return AURA_STATE_WEAKENED_SOUL;
        }
    }

    if (SpellFamilyName == SPELLFAMILY_WARRIOR
        && (SpellFamilyFlags == SPELLFAMILYFLAG_WARRIOR_VICTORYRUSH))
        return AURA_STATE_WARRIOR_VICTORY_RUSH;

    switch (Id)
    {
    case 41425: // Hypothermia
        return AURA_STATE_HYPOTHERMIA;
    case 32216: // Victorious
        return AURA_STATE_WARRIOR_VICTORY_RUSH;
    case 1661: // Berserking (troll racial traits)
        return AURA_STATE_BERSERKING;
    default:
        break;
    }

    return AURA_STATE_NONE;
}

bool SpellInfo::IsStackableWithRanks() const
{
    if (IsPassive())
        return false;
    if (PowerType != POWER_MANA && PowerType != POWER_HEALTH)
        return false;
    if (IsProfessionOrRiding())
        return false;

    if (IsAbilityLearnedWithProfession())
        return false;

    // All stance spells. if any better way, change it.
    for (const auto & Effect : Effects)
    {
        switch (SpellFamilyName)
        {
        case SPELLFAMILY_PALADIN:
            // Paladin aura Spell
#ifdef LICH_KING
            if (Effects[i].Effect == SPELL_EFFECT_APPLY_AREA_AURA_RAID)
#else
            if (Effect.Effect == SPELL_EFFECT_APPLY_AREA_AURA_PARTY)
#endif
                return false;
            break;
        case SPELLFAMILY_DRUID:
            // Druid form Spell
            if (Effect.Effect == SPELL_EFFECT_APPLY_AURA &&
                Effect.ApplyAuraName == SPELL_AURA_MOD_SHAPESHIFT)
                return false;
            break;
        }
    }
    return true;
}


bool SpellInfo::IsBinarySpell() const
{
    return AttributesCu & SPELL_ATTR0_CU_BINARY_SPELL;
}

bool SpellInfo::IsProfessionOrRiding() const
{
    for (const auto & Effect : Effects)
    {
        if (Effect.Effect == SPELL_EFFECT_SKILL)
        {
            uint32 skill = Effect.MiscValue;

            if (sSpellMgr->IsProfessionOrRidingSkill(skill))
                return true;
        }
    }
    return false;
}

bool SpellInfo::IsAbilityLearnedWithProfession() const
{
    SkillLineAbilityMapBounds bounds = sSpellMgr->GetSkillLineAbilityMapBounds(Id);

    for (auto _spell_idx = bounds.first; _spell_idx != bounds.second; ++_spell_idx)
    {
        SkillLineAbilityEntry const* pAbility = _spell_idx->second;
        if (!pAbility || pAbility->AutolearnType != SKILL_LINE_ABILITY_LEARNED_ON_SKILL_VALUE)
            continue;

        if (pAbility->req_skill_value > 0)
            return true;
    }

    return false;
}

int32 SpellEffectInfo::CalcValue(Unit const* caster, int32 const* bp, Unit const* /*target*/) const
{
    float basePointsPerLevel = RealPointsPerLevel;
    int32 basePoints = bp ? *bp : BasePoints;
    int32 randomPoints = int32(DieSides);

    if (caster && basePointsPerLevel)
    {
        //Cap caster level with MaxLevel (upper) and BaseLevel (lower), then get the difference between that level and the spell level
        int32 level = int32(caster->GetLevel());
        if (level > (int32)_spellInfo->MaxLevel && _spellInfo->MaxLevel > 0)
            level = (int32)_spellInfo->MaxLevel;
        else if (level < (int32)_spellInfo->BaseLevel)
            level = (int32)_spellInfo->BaseLevel;

        // if base level is greater than spell level, reduce by base level (eg. pilgrims foods)
        level -= int32(std::max(_spellInfo->BaseLevel, _spellInfo->SpellLevel));
        basePoints += int32(level * basePointsPerLevel);
    }

    // roll in a range <1;EffectDieSides> as of patch 3.3.3 (whats the diff for BC?)
    switch (randomPoints)
    {
    case 0: break;
    case 1: basePoints += 1; break;                     // range 1..1
    default:
        // range can have positive (1..rand) and negative (rand..1) values, so order its for irand
        int32 randvalue = (randomPoints >= 1)
            ? irand(1, randomPoints)
            : irand(randomPoints, 1);

        basePoints += randvalue;
        break;
    }

    float value = float(basePoints);

    // random damage
    if (caster)
    {
        //random damage
        if (PointsPerComboPoint != 0 && caster->GetTypeId() == TYPEID_PLAYER)
            value += (int32)(PointsPerComboPoint *  caster->ToPlayer()->GetComboPoints());

        value = caster->ApplyEffectModifiers(_spellInfo, _effIndex, value);

        if (!basePointsPerLevel && (_spellInfo->Attributes & SPELL_ATTR0_LEVEL_DAMAGE_CALCULATION && _spellInfo->SpellLevel) &&
            _spellInfo->Effects[_effIndex].Effect != SPELL_EFFECT_WEAPON_PERCENT_DAMAGE &&
            _spellInfo->Effects[_effIndex].Effect != SPELL_EFFECT_KNOCK_BACK)
        {
            bool canEffectScale = false;
            switch (_spellInfo->Effects[_effIndex].Effect)
            {
            case SPELL_EFFECT_SCHOOL_DAMAGE:
            case SPELL_EFFECT_DUMMY:
            case SPELL_EFFECT_POWER_DRAIN:
            case SPELL_EFFECT_HEALTH_LEECH:
            case SPELL_EFFECT_HEAL:
            case SPELL_EFFECT_WEAPON_DAMAGE:
            case SPELL_EFFECT_POWER_BURN:
            case SPELL_EFFECT_SCRIPT_EFFECT:
            case SPELL_EFFECT_NORMALIZED_WEAPON_DMG:
            case SPELL_EFFECT_FORCE_CAST_WITH_VALUE:
            case SPELL_EFFECT_TRIGGER_SPELL_WITH_VALUE:
#ifdef LICH_KING
            case SPELL_EFFECT_TRIGGER_MISSILE_SPELL_WITH_VALUE:
#endif
                canEffectScale = true;
                break;
            default:
                break;
            }

            switch (_spellInfo->Effects[_effIndex].ApplyAuraName)
            {
            case SPELL_AURA_PERIODIC_DAMAGE:
            case SPELL_AURA_DUMMY:
            case SPELL_AURA_PERIODIC_HEAL:
            case SPELL_AURA_DAMAGE_SHIELD:
            case SPELL_AURA_PROC_TRIGGER_DAMAGE:
            case SPELL_AURA_PERIODIC_LEECH:
            case SPELL_AURA_PERIODIC_MANA_LEECH:
            case SPELL_AURA_SCHOOL_ABSORB:
            case SPELL_AURA_PERIODIC_TRIGGER_SPELL_WITH_VALUE:
                canEffectScale = true;
                break;
            default:
                break;
            }

            if (canEffectScale)
            {
                GtNPCManaCostScalerEntry const* spellScaler = sGtNPCManaCostScalerStore.LookupEntry(_spellInfo->SpellLevel - 1);
                GtNPCManaCostScalerEntry const* casterScaler = sGtNPCManaCostScalerStore.LookupEntry(caster->GetLevel() - 1);
                if (spellScaler && casterScaler)
                    value *= casterScaler->ratio / spellScaler->ratio;
                //old calculation //value = int32(value * (int32)GetLevel() / (int32)(_spellInfo->SpellLevel ? _spellInfo->SpellLevel : 1));
            }
        }
    }

    return value;
}

int32 SpellEffectInfo::CalcBaseValue(int32 value) const
{
    if (DieSides == 0)
        return value;
    else
        return value - 1;
}

float SpellEffectInfo::CalcValueMultiplier(Unit* caster, Spell* spell) const
{
    float multiplier = ValueMultiplier;
    if (Player* modOwner = (caster ? caster->GetSpellModOwner() : nullptr))
        modOwner->ApplySpellMod(_spellInfo->Id, SPELLMOD_VALUE_MULTIPLIER, multiplier, spell);
    return multiplier;
}

float SpellEffectInfo::CalcDamageMultiplier(Unit* caster, Spell* spell) const
{
    float multiplierPercent = DamageMultiplier * 100.0f;
    if (Player* modOwner = (caster ? caster->GetSpellModOwner() : nullptr))
        modOwner->ApplySpellMod(_spellInfo->Id, SPELLMOD_DAMAGE_MULTIPLIER, multiplierPercent, spell);
        
    return multiplierPercent / 100.0f; 
}

bool SpellEffectInfo::HasRadius() const
{
    return RadiusEntry != nullptr;
}

float SpellEffectInfo::CalcRadius(Unit* caster, Spell* spell) const
{
    if (!HasRadius())
        return 0.0f;

    float radius = RadiusEntry->RadiusMin;
    if (caster)
    {
        //LK radius += RadiusEntry->RadiusPerLevel * caster->getLevel();
        radius = std::min(radius, RadiusEntry->RadiusMax);
        if (Player* modOwner = caster->GetSpellModOwner())
            modOwner->ApplySpellMod(_spellInfo->Id, SPELLMOD_RADIUS, radius, spell);
    }

    return radius;
}


uint32 SpellEffectInfo::GetProvidedTargetMask() const
{
    return GetTargetFlagMask(TargetA.GetObjectType()) | GetTargetFlagMask(TargetB.GetObjectType());
}

uint32 SpellEffectInfo::GetMissingTargetMask(bool srcSet /*= false*/, bool dstSet /*= false*/, uint32 mask /*=0*/) const
{
    uint32 effImplicitTargetMask = GetTargetFlagMask(GetUsedTargetObjectType());
    uint32 providedTargetMask = GetTargetFlagMask(TargetA.GetObjectType()) | GetTargetFlagMask(TargetB.GetObjectType()) | mask;

    // remove all flags covered by effect target mask
    if (providedTargetMask & TARGET_FLAG_UNIT_MASK)
        effImplicitTargetMask &= ~(TARGET_FLAG_UNIT_MASK);
    if (providedTargetMask & TARGET_FLAG_CORPSE_MASK)
        effImplicitTargetMask &= ~(TARGET_FLAG_UNIT_MASK | TARGET_FLAG_CORPSE_MASK);
    if (providedTargetMask & TARGET_FLAG_GAMEOBJECT_ITEM)
        effImplicitTargetMask &= ~(TARGET_FLAG_GAMEOBJECT_ITEM | TARGET_FLAG_GAMEOBJECT | TARGET_FLAG_ITEM);
    if (providedTargetMask & TARGET_FLAG_GAMEOBJECT)
        effImplicitTargetMask &= ~(TARGET_FLAG_GAMEOBJECT | TARGET_FLAG_GAMEOBJECT_ITEM);
    if (providedTargetMask & TARGET_FLAG_ITEM)
        effImplicitTargetMask &= ~(TARGET_FLAG_ITEM | TARGET_FLAG_GAMEOBJECT_ITEM);
    if (dstSet || providedTargetMask & TARGET_FLAG_DEST_LOCATION)
        effImplicitTargetMask &= ~(TARGET_FLAG_DEST_LOCATION);
    if (srcSet || providedTargetMask & TARGET_FLAG_SOURCE_LOCATION)
        effImplicitTargetMask &= ~(TARGET_FLAG_SOURCE_LOCATION);

    return effImplicitTargetMask;
}

SpellEffectImplicitTargetTypes SpellEffectInfo::GetImplicitTargetType() const
{
    return _data[Effect].ImplicitTargetType;
}

SpellTargetObjectTypes SpellEffectInfo::GetUsedTargetObjectType() const
{
    return _data[Effect].UsedTargetObjectType;
}

SpellEffectInfo::StaticData  SpellEffectInfo::_data[TOTAL_SPELL_EFFECTS] =
{
    // implicit target type           used target object type
    { EFFECT_IMPLICIT_TARGET_NONE,     TARGET_OBJECT_TYPE_NONE }, // 0
    { EFFECT_IMPLICIT_TARGET_EXPLICIT, TARGET_OBJECT_TYPE_UNIT }, // 1 SPELL_EFFECT_INSTAKILL
    { EFFECT_IMPLICIT_TARGET_EXPLICIT, TARGET_OBJECT_TYPE_UNIT }, // 2 SPELL_EFFECT_SCHOOL_DAMAGE
    { EFFECT_IMPLICIT_TARGET_NONE,     TARGET_OBJECT_TYPE_NONE }, // 3 SPELL_EFFECT_DUMMY
    { EFFECT_IMPLICIT_TARGET_NONE,     TARGET_OBJECT_TYPE_NONE }, // 4 SPELL_EFFECT_PORTAL_TELEPORT
    { EFFECT_IMPLICIT_TARGET_EXPLICIT, TARGET_OBJECT_TYPE_UNIT_AND_DEST }, // 5 SPELL_EFFECT_TELEPORT_UNITS
    { EFFECT_IMPLICIT_TARGET_EXPLICIT, TARGET_OBJECT_TYPE_UNIT }, // 6 SPELL_EFFECT_APPLY_AURA
    { EFFECT_IMPLICIT_TARGET_EXPLICIT, TARGET_OBJECT_TYPE_UNIT }, // 7 SPELL_EFFECT_ENVIRONMENTAL_DAMAGE
    { EFFECT_IMPLICIT_TARGET_EXPLICIT, TARGET_OBJECT_TYPE_UNIT }, // 8 SPELL_EFFECT_POWER_DRAIN
    { EFFECT_IMPLICIT_TARGET_EXPLICIT, TARGET_OBJECT_TYPE_UNIT }, // 9 SPELL_EFFECT_HEALTH_LEECH
    { EFFECT_IMPLICIT_TARGET_EXPLICIT, TARGET_OBJECT_TYPE_UNIT }, // 10 SPELL_EFFECT_HEAL
    { EFFECT_IMPLICIT_TARGET_EXPLICIT, TARGET_OBJECT_TYPE_UNIT }, // 11 SPELL_EFFECT_BIND
    { EFFECT_IMPLICIT_TARGET_NONE,     TARGET_OBJECT_TYPE_NONE }, // 12 SPELL_EFFECT_PORTAL
    { EFFECT_IMPLICIT_TARGET_NONE,     TARGET_OBJECT_TYPE_NONE }, // 13 SPELL_EFFECT_RITUAL_BASE
    { EFFECT_IMPLICIT_TARGET_NONE,     TARGET_OBJECT_TYPE_NONE }, // 14 SPELL_EFFECT_RITUAL_SPECIALIZE
    { EFFECT_IMPLICIT_TARGET_NONE,     TARGET_OBJECT_TYPE_NONE }, // 15 SPELL_EFFECT_RITUAL_ACTIVATE_PORTAL
    { EFFECT_IMPLICIT_TARGET_EXPLICIT, TARGET_OBJECT_TYPE_UNIT }, // 16 SPELL_EFFECT_QUEST_COMPLETE
    { EFFECT_IMPLICIT_TARGET_EXPLICIT, TARGET_OBJECT_TYPE_UNIT }, // 17 SPELL_EFFECT_WEAPON_DAMAGE_NOSCHOOL
    { EFFECT_IMPLICIT_TARGET_EXPLICIT, TARGET_OBJECT_TYPE_CORPSE_ALLY }, // 18 SPELL_EFFECT_RESURRECT
    { EFFECT_IMPLICIT_TARGET_EXPLICIT, TARGET_OBJECT_TYPE_UNIT }, // 19 SPELL_EFFECT_ADD_EXTRA_ATTACKS
    { EFFECT_IMPLICIT_TARGET_NONE,     TARGET_OBJECT_TYPE_UNIT }, // 20 SPELL_EFFECT_DODGE
    { EFFECT_IMPLICIT_TARGET_NONE,     TARGET_OBJECT_TYPE_UNIT }, // 21 SPELL_EFFECT_EVADE
    { EFFECT_IMPLICIT_TARGET_NONE,     TARGET_OBJECT_TYPE_UNIT }, // 22 SPELL_EFFECT_PARRY
    { EFFECT_IMPLICIT_TARGET_NONE,     TARGET_OBJECT_TYPE_UNIT }, // 23 SPELL_EFFECT_BLOCK
    { EFFECT_IMPLICIT_TARGET_EXPLICIT, TARGET_OBJECT_TYPE_UNIT }, // 24 SPELL_EFFECT_CREATE_ITEM
    { EFFECT_IMPLICIT_TARGET_NONE,     TARGET_OBJECT_TYPE_UNIT }, // 25 SPELL_EFFECT_WEAPON
    { EFFECT_IMPLICIT_TARGET_NONE,     TARGET_OBJECT_TYPE_UNIT }, // 26 SPELL_EFFECT_DEFENSE
    { EFFECT_IMPLICIT_TARGET_EXPLICIT, TARGET_OBJECT_TYPE_DEST }, // 27 SPELL_EFFECT_PERSISTENT_AREA_AURA
    { EFFECT_IMPLICIT_TARGET_EXPLICIT, TARGET_OBJECT_TYPE_DEST }, // 28 SPELL_EFFECT_SUMMON
    { EFFECT_IMPLICIT_TARGET_EXPLICIT, TARGET_OBJECT_TYPE_UNIT_AND_DEST }, // 29 SPELL_EFFECT_LEAP
    { EFFECT_IMPLICIT_TARGET_NONE,     TARGET_OBJECT_TYPE_UNIT }, // 30 SPELL_EFFECT_ENERGIZE
    { EFFECT_IMPLICIT_TARGET_EXPLICIT, TARGET_OBJECT_TYPE_UNIT }, // 31 SPELL_EFFECT_WEAPON_PERCENT_DAMAGE
    { EFFECT_IMPLICIT_TARGET_NONE,     TARGET_OBJECT_TYPE_NONE }, // 32 SPELL_EFFECT_TRIGGER_MISSILE
    { EFFECT_IMPLICIT_TARGET_EXPLICIT, TARGET_OBJECT_TYPE_GOBJ_ITEM }, // 33 SPELL_EFFECT_OPEN_LOCK
    { EFFECT_IMPLICIT_TARGET_NONE,     TARGET_OBJECT_TYPE_UNIT }, // 34 SPELL_EFFECT_SUMMON_CHANGE_ITEM
    { EFFECT_IMPLICIT_TARGET_EXPLICIT, TARGET_OBJECT_TYPE_UNIT }, // 35 SPELL_EFFECT_APPLY_AREA_AURA_PARTY
    { EFFECT_IMPLICIT_TARGET_EXPLICIT, TARGET_OBJECT_TYPE_UNIT }, // 36 SPELL_EFFECT_LEARN_SPELL
    { EFFECT_IMPLICIT_TARGET_NONE,     TARGET_OBJECT_TYPE_UNIT }, // 37 SPELL_EFFECT_SPELL_DEFENSE
    { EFFECT_IMPLICIT_TARGET_EXPLICIT, TARGET_OBJECT_TYPE_UNIT }, // 38 SPELL_EFFECT_DISPEL
    { EFFECT_IMPLICIT_TARGET_NONE,     TARGET_OBJECT_TYPE_UNIT }, // 39 SPELL_EFFECT_LANGUAGE
    { EFFECT_IMPLICIT_TARGET_EXPLICIT, TARGET_OBJECT_TYPE_UNIT }, // 40 SPELL_EFFECT_DUAL_WIELD
#ifdef LICH_KING
    { EFFECT_IMPLICIT_TARGET_EXPLICIT, TARGET_OBJECT_TYPE_UNIT }, // 41 SPELL_EFFECT_JUMP
    { EFFECT_IMPLICIT_TARGET_NONE,     TARGET_OBJECT_TYPE_DEST }, // 42 SPELL_EFFECT_JUMP_DEST
#else
    { EFFECT_IMPLICIT_TARGET_NONE,     TARGET_OBJECT_TYPE_NONE }, // 41 SPELL_EFFECT_SUMMON_WILD
    { EFFECT_IMPLICIT_TARGET_NONE,     TARGET_OBJECT_TYPE_NONE }, // 42 SPELL_EFFECT_SUMMON_GUARDIAN
#endif
    { EFFECT_IMPLICIT_TARGET_EXPLICIT, TARGET_OBJECT_TYPE_UNIT_AND_DEST }, // 43 SPELL_EFFECT_TELEPORT_UNITS_FACE_CASTER
    { EFFECT_IMPLICIT_TARGET_EXPLICIT, TARGET_OBJECT_TYPE_UNIT }, // 44 SPELL_EFFECT_SKILL_STEP
    { EFFECT_IMPLICIT_TARGET_EXPLICIT, TARGET_OBJECT_TYPE_UNIT }, // 45 SPELL_EFFECT_ADD_HONOR
    { EFFECT_IMPLICIT_TARGET_NONE,     TARGET_OBJECT_TYPE_UNIT }, // 46 SPELL_EFFECT_SPAWN
    { EFFECT_IMPLICIT_TARGET_NONE,     TARGET_OBJECT_TYPE_UNIT }, // 47 SPELL_EFFECT_TRADE_SKILL
    { EFFECT_IMPLICIT_TARGET_NONE,     TARGET_OBJECT_TYPE_UNIT }, // 48 SPELL_EFFECT_STEALTH
    { EFFECT_IMPLICIT_TARGET_NONE,     TARGET_OBJECT_TYPE_UNIT }, // 49 SPELL_EFFECT_DETECT
    { EFFECT_IMPLICIT_TARGET_EXPLICIT, TARGET_OBJECT_TYPE_DEST }, // 50 SPELL_EFFECT_TRANS_DOOR
    { EFFECT_IMPLICIT_TARGET_NONE,     TARGET_OBJECT_TYPE_UNIT }, // 51 SPELL_EFFECT_FORCE_CRITICAL_HIT
    { EFFECT_IMPLICIT_TARGET_NONE,     TARGET_OBJECT_TYPE_UNIT }, // 52 SPELL_EFFECT_GUARANTEE_HIT
    { EFFECT_IMPLICIT_TARGET_EXPLICIT, TARGET_OBJECT_TYPE_ITEM }, // 53 SPELL_EFFECT_ENCHANT_ITEM
    { EFFECT_IMPLICIT_TARGET_EXPLICIT, TARGET_OBJECT_TYPE_ITEM }, // 54 SPELL_EFFECT_ENCHANT_ITEM_TEMPORARY
    { EFFECT_IMPLICIT_TARGET_EXPLICIT, TARGET_OBJECT_TYPE_UNIT }, // 55 SPELL_EFFECT_TAMECREATURE
    { EFFECT_IMPLICIT_TARGET_EXPLICIT, TARGET_OBJECT_TYPE_DEST }, // 56 SPELL_EFFECT_SUMMON_PET
    { EFFECT_IMPLICIT_TARGET_EXPLICIT, TARGET_OBJECT_TYPE_UNIT }, // 57 SPELL_EFFECT_LEARN_PET_SPELL
    { EFFECT_IMPLICIT_TARGET_EXPLICIT, TARGET_OBJECT_TYPE_UNIT }, // 58 SPELL_EFFECT_WEAPON_DAMAGE
#ifdef LICH_KING
    { EFFECT_IMPLICIT_TARGET_EXPLICIT, TARGET_OBJECT_TYPE_UNIT }, // 59 SPELL_EFFECT_CREATE_RANDOM_ITEM
#else
    { EFFECT_IMPLICIT_TARGET_EXPLICIT, TARGET_OBJECT_TYPE_ITEM }, // 59 SPELL_EFFECT_OPEN_LOCK_ITEM
#endif
    { EFFECT_IMPLICIT_TARGET_NONE,     TARGET_OBJECT_TYPE_UNIT }, // 60 SPELL_EFFECT_PROFICIENCY
    { EFFECT_IMPLICIT_TARGET_NONE,     TARGET_OBJECT_TYPE_NONE }, // 61 SPELL_EFFECT_SEND_EVENT
    { EFFECT_IMPLICIT_TARGET_EXPLICIT, TARGET_OBJECT_TYPE_UNIT }, // 62 SPELL_EFFECT_POWER_BURN
    { EFFECT_IMPLICIT_TARGET_EXPLICIT, TARGET_OBJECT_TYPE_UNIT }, // 63 SPELL_EFFECT_THREAT
    { EFFECT_IMPLICIT_TARGET_NONE,     TARGET_OBJECT_TYPE_NONE }, // 64 SPELL_EFFECT_TRIGGER_SPELL
#ifdef LICH_KING
    { EFFECT_IMPLICIT_TARGET_EXPLICIT, TARGET_OBJECT_TYPE_UNIT }, // 65 SPELL_EFFECT_APPLY_AREA_AURA_RAID
    { EFFECT_IMPLICIT_TARGET_EXPLICIT, TARGET_OBJECT_TYPE_UNIT }, // 66 SPELL_EFFECT_CREATE_MANA_GEM
#else
    { EFFECT_IMPLICIT_TARGET_EXPLICIT, TARGET_OBJECT_TYPE_UNIT }, // 66 SPELL_EFFECT_HEALTH_FUNNEL
    { EFFECT_IMPLICIT_TARGET_EXPLICIT, TARGET_OBJECT_TYPE_UNIT }, // 66 SPELL_EFFECT_POWER_FUNNEL
#endif
    { EFFECT_IMPLICIT_TARGET_EXPLICIT, TARGET_OBJECT_TYPE_UNIT }, // 67 SPELL_EFFECT_HEAL_MAX_HEALTH
    { EFFECT_IMPLICIT_TARGET_EXPLICIT, TARGET_OBJECT_TYPE_UNIT }, // 68 SPELL_EFFECT_INTERRUPT_CAST
    { EFFECT_IMPLICIT_TARGET_EXPLICIT, TARGET_OBJECT_TYPE_UNIT_AND_DEST }, // 69 SPELL_EFFECT_DISTRACT
    { EFFECT_IMPLICIT_TARGET_EXPLICIT, TARGET_OBJECT_TYPE_UNIT }, // 70 SPELL_EFFECT_PULL
    { EFFECT_IMPLICIT_TARGET_EXPLICIT, TARGET_OBJECT_TYPE_UNIT }, // 71 SPELL_EFFECT_PICKPOCKET
    { EFFECT_IMPLICIT_TARGET_EXPLICIT, TARGET_OBJECT_TYPE_DEST }, // 72 SPELL_EFFECT_ADD_FARSIGHT
#ifdef LICH_KING
    { EFFECT_IMPLICIT_TARGET_EXPLICIT, TARGET_OBJECT_TYPE_UNIT }, // 73 SPELL_EFFECT_UNTRAIN_TALENTS
    { EFFECT_IMPLICIT_TARGET_NONE,     TARGET_OBJECT_TYPE_UNIT }, // 74 SPELL_EFFECT_APPLY_GLYPH
#else
    { EFFECT_IMPLICIT_TARGET_EXPLICIT,     TARGET_OBJECT_TYPE_DEST }, // 73 SPELL_EFFECT_SUMMON_POSSESSED
    { EFFECT_IMPLICIT_TARGET_EXPLICIT,     TARGET_OBJECT_TYPE_DEST }, // 74 SPELL_EFFECT_SUMMON_TOTEM
#endif
    { EFFECT_IMPLICIT_TARGET_EXPLICIT, TARGET_OBJECT_TYPE_UNIT }, // 75 SPELL_EFFECT_HEAL_MECHANICAL
    { EFFECT_IMPLICIT_TARGET_EXPLICIT, TARGET_OBJECT_TYPE_DEST }, // 76 SPELL_EFFECT_SUMMON_OBJECT_WILD
    { EFFECT_IMPLICIT_TARGET_NONE,     TARGET_OBJECT_TYPE_NONE }, // 77 SPELL_EFFECT_SCRIPT_EFFECT
    { EFFECT_IMPLICIT_TARGET_NONE,     TARGET_OBJECT_TYPE_UNIT }, // 78 SPELL_EFFECT_ATTACK
    { EFFECT_IMPLICIT_TARGET_NONE,     TARGET_OBJECT_TYPE_UNIT }, // 79 SPELL_EFFECT_SANCTUARY
    { EFFECT_IMPLICIT_TARGET_EXPLICIT, TARGET_OBJECT_TYPE_UNIT }, // 80 SPELL_EFFECT_ADD_COMBO_POINTS
    { EFFECT_IMPLICIT_TARGET_EXPLICIT, TARGET_OBJECT_TYPE_DEST }, // 81 SPELL_EFFECT_CREATE_HOUSE
    { EFFECT_IMPLICIT_TARGET_EXPLICIT, TARGET_OBJECT_TYPE_UNIT }, // 82 SPELL_EFFECT_BIND_SIGHT
    { EFFECT_IMPLICIT_TARGET_EXPLICIT, TARGET_OBJECT_TYPE_UNIT_AND_DEST }, // 83 SPELL_EFFECT_DUEL
    { EFFECT_IMPLICIT_TARGET_NONE,     TARGET_OBJECT_TYPE_UNIT }, // 84 SPELL_EFFECT_STUCK
    { EFFECT_IMPLICIT_TARGET_NONE,     TARGET_OBJECT_TYPE_NONE }, // 85 SPELL_EFFECT_SUMMON_PLAYER
    { EFFECT_IMPLICIT_TARGET_EXPLICIT, TARGET_OBJECT_TYPE_GOBJ }, // 86 SPELL_EFFECT_ACTIVATE_OBJECT
#ifdef LICH_KING
    { EFFECT_IMPLICIT_TARGET_EXPLICIT, TARGET_OBJECT_TYPE_GOBJ }, // 87 SPELL_EFFECT_GAMEOBJECT_DAMAGE
    { EFFECT_IMPLICIT_TARGET_EXPLICIT, TARGET_OBJECT_TYPE_GOBJ }, // 88 SPELL_EFFECT_GAMEOBJECT_REPAIR
    { EFFECT_IMPLICIT_TARGET_EXPLICIT, TARGET_OBJECT_TYPE_GOBJ }, // 89 SPELL_EFFECT_GAMEOBJECT_SET_DESTRUCTION_STATE
    { EFFECT_IMPLICIT_TARGET_EXPLICIT, TARGET_OBJECT_TYPE_UNIT }, // 90 SPELL_EFFECT_KILL_CREDIT
#else
    { EFFECT_IMPLICIT_TARGET_EXPLICIT, TARGET_OBJECT_TYPE_DEST }, // 87 SPELL_EFFECT_SUMMON_TOTEM_SLOT1
    { EFFECT_IMPLICIT_TARGET_EXPLICIT, TARGET_OBJECT_TYPE_DEST }, // 88 SPELL_EFFECT_SUMMON_TOTEM_SLOT2
    { EFFECT_IMPLICIT_TARGET_EXPLICIT, TARGET_OBJECT_TYPE_DEST }, // 89 SPELL_EFFECT_SUMMON_TOTEM_SLOT3
    { EFFECT_IMPLICIT_TARGET_EXPLICIT, TARGET_OBJECT_TYPE_DEST }, // 90 SPELL_EFFECT_SUMMON_TOTEM_SLOT4
#endif
    { EFFECT_IMPLICIT_TARGET_EXPLICIT, TARGET_OBJECT_TYPE_UNIT }, // 91 SPELL_EFFECT_THREAT_ALL
    { EFFECT_IMPLICIT_TARGET_EXPLICIT, TARGET_OBJECT_TYPE_UNIT }, // 92 SPELL_EFFECT_ENCHANT_HELD_ITEM
#ifdef LICH_KING
    { EFFECT_IMPLICIT_TARGET_NONE,     TARGET_OBJECT_TYPE_UNIT }, // 93 SPELL_EFFECT_FORCE_DESELECT
#else
    { EFFECT_IMPLICIT_TARGET_EXPLICIT,  TARGET_OBJECT_TYPE_DEST }, // 93 SPELL_EFFECT_SUMMON_PHANTASM
#endif
    { EFFECT_IMPLICIT_TARGET_NONE,     TARGET_OBJECT_TYPE_UNIT }, // 94 SPELL_EFFECT_SELF_RESURRECT
    { EFFECT_IMPLICIT_TARGET_EXPLICIT, TARGET_OBJECT_TYPE_UNIT }, // 95 SPELL_EFFECT_SKINNING
    { EFFECT_IMPLICIT_TARGET_EXPLICIT, TARGET_OBJECT_TYPE_UNIT }, // 96 SPELL_EFFECT_CHARGE
#ifdef LICH_KING
    { EFFECT_IMPLICIT_TARGET_NONE,     TARGET_OBJECT_TYPE_UNIT }, // 97 SPELL_EFFECT_CAST_BUTTON
#else
    { EFFECT_IMPLICIT_TARGET_EXPLICIT, TARGET_OBJECT_TYPE_DEST }, // 97 SPELL_EFFECT_SUMMON_CRITTER
#endif
    { EFFECT_IMPLICIT_TARGET_EXPLICIT, TARGET_OBJECT_TYPE_UNIT }, // 98 SPELL_EFFECT_KNOCK_BACK
    { EFFECT_IMPLICIT_TARGET_EXPLICIT, TARGET_OBJECT_TYPE_ITEM }, // 99 SPELL_EFFECT_DISENCHANT
    { EFFECT_IMPLICIT_TARGET_EXPLICIT, TARGET_OBJECT_TYPE_UNIT }, // 100 SPELL_EFFECT_INEBRIATE
    { EFFECT_IMPLICIT_TARGET_EXPLICIT, TARGET_OBJECT_TYPE_ITEM }, // 101 SPELL_EFFECT_FEED_PET
    { EFFECT_IMPLICIT_TARGET_EXPLICIT, TARGET_OBJECT_TYPE_UNIT }, // 102 SPELL_EFFECT_DISMISS_PET
    { EFFECT_IMPLICIT_TARGET_EXPLICIT, TARGET_OBJECT_TYPE_UNIT }, // 103 SPELL_EFFECT_REPUTATION
    { EFFECT_IMPLICIT_TARGET_EXPLICIT, TARGET_OBJECT_TYPE_DEST }, // 104 SPELL_EFFECT_SUMMON_OBJECT_SLOT1
    { EFFECT_IMPLICIT_TARGET_EXPLICIT, TARGET_OBJECT_TYPE_DEST }, // 105 SPELL_EFFECT_SUMMON_OBJECT_SLOT2
    { EFFECT_IMPLICIT_TARGET_EXPLICIT, TARGET_OBJECT_TYPE_DEST }, // 106 SPELL_EFFECT_SUMMON_OBJECT_SLOT3
    { EFFECT_IMPLICIT_TARGET_EXPLICIT, TARGET_OBJECT_TYPE_DEST }, // 107 SPELL_EFFECT_SUMMON_OBJECT_SLOT4
    { EFFECT_IMPLICIT_TARGET_EXPLICIT, TARGET_OBJECT_TYPE_UNIT }, // 108 SPELL_EFFECT_DISPEL_MECHANIC
#ifdef LICH_KING
    { EFFECT_IMPLICIT_TARGET_EXPLICIT, TARGET_OBJECT_TYPE_DEST }, // 109 SPELL_EFFECT_RESURRECT_PET
#else
    { EFFECT_IMPLICIT_TARGET_EXPLICIT, TARGET_OBJECT_TYPE_DEST }, // 109 SPELL_EFFECT_RESURRECT_PET
#endif
    { EFFECT_IMPLICIT_TARGET_NONE,     TARGET_OBJECT_TYPE_UNIT }, // 110 SPELL_EFFECT_DESTROY_ALL_TOTEMS
    { EFFECT_IMPLICIT_TARGET_EXPLICIT, TARGET_OBJECT_TYPE_UNIT }, // 111 SPELL_EFFECT_DURABILITY_DAMAGE
#ifdef LICH_KING
    { EFFECT_IMPLICIT_TARGET_EXPLICIT, TARGET_OBJECT_TYPE_UNIT }, // 112 SPELL_EFFECT_112
#else
    { EFFECT_IMPLICIT_TARGET_EXPLICIT, TARGET_OBJECT_TYPE_DEST }, // 112 SPELL_EFFECT_SUMMON_DEMON

#endif
    { EFFECT_IMPLICIT_TARGET_EXPLICIT, TARGET_OBJECT_TYPE_CORPSE_ALLY }, // 113 SPELL_EFFECT_RESURRECT_NEW
    { EFFECT_IMPLICIT_TARGET_EXPLICIT, TARGET_OBJECT_TYPE_UNIT }, // 114 SPELL_EFFECT_ATTACK_ME
    { EFFECT_IMPLICIT_TARGET_EXPLICIT, TARGET_OBJECT_TYPE_UNIT }, // 115 SPELL_EFFECT_DURABILITY_DAMAGE_PCT
    { EFFECT_IMPLICIT_TARGET_EXPLICIT, TARGET_OBJECT_TYPE_CORPSE_ENEMY }, // 116 SPELL_EFFECT_SKIN_PLAYER_CORPSE
    { EFFECT_IMPLICIT_TARGET_EXPLICIT, TARGET_OBJECT_TYPE_UNIT }, // 117 SPELL_EFFECT_SPIRIT_HEAL
    { EFFECT_IMPLICIT_TARGET_NONE,     TARGET_OBJECT_TYPE_UNIT }, // 118 SPELL_EFFECT_SKILL
    { EFFECT_IMPLICIT_TARGET_EXPLICIT, TARGET_OBJECT_TYPE_UNIT }, // 119 SPELL_EFFECT_APPLY_AREA_AURA_PET
    { EFFECT_IMPLICIT_TARGET_EXPLICIT, TARGET_OBJECT_TYPE_UNIT }, // 120 SPELL_EFFECT_TELEPORT_GRAVEYARD
    { EFFECT_IMPLICIT_TARGET_EXPLICIT, TARGET_OBJECT_TYPE_UNIT }, // 121 SPELL_EFFECT_NORMALIZED_WEAPON_DMG
    { EFFECT_IMPLICIT_TARGET_NONE,     TARGET_OBJECT_TYPE_NONE }, // 122 SPELL_EFFECT_122
    { EFFECT_IMPLICIT_TARGET_EXPLICIT, TARGET_OBJECT_TYPE_UNIT }, // 123 SPELL_EFFECT_SEND_TAXI
    { EFFECT_IMPLICIT_TARGET_EXPLICIT, TARGET_OBJECT_TYPE_UNIT }, // 124 SPELL_EFFECT_PULL_TOWARDS
    { EFFECT_IMPLICIT_TARGET_EXPLICIT, TARGET_OBJECT_TYPE_UNIT }, // 125 SPELL_EFFECT_MODIFY_THREAT_PERCENT
    { EFFECT_IMPLICIT_TARGET_EXPLICIT, TARGET_OBJECT_TYPE_UNIT }, // 126 SPELL_EFFECT_STEAL_BENEFICIAL_BUFF
    { EFFECT_IMPLICIT_TARGET_EXPLICIT, TARGET_OBJECT_TYPE_ITEM }, // 127 SPELL_EFFECT_PROSPECTING
    { EFFECT_IMPLICIT_TARGET_EXPLICIT, TARGET_OBJECT_TYPE_UNIT }, // 128 SPELL_EFFECT_APPLY_AREA_AURA_FRIEND
    { EFFECT_IMPLICIT_TARGET_EXPLICIT, TARGET_OBJECT_TYPE_UNIT }, // 129 SPELL_EFFECT_APPLY_AREA_AURA_ENEMY
    { EFFECT_IMPLICIT_TARGET_EXPLICIT, TARGET_OBJECT_TYPE_UNIT }, // 130 SPELL_EFFECT_REDIRECT_THREAT
    { EFFECT_IMPLICIT_TARGET_NONE,     TARGET_OBJECT_TYPE_UNIT }, // 131 SPELL_EFFECT_PLAY_SOUND // unused SPELL_EFFECT_131 on BC
    { EFFECT_IMPLICIT_TARGET_EXPLICIT, TARGET_OBJECT_TYPE_UNIT }, // 132 SPELL_EFFECT_PLAY_MUSIC
    { EFFECT_IMPLICIT_TARGET_EXPLICIT, TARGET_OBJECT_TYPE_UNIT }, // 133 SPELL_EFFECT_UNLEARN_SPECIALIZATION
    { EFFECT_IMPLICIT_TARGET_NONE,     TARGET_OBJECT_TYPE_UNIT }, // 134 SPELL_EFFECT_KILL_CREDIT2
    { EFFECT_IMPLICIT_TARGET_EXPLICIT, TARGET_OBJECT_TYPE_DEST }, // 135 SPELL_EFFECT_CALL_PET
    { EFFECT_IMPLICIT_TARGET_EXPLICIT, TARGET_OBJECT_TYPE_UNIT }, // 136 SPELL_EFFECT_HEAL_PCT
    { EFFECT_IMPLICIT_TARGET_EXPLICIT, TARGET_OBJECT_TYPE_UNIT }, // 137 SPELL_EFFECT_ENERGIZE_PCT
    { EFFECT_IMPLICIT_TARGET_EXPLICIT, TARGET_OBJECT_TYPE_UNIT }, // 138 SPELL_EFFECT_LEAP_BACK //unused on BC
    { EFFECT_IMPLICIT_TARGET_EXPLICIT, TARGET_OBJECT_TYPE_UNIT }, // 139 SPELL_EFFECT_CLEAR_QUEST //unused on BC
    { EFFECT_IMPLICIT_TARGET_EXPLICIT, TARGET_OBJECT_TYPE_UNIT }, // 140 SPELL_EFFECT_FORCE_CAST
    { EFFECT_IMPLICIT_TARGET_EXPLICIT, TARGET_OBJECT_TYPE_UNIT }, // 141 SPELL_EFFECT_FORCE_CAST_WITH_VALUE
    { EFFECT_IMPLICIT_TARGET_EXPLICIT, TARGET_OBJECT_TYPE_UNIT }, // 142 SPELL_EFFECT_TRIGGER_SPELL_WITH_VALUE
    { EFFECT_IMPLICIT_TARGET_EXPLICIT, TARGET_OBJECT_TYPE_UNIT }, // 143 SPELL_EFFECT_APPLY_AREA_AURA_OWNER
    { EFFECT_IMPLICIT_TARGET_EXPLICIT, TARGET_OBJECT_TYPE_UNIT_AND_DEST }, // 144 SPELL_EFFECT_KNOCK_BACK_DEST
    { EFFECT_IMPLICIT_TARGET_EXPLICIT, TARGET_OBJECT_TYPE_UNIT_AND_DEST }, // 145 SPELL_EFFECT_PULL_TOWARDS_DEST //unused on BC
    { EFFECT_IMPLICIT_TARGET_EXPLICIT, TARGET_OBJECT_TYPE_UNIT }, // 146 SPELL_EFFECT_ACTIVATE_RUNE //unused on BC
    { EFFECT_IMPLICIT_TARGET_EXPLICIT, TARGET_OBJECT_TYPE_UNIT }, // 147 SPELL_EFFECT_QUEST_FAIL
    { EFFECT_IMPLICIT_TARGET_NONE,     TARGET_OBJECT_TYPE_NONE }, // 148 SPELL_EFFECT_TRIGGER_MISSILE_SPELL_WITH_VALUE //unused on BC
    { EFFECT_IMPLICIT_TARGET_EXPLICIT, TARGET_OBJECT_TYPE_DEST }, // 149 SPELL_EFFECT_CHARGE_DEST //unused on BC
    { EFFECT_IMPLICIT_TARGET_EXPLICIT, TARGET_OBJECT_TYPE_UNIT }, // 150 SPELL_EFFECT_QUEST_START // unused on BC
    { EFFECT_IMPLICIT_TARGET_NONE,     TARGET_OBJECT_TYPE_NONE }, // 151 SPELL_EFFECT_TRIGGER_SPELL_2
    { EFFECT_IMPLICIT_TARGET_NONE,     TARGET_OBJECT_TYPE_NONE }, // 152 SPELL_EFFECT_SUMMON_RAF_FRIEND // unused on BC
    { EFFECT_IMPLICIT_TARGET_EXPLICIT, TARGET_OBJECT_TYPE_UNIT }, // 153 SPELL_EFFECT_CREATE_TAMED_PET
#ifdef LICH_KING
    { EFFECT_IMPLICIT_TARGET_EXPLICIT, TARGET_OBJECT_TYPE_UNIT }, // 154 SPELL_EFFECT_DISCOVER_TAXI
    { EFFECT_IMPLICIT_TARGET_NONE,     TARGET_OBJECT_TYPE_UNIT }, // 155 SPELL_EFFECT_TITAN_GRIP
    { EFFECT_IMPLICIT_TARGET_EXPLICIT, TARGET_OBJECT_TYPE_ITEM }, // 156 SPELL_EFFECT_ENCHANT_ITEM_PRISMATIC
    { EFFECT_IMPLICIT_TARGET_EXPLICIT, TARGET_OBJECT_TYPE_UNIT }, // 157 SPELL_EFFECT_CREATE_ITEM_2
    { EFFECT_IMPLICIT_TARGET_EXPLICIT, TARGET_OBJECT_TYPE_ITEM }, // 158 SPELL_EFFECT_MILLING
    { EFFECT_IMPLICIT_TARGET_EXPLICIT, TARGET_OBJECT_TYPE_UNIT }, // 159 SPELL_EFFECT_ALLOW_RENAME_PET
    { EFFECT_IMPLICIT_TARGET_EXPLICIT, TARGET_OBJECT_TYPE_UNIT }, // 160 SPELL_EFFECT_160
    { EFFECT_IMPLICIT_TARGET_EXPLICIT, TARGET_OBJECT_TYPE_UNIT }, // 161 SPELL_EFFECT_TALENT_SPEC_COUNT
    { EFFECT_IMPLICIT_TARGET_EXPLICIT, TARGET_OBJECT_TYPE_UNIT }, // 162 SPELL_EFFECT_TALENT_SPEC_SELECT
    { EFFECT_IMPLICIT_TARGET_EXPLICIT, TARGET_OBJECT_TYPE_UNIT }, // 163 SPELL_EFFECT_163
    { EFFECT_IMPLICIT_TARGET_EXPLICIT, TARGET_OBJECT_TYPE_UNIT }, // 164 SPELL_EFFECT_REMOVE_AURA
#endif
};

bool SpellInfo::IsSingleTarget() const
{
    // all other single target spells have if it has AttributesEx5
    if (HasAttribute(SPELL_ATTR5_SINGLE_TARGET_SPELL))
        return true;

    // TODO - need find Judgements rule
    switch (GetSpellSpecific())
    {
    case SPELL_SPECIFIC_JUDGEMENT:
        return true;
    default:
        break;
    }

    // single target triggered spell.
    // Not real client side single target spell, but it' not triggered until prev. aura expired.
    // This is allow store it in single target spells list for caster for spell proc checking
    if (Id == 38324)                                // Regeneration (triggered by 38299 (HoTs on Heals))
        return true;

    return false;
}

bool SpellInfo::IsAuraExclusiveBySpecificWith(SpellInfo const* spellInfo) const
{
    SpellSpecificType spellSpec1 = GetSpellSpecific();
    SpellSpecificType spellSpec2 = spellInfo->GetSpellSpecific();
    switch (spellSpec1)
    {
    case SPELL_SPECIFIC_TRACKER:
    case SPELL_SPECIFIC_WARLOCK_ARMOR:
    case SPELL_SPECIFIC_MAGE_ARMOR:
    case SPELL_SPECIFIC_ELEMENTAL_SHIELD:
    case SPELL_SPECIFIC_MAGE_POLYMORPH:
#ifdef LICH_KING
    case SPELL_SPECIFIC_PRESENCE:
#endif
    case SPELL_SPECIFIC_CHARM:
    case SPELL_SPECIFIC_SCROLL:
    case SPELL_SPECIFIC_WARRIOR_ENRAGE:
    case SPELL_SPECIFIC_MAGE_ARCANE_BRILLANCE:
    case SPELL_SPECIFIC_PRIEST_DIVINE_SPIRIT:
    case SPELL_SPECIFIC_DRUID_MANGLE:
    case SPELL_SPECIFIC_WELL_FED:
        return spellSpec1 == spellSpec2;
    case SPELL_SPECIFIC_FOOD:
        return spellSpec2 == SPELL_SPECIFIC_FOOD
            || spellSpec2 == SPELL_SPECIFIC_FOOD_AND_DRINK;
    case SPELL_SPECIFIC_DRINK:
        return spellSpec2 == SPELL_SPECIFIC_DRINK
            || spellSpec2 == SPELL_SPECIFIC_FOOD_AND_DRINK;
    case SPELL_SPECIFIC_FOOD_AND_DRINK:
        return spellSpec2 == SPELL_SPECIFIC_FOOD
            || spellSpec2 == SPELL_SPECIFIC_DRINK
            || spellSpec2 == SPELL_SPECIFIC_FOOD_AND_DRINK;
    default:
        return false;
    }
}

bool SpellInfo::IsAuraExclusiveBySpecificPerCasterWith(SpellInfo const* spellInfo) const
{
    SpellSpecificType spellSpec = GetSpellSpecific();
    switch (spellSpec)
    {
    case SPELL_SPECIFIC_SEAL:
    //TC case SPELL_SPECIFIC_HAND:
    case SPELL_SPECIFIC_BLESSING:
    case SPELL_SPECIFIC_AURA:
    case SPELL_SPECIFIC_STING:
    case SPELL_SPECIFIC_CURSE:
    case SPELL_SPECIFIC_ASPECT:
    case SPELL_SPECIFIC_POSITIVE_SHOUT:
    case SPELL_SPECIFIC_JUDGEMENT:
    case SPELL_SPECIFIC_WARLOCK_CORRUPTION:
        return spellSpec == spellInfo->GetSpellSpecific();
    default:
        return false;
    }
}

uint32 SpellInfo::_GetExplicitTargetMask() const
{
    bool srcSet = false;
    bool dstSet = false;
    uint32 targetMask = Targets;
    // prepare target mask using effect target entries
    for (const auto & Effect : Effects)
    {
        if (!Effect.IsEffect())
            continue;
        targetMask |= Effect.TargetA.GetExplicitTargetMask(srcSet, dstSet);
        targetMask |= Effect.TargetB.GetExplicitTargetMask(srcSet, dstSet);

        // add explicit target flags based on spell effects which have EFFECT_IMPLICIT_TARGET_EXPLICIT and no valid target provided
        if (Effect.GetImplicitTargetType() != EFFECT_IMPLICIT_TARGET_EXPLICIT)
            continue;

        // extend explicit target mask only if valid targets for effect could not be provided by target types
        uint32 effectTargetMask = Effect.GetMissingTargetMask(srcSet, dstSet, targetMask);

        // don't add explicit object/dest flags when spell has no max range
        if (GetMaxRange(true) == 0.0f && GetMaxRange(false) == 0.0f)
            effectTargetMask &= ~(TARGET_FLAG_UNIT_MASK | TARGET_FLAG_GAMEOBJECT | TARGET_FLAG_CORPSE_MASK | TARGET_FLAG_DEST_LOCATION);
        targetMask |= effectTargetMask;
    }
    return targetMask;
}

bool SpellInfo::_IsPositiveEffect(uint32 effIndex, bool deep) const
{
    // talents
    if (IsPassive() && GetTalentSpellCost(Id))
        return true;

    if (HasAttribute(SPELL_ATTR1_CANT_BE_REFLECTED) //all those should be negative
        || HasAttribute(SPELL_ATTR0_NEGATIVE_1))
        return false;

    switch(Id)
    {
        case 23333:                                         // BG spell
        case 23335:                                         // BG spell
        case 34976:                                         // BG spell
        case 31579:                                         // Arcane Empowerment Rank1 talent aura with one positive and one negative (check not needed in wotlk)
        case 31582:                                         // Arcane Empowerment Rank2
        case 31583:                                         // Arcane Empowerment Rank3
        case 38307:                                         // The Dark of Night
        case 40477:                                         // Forceful Strike
        case 38318:                                         // Transformation - Black Whelp
        case 24732:                                         // Bat Costume
        case 24740:                                         // Wisp Costume
        case 43730:                                         // Electrified
        case 37472:
        case 45989:
        case 20553:
        case 45856:                                         // Breath: Haste
        case 45860:                                         // Breath: Revitalize
        case 45848:                                         // Shield of the Blue
        case 45839:                                         // Vengeance of the Blue Flight
        case 23505:                                         // Berserking (BG buff)
        case 1462:                                          // Beast Lore
        case 30877:                                         // Tag Murloc
            return true;
        case 1852:                                          // Silenced (GM)
        case 46392:                                         // Focused Assault
        case 46393:                                         // Brutal Assault
        case 43437:                                         // Paralyzed
        case 28441:                                         // not positive dummy spell
        case 37675:                                         // Chaos Blast
        case 41519:                                         // Mark of Stormrage
        case 34877:                                         // Custodian of Time
        case 34700:                                         // Allergic Reaction
        case 31719:                                         // Suspension
        case 43501:                                         // Siphon Soul (Malacrass)
        case 30500:                                         // Death Coil (Nethekurse - Shattered Halls)
        case 38065:                                         // Death Coil (Nexus Terror - Mana Tombs)
        case 45661:                                         // Encapsulate (Felmyst - Sunwell Plateau)
        case 45662:
        case 45665:
        case 47002:
        case 41070:                                         // Death Coil (Shadowmoon Deathshaper - Black Temple)
        case 40165:
        case 40055:
        case 40166:
        case 40167:                                         // Introspection
        case 46458:
        case 16097:                                         // Hex
        case 7103:
        case 6945:                                          // Chest Pains
        case 23182:
        case 40695:                                         // Illidan - Caged
        case 37108:                                         // Quest 10557
        case 37966:
        case 30529:
        case 37463:
        case 37461:
        case 37462:
        case 37465:
        case 37453:
        case 37498:
        case 37427:
        case 37406:
        case 27861:
        case 29214: // Wrath of the Plaguebringer
        case 30421: //netherspite spell
        case 30422: //netherspite spell
        case 30423: //netherspite spell
            return false;
    }

    switch (SpellFamilyName)
    {
    case SPELLFAMILY_MAGE:
        // Amplify Magic, Dampen Magic
        if (SpellFamilyFlags == 0x00002000)
            return true;
        // Arcane Missiles
        if (SpellFamilyFlags == 0x00000800)
            return false;
    case SPELLFAMILY_ROGUE:
        switch (Id)
        {
        // Slice and Dice. Prevents breaking Stealth
        case 5171:      // Slice and Dice (Rank 1)
        case 6774:      // Slice and Dice (Rank 2)
            return true;
        default:
            break;
        }
    }

    switch (Mechanic)
    {
    case MECHANIC_IMMUNE_SHIELD:
        return true;
    default:
        break;
    }

    // Special case: effects which determine positivity of whole spell
    for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
    {
        if (Effects[i].IsAura())
        {
            switch (Effects[i].ApplyAuraName)
            {
            case SPELL_AURA_MOD_STEALTH:
                return true;
            case SPELL_AURA_CHANNEL_DEATH_ITEM:
            case SPELL_AURA_EMPATHY:
                return false;
            }
        }
    }

    switch(Effects[effIndex].Effect)
    {
        // Those are positive on friendly, negative on hostile, handled is IsPositive instead since we cannot give a static value here
        /* case SPELL_EFFECT_DISPEL:
        case SPELL_EFFECT_DISPEL_MECHANIC:
            return true; */

        // always positive effects (check before target checks that provided non-positive result in some case for positive effects)
        case SPELL_EFFECT_HEAL:
        case SPELL_EFFECT_LEARN_SPELL:
        case SPELL_EFFECT_SKILL_STEP:
        case SPELL_EFFECT_HEAL_PCT:
        case SPELL_EFFECT_ENERGIZE_PCT:
        case SPELL_EFFECT_PLAY_SOUND:
        case SPELL_EFFECT_PLAY_MUSIC:
            return true;

        case SPELL_EFFECT_APPLY_AREA_AURA_ENEMY:
            return false;
            // non-positive aura use
        case SPELL_EFFECT_APPLY_AURA:
        case SPELL_EFFECT_APPLY_AREA_AURA_FRIEND:
        {
            switch(Effects[effIndex].ApplyAuraName)
            {
                case SPELL_AURA_DUMMY:
                {
                    // dummy aura can be positive or negative dependent from casted spell
                    switch(Id)
                    {
                        case 13139:                         // net-o-matic special effect
                        case 23445:                         // evil twin
                        case 38637:                         // Nether Exhaustion (red)
                        case 38638:                         // Nether Exhaustion (green)
                        case 38639:                         // Nether Exhaustion (blue)
                            return false;
                        default:
                            break;
                    }
                }   break;
                case SPELL_AURA_MOD_DAMAGE_DONE:            // dependent from bas point sign (negative -> negative)
                case SPELL_AURA_MOD_STAT:
                case SPELL_AURA_MOD_SKILL:
                case SPELL_AURA_MOD_DODGE_PERCENT:
                case SPELL_AURA_MOD_HEALING_DONE:
                case SPELL_AURA_MOD_HEALING_PCT:
                case SPELL_AURA_MOD_DAMAGE_PERCENT_DONE:
                {
                    if(Effects[effIndex].BasePoints+int32(Effects[effIndex].BaseDice) < 0)
                        return false;
                    break;
                }
                case SPELL_AURA_MOD_DAMAGE_TAKEN:           // dependent from bas point sign (positive -> negative)
                    if(Effects[effIndex].BasePoints+int32(Effects[effIndex].BaseDice) > 0)
                        return false;
                    break;
                case SPELL_AURA_MOD_SPELL_CRIT_CHANCE:
                    if(Effects[effIndex].BasePoints+int32(Effects[effIndex].BaseDice) > 0)
                        return true;                        // some expected positive spells have SPELL_ATTR1_NEGATIVE
                    break;
                case SPELL_AURA_ADD_TARGET_TRIGGER:
                    return true;
                case SPELL_AURA_PERIODIC_TRIGGER_SPELL_WITH_VALUE:
                case SPELL_AURA_PERIODIC_TRIGGER_SPELL:
                    if (deep)
                    {
                        if(Id != Effects[effIndex].TriggerSpell)
                        {
                            uint32 spellTriggeredId = Effects[effIndex].TriggerSpell;
                            SpellInfo const *spellTriggeredProto = sSpellMgr->GetSpellInfo(spellTriggeredId);

                            if(spellTriggeredProto)
                            {
                                // non-positive targets of main spell return early
                                for(int i = 0; i < MAX_SPELL_EFFECTS; ++i)
                                {
                                    // if non-positive trigger cast targeted to positive target this main cast is non-positive
                                    // this will place this spell auras as debuffs
                                    if(_IsPositiveTarget(spellTriggeredProto->Effects[effIndex].TargetA.GetTarget(),spellTriggeredProto->Effects[effIndex].TargetB.GetTarget()) && !spellTriggeredProto->IsPositiveEffect(i))
                                        return false;
                                }
                            }
                        }
                    }
                    break;
                case SPELL_AURA_PROC_TRIGGER_SPELL:
                    // many positive auras have negative triggered spells at damage for example and this not make it negative (it can be canceled for example)
                    break;
                case SPELL_AURA_MOD_STUN:                   //have positive and negative spells, we can't sort its correctly at this moment.
                    if(effIndex==0 && Effects[1].Effect==0 && Effects[2].Effect==0)
                        return false;                       // but all single stun aura spells is negative

                    // Petrification
                    if(Id == 17624)
                        return false;
                    break;
                case SPELL_AURA_MOD_PACIFY_SILENCE:
                    if (Id == 24740)             // Wisp Costume
                        return true;
                    return false;
                case SPELL_AURA_MOD_ROOT:
                case SPELL_AURA_MOD_SILENCE:
                case SPELL_AURA_GHOST:
                case SPELL_AURA_PERIODIC_LEECH:
                case SPELL_AURA_MOD_STALKED:
                case SPELL_AURA_PERIODIC_DAMAGE_PERCENT:
                    return false;
                case SPELL_AURA_PERIODIC_DAMAGE:            // used in positive spells also.
                    // part of negative spell if casted at self (prevent cancel)
                    if(Effects[effIndex].TargetA.GetTarget() == TARGET_UNIT_CASTER)
                        return false;
                    break;
                case SPELL_AURA_MOD_DECREASE_SPEED:         // used in positive spells also
                    // part of positive spell if casted at self
                    if(Effects[effIndex].TargetA.GetTarget() != TARGET_UNIT_CASTER)
                        return false;
                    // but not this if this first effect (don't found batter check)
                    if(Attributes & SPELL_ATTR0_NEGATIVE_1 && effIndex==0)
                        return false;
                    break;
                case SPELL_AURA_TRANSFORM:
                    // some spells negative
                    switch(Id)
                    {
                        case 36897:                         // Transporter Malfunction (race mutation to horde)
                        case 36899:                         // Transporter Malfunction (race mutation to alliance)
                            return false;
                    }
                    break;
                case SPELL_AURA_MOD_SCALE:
                    // some spells negative
                    switch(Id)
                    {
                        case 36900:                         // Soul Split: Evil!
                        case 36901:                         // Soul Split: Good
                        case 36893:                         // Transporter Malfunction (decrease size case)
                        case 36895:                         // Transporter Malfunction (increase size case)
                            return false;
                    }
                    break;
                case SPELL_AURA_MECHANIC_IMMUNITY:
                {
                    // non-positive immunities
                    switch(Effects[effIndex].MiscValue)
                    {
                        case MECHANIC_BANDAGE:
                        case MECHANIC_SHIELD:
                        case MECHANIC_MOUNT:
                        case MECHANIC_INVULNERABILITY:
                            return false;
                        default:
                            break;
                    }
                }   break;
                case SPELL_AURA_ADD_FLAT_MODIFIER:          // mods
                case SPELL_AURA_ADD_PCT_MODIFIER:
                {
                    // non-positive mods
                    switch(Effects[effIndex].MiscValue)
                    {
                        case SPELLMOD_COST:                 // dependent from bas point sign (negative -> positive)
                            if(Effects[effIndex].BasePoints+int32(Effects[effIndex].BaseDice) > 0)
                            {
                                if (!deep)
                                {
                                    bool negative = true;
                                    for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
                                    {
                                        if (i != effIndex)
                                            if (_IsPositiveEffect(i, true))
                                            {
                                                negative = false;
                                                break;
                                            }
                                    }
                                    if (negative)
                                        return false;
                                }
                            }
                            break;
                        default:
                            break;
                    }
                }   break;
                case SPELL_AURA_FORCE_REACTION:
                    if(Id==42792)               // Recently Dropped Flag (prevent cancel)
                        return false;
                    break;
                case SPELL_AURA_MOD_DAMAGE_PERCENT_TAKEN:
                    if (Effects[effIndex].BasePoints+int32(Effects[effIndex].BaseDice) > 0)
                        return false;
                    break;
                default:
                    break;
            }
            break;
        }
        
        default:
            break;
    }

    // non-positive targets
    if(!_IsPositiveTarget(Effects[effIndex].TargetA.GetTarget(),Effects[effIndex].TargetB.GetTarget()))
        return false;

    // negative spell if triggered spell is negative
    if (!deep && !Effects[effIndex].ApplyAuraName && Effects[effIndex].TriggerSpell)
    {
        if (SpellInfo const* spellTriggeredProto = sSpellMgr->GetSpellInfo(Effects[effIndex].TriggerSpell))
            if (!spellTriggeredProto->_IsPositiveSpell())
                return false;
    }

    // ok, positive
    return true;
}

bool SpellInfo::_IsPositiveTarget(uint32 targetA, uint32 targetB)
{
    // non-positive targets
    switch (targetA)
    {
        case TARGET_UNIT_NEARBY_ENEMY:
        case TARGET_UNIT_TARGET_ENEMY:
        case TARGET_UNIT_SRC_AREA_ENEMY:
        case TARGET_UNIT_DEST_AREA_ENEMY:
        case TARGET_UNIT_CONE_ENEMY:
//LK    case TARGET_UNIT_CONE_ENEMY_104:
        case TARGET_DEST_DYNOBJ_ENEMY:
        case TARGET_DEST_TARGET_ENEMY:
            return false;
        default:
            break;
    }
    if (targetB)
        return _IsPositiveTarget(targetB, 0);
    return true;
}

bool SpellInfo::_IsPositiveSpell() const
{
    // spells with at least one negative effect are considered negative
    // some self-applied spells have negative effects but in self casting case negative check ignored.
    for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
        if (!_IsPositiveEffect(i, true))
            return false;
    return true;
}

void SpellInfo::_UnloadImplicitTargetConditionLists()
{
    // find the same instances of ConditionContainer and delete them.
    for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
    {
        ConditionContainer* cur = Effects[i].ImplicitTargetConditions;
        if (!cur)
            continue;
        for (uint8 j = i; j < MAX_SPELL_EFFECTS; ++j)
        {
            if (Effects[j].ImplicitTargetConditions == cur)
                Effects[j].ImplicitTargetConditions = nullptr;
        }
        delete cur;
    }
}

void SpellInfo::_LoadImmunityInfo()
{
    for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
    {
        uint32 schoolImmunityMask = 0;
#ifdef LICH_KING
        uint32 applyHarmfulAuraImmunityMask = 0;
#endif
        uint32 mechanicImmunityMask = 0;
        uint32 dispelImmunity = 0;
        uint32 damageImmunityMask = 0;

        int32 miscVal = Effects[i].MiscValue;
//        int32 amount = Effects[i].CalcValue();

        ImmunityInfo& immuneInfo = _immunityInfo[i];

        switch (Effects[i].ApplyAuraName)
        {
        case SPELL_AURA_MECHANIC_IMMUNITY_MASK:
        {
            switch (miscVal)
            {
            case 96:   // Free Friend, Uncontrollable Frenzy, Warlord's Presence
            {
                mechanicImmunityMask |= IMMUNE_TO_MOVEMENT_IMPAIRMENT_AND_LOSS_CONTROL_MASK;

                immuneInfo.AuraTypeImmune.insert(SPELL_AURA_MOD_STUN);
                immuneInfo.AuraTypeImmune.insert(SPELL_AURA_MOD_DECREASE_SPEED);
                immuneInfo.AuraTypeImmune.insert(SPELL_AURA_MOD_ROOT);
                immuneInfo.AuraTypeImmune.insert(SPELL_AURA_MOD_CONFUSE);
                immuneInfo.AuraTypeImmune.insert(SPELL_AURA_MOD_FEAR);
                break;
            }
            case 1615: // Incite Rage, Wolf Spirit, Overload, Lightning Tendrils
            {
                switch (Id)
                {
                case 43292: // Incite Rage
                case 49172: // Wolf Spirit
                    mechanicImmunityMask |= IMMUNE_TO_MOVEMENT_IMPAIRMENT_AND_LOSS_CONTROL_MASK;

                    immuneInfo.AuraTypeImmune.insert(SPELL_AURA_MOD_STUN);
                    immuneInfo.AuraTypeImmune.insert(SPELL_AURA_MOD_DECREASE_SPEED);
                    immuneInfo.AuraTypeImmune.insert(SPELL_AURA_MOD_ROOT);
                    immuneInfo.AuraTypeImmune.insert(SPELL_AURA_MOD_CONFUSE);
                    immuneInfo.AuraTypeImmune.insert(SPELL_AURA_MOD_FEAR);
                    // no break intended
                case 61869: // Overload
                case 63481:
                case 61887: // Lightning Tendrils
                case 63486:
                    mechanicImmunityMask |= (1 << MECHANIC_INTERRUPT) | (1 << MECHANIC_SILENCE);

                    immuneInfo.SpellEffectImmune.insert(SPELL_EFFECT_KNOCK_BACK);
                    immuneInfo.SpellEffectImmune.insert(SPELL_EFFECT_KNOCK_BACK_DEST);
                    break;
                default:
                    break;
                }
                break;
            }
            case 679:  // Mind Control, Avenging Fury
            {
                if (Id == 57742) // Avenging Fury
                {
                    mechanicImmunityMask |= IMMUNE_TO_MOVEMENT_IMPAIRMENT_AND_LOSS_CONTROL_MASK;

                    immuneInfo.AuraTypeImmune.insert(SPELL_AURA_MOD_STUN);
                    immuneInfo.AuraTypeImmune.insert(SPELL_AURA_MOD_DECREASE_SPEED);
                    immuneInfo.AuraTypeImmune.insert(SPELL_AURA_MOD_ROOT);
                    immuneInfo.AuraTypeImmune.insert(SPELL_AURA_MOD_CONFUSE);
                    immuneInfo.AuraTypeImmune.insert(SPELL_AURA_MOD_FEAR);
                }
                break;
            }
            case 1557: // Startling Roar, Warlord Roar, Break Bonds, Stormshield
            {
                if (Id == 64187) // Stormshield
                {
                    mechanicImmunityMask |= (1 << MECHANIC_STUN);
                    immuneInfo.AuraTypeImmune.insert(SPELL_AURA_MOD_STUN);
                }
                else
                {
                    mechanicImmunityMask |= IMMUNE_TO_MOVEMENT_IMPAIRMENT_AND_LOSS_CONTROL_MASK;

                    immuneInfo.AuraTypeImmune.insert(SPELL_AURA_MOD_STUN);
                    immuneInfo.AuraTypeImmune.insert(SPELL_AURA_MOD_DECREASE_SPEED);
                    immuneInfo.AuraTypeImmune.insert(SPELL_AURA_MOD_ROOT);
                    immuneInfo.AuraTypeImmune.insert(SPELL_AURA_MOD_CONFUSE);
                    immuneInfo.AuraTypeImmune.insert(SPELL_AURA_MOD_FEAR);
                }
                break;
            }
            case 1614: // Fixate
            case 1694: // Fixated, Lightning Tendrils
            {
                immuneInfo.SpellEffectImmune.insert(SPELL_EFFECT_ATTACK_ME);
                immuneInfo.AuraTypeImmune.insert(SPELL_AURA_MOD_TAUNT);
                break;
            }
            case 1630: // Fervor, Berserk
            {
                if (Id == 64112) // Berserk
                {
                    immuneInfo.SpellEffectImmune.insert(SPELL_EFFECT_ATTACK_ME);
                    immuneInfo.AuraTypeImmune.insert(SPELL_AURA_MOD_TAUNT);
                }
                else
                {
                    mechanicImmunityMask |= IMMUNE_TO_MOVEMENT_IMPAIRMENT_AND_LOSS_CONTROL_MASK;

                    immuneInfo.AuraTypeImmune.insert(SPELL_AURA_MOD_STUN);
                    immuneInfo.AuraTypeImmune.insert(SPELL_AURA_MOD_DECREASE_SPEED);
                    immuneInfo.AuraTypeImmune.insert(SPELL_AURA_MOD_ROOT);
                    immuneInfo.AuraTypeImmune.insert(SPELL_AURA_MOD_CONFUSE);
                    immuneInfo.AuraTypeImmune.insert(SPELL_AURA_MOD_FEAR);
                }
                break;
            }
#ifdef LICH_KING
            case 477:  // Bladestorm
            case 1733: // Bladestorm, Killing Spree
            {
                if (!amount)
                {
                    mechanicImmunityMask |= IMMUNE_TO_MOVEMENT_IMPAIRMENT_AND_LOSS_CONTROL_MASK;

                    immuneInfo.SpellEffectImmune.insert(SPELL_EFFECT_KNOCK_BACK);
                    immuneInfo.SpellEffectImmune.insert(SPELL_EFFECT_KNOCK_BACK_DEST);

                    immuneInfo.AuraTypeImmune.insert(SPELL_AURA_MOD_STUN);
                    immuneInfo.AuraTypeImmune.insert(SPELL_AURA_MOD_DECREASE_SPEED);
                    immuneInfo.AuraTypeImmune.insert(SPELL_AURA_MOD_ROOT);
                    immuneInfo.AuraTypeImmune.insert(SPELL_AURA_MOD_CONFUSE);
                    immuneInfo.AuraTypeImmune.insert(SPELL_AURA_MOD_FEAR);
                }
                break;
            }
#endif
            case 878: // Whirlwind, Fog of Corruption, Determination
            {
                if (Id == 66092) // Determination
                {
                    mechanicImmunityMask |= (1 << MECHANIC_SNARE) | (1 << MECHANIC_STUN)
                        | (1 << MECHANIC_DISORIENTED) | (1 << MECHANIC_FREEZE);

                    immuneInfo.AuraTypeImmune.insert(SPELL_AURA_MOD_STUN);
                    immuneInfo.AuraTypeImmune.insert(SPELL_AURA_MOD_DECREASE_SPEED);
                }
                break;
            }
            default:
                break;
            }

            if (immuneInfo.AuraTypeImmune.empty())
            {
                if (miscVal & (1 << 10))
                    immuneInfo.AuraTypeImmune.insert(SPELL_AURA_MOD_STUN);
                if (miscVal & (1 << 1))
                    immuneInfo.AuraTypeImmune.insert(SPELL_AURA_TRANSFORM);

                // These flag can be recognized wrong:
                if (miscVal & (1 << 6))
                    immuneInfo.AuraTypeImmune.insert(SPELL_AURA_MOD_DECREASE_SPEED);
                if (miscVal & (1 << 0))
                    immuneInfo.AuraTypeImmune.insert(SPELL_AURA_MOD_ROOT);
                if (miscVal & (1 << 2))
                    immuneInfo.AuraTypeImmune.insert(SPELL_AURA_MOD_CONFUSE);
                if (miscVal & (1 << 9))
                    immuneInfo.AuraTypeImmune.insert(SPELL_AURA_MOD_FEAR);
                if (miscVal & (1 << 7))
                    immuneInfo.AuraTypeImmune.insert(SPELL_AURA_MOD_DISARM);
            }
            break;
        }
        case SPELL_AURA_MECHANIC_IMMUNITY:
        {
            switch (Id)
            {
            case 34471: // The Beast Within
            case 19574: // Bestial Wrath
            case 42292: // PvP trinket
            case 59752: // Every Man for Himself
            case 53490: // Bullheaded
                mechanicImmunityMask |= IMMUNE_TO_MOVEMENT_IMPAIRMENT_AND_LOSS_CONTROL_MASK;
                break;
            case 54508: // Demonic Empowerment
                mechanicImmunityMask |= (1 << MECHANIC_SNARE) | (1 << MECHANIC_ROOT) | (1 << MECHANIC_STUN);
                break;
            default:
                if (miscVal < 1)
                    continue;

                mechanicImmunityMask |= 1 << miscVal;
                break;
            }
            break;
        }
        case SPELL_AURA_EFFECT_IMMUNITY:
        {
            immuneInfo.SpellEffectImmune.insert(static_cast<SpellEffects>(miscVal));
            break;
        }
        case SPELL_AURA_STATE_IMMUNITY:
        {
            immuneInfo.AuraTypeImmune.insert(static_cast<AuraType>(miscVal));
            break;
        }
        case SPELL_AURA_SCHOOL_IMMUNITY:
        {
            schoolImmunityMask |= uint32(miscVal);
            break;
        }
#ifdef LICH_KING
        case SPELL_AURA_MOD_IMMUNE_AURA_APPLY_SCHOOL:
        {
            applyHarmfulAuraImmunityMask |= uint32(miscVal);
            break;
        }
#endif
        case SPELL_AURA_DAMAGE_IMMUNITY:
        {
            damageImmunityMask |= uint32(miscVal);
            break;
        }
        case SPELL_AURA_DISPEL_IMMUNITY:
        {
            dispelImmunity = uint32(miscVal);
            break;
        }
        default:
            break;
        }

        immuneInfo.SchoolImmuneMask = schoolImmunityMask;
#ifdef LICH_KING
        immuneInfo.ApplyHarmfulAuraImmuneMask = applyHarmfulAuraImmunityMask;
#endif
        immuneInfo.MechanicImmuneMask = mechanicImmunityMask;
        immuneInfo.DispelImmune = dispelImmunity;
        immuneInfo.DamageSchoolMask = damageImmunityMask;

        immuneInfo.AuraTypeImmune.shrink_to_fit();
        immuneInfo.SpellEffectImmune.shrink_to_fit();

        _allowedMechanicMask |= immuneInfo.MechanicImmuneMask;
    }

    if (HasAttribute(SPELL_ATTR5_USABLE_WHILE_STUNNED))
    {
        switch (Id)
        {
        case 22812: // Barkskin
            _allowedMechanicMask |=
                (1 << MECHANIC_STUN) |
                (1 << MECHANIC_FREEZE) |
                (1 << MECHANIC_KNOCKOUT) |
                (1 << MECHANIC_SLEEP);
            break;
        case 49039: // Lichborne, don't allow normal stuns
            break;
        default:
            _allowedMechanicMask |= (1 << MECHANIC_STUN);
            break;
        }
    }

    if (HasAttribute(SPELL_ATTR5_USABLE_WHILE_CONFUSED))
        _allowedMechanicMask |= (1 << MECHANIC_DISORIENTED);

    if (HasAttribute(SPELL_ATTR5_USABLE_WHILE_FEARED))
        _allowedMechanicMask |= (1 << MECHANIC_FEAR);
}

bool SpellInfo::CanPierceImmuneAura(SpellInfo const* auraSpellInfo) const
{
    // aura can't be pierced
    if (!auraSpellInfo || auraSpellInfo->HasAttribute(SPELL_ATTR0_UNAFFECTED_BY_INVULNERABILITY))
        return false;

    // these spells pierce all available spells (Resurrection Sickness for example)
    if (HasAttribute(SPELL_ATTR0_UNAFFECTED_BY_INVULNERABILITY))
        return true;

    // these spells (Cyclone for example) can pierce all...
    if (HasAttribute(SPELL_ATTR1_UNAFFECTED_BY_SCHOOL_IMMUNE) || HasAttribute(SPELL_ATTR2_UNAFFECTED_BY_AURA_SCHOOL_IMMUNE))
    {
        // ...but not these (Divine shield, Ice block, Cyclone and Banish for example)
        if (auraSpellInfo->Mechanic != MECHANIC_IMMUNE_SHIELD &&
            auraSpellInfo->Mechanic != MECHANIC_INVULNERABILITY &&
            (auraSpellInfo->Mechanic != MECHANIC_BANISH || (IsRankOf(auraSpellInfo) && auraSpellInfo->Dispel != DISPEL_NONE))) // Banish shouldn't be immune to itself, but Cyclone should
            return true;
    }

    // Dispels other auras on immunity, check if this spell makes the unit immune to aura
    if (HasAttribute(SPELL_ATTR1_DISPEL_AURAS_ON_IMMUNITY) && CanSpellProvideImmunityAgainstAura(auraSpellInfo))
        return true;

    return false;
}

bool SpellInfo::CanDispelAura(SpellInfo const* auraSpellInfo) const
{
    // These auras (like Divine Shield) can't be dispelled
    if (auraSpellInfo->HasAttribute(SPELL_ATTR0_UNAFFECTED_BY_INVULNERABILITY))
        return false;

    // These spells (like Mass Dispel) can dispel all auras
    if (HasAttribute(SPELL_ATTR0_UNAFFECTED_BY_INVULNERABILITY))
        return true;

    // These auras (Cyclone for example) are not dispelable
    if ((auraSpellInfo->HasAttribute(SPELL_ATTR1_UNAFFECTED_BY_SCHOOL_IMMUNE) && auraSpellInfo->Mechanic != MECHANIC_NONE)
        || auraSpellInfo->HasAttribute(SPELL_ATTR2_UNAFFECTED_BY_AURA_SCHOOL_IMMUNE))
        return false;

    return true;
}

void SpellInfo::ApplyDispelImmune(Unit* target, DispelType dispelType, bool apply) const
{
    target->ApplySpellImmune(Id, IMMUNITY_DISPEL, dispelType, apply);

    if (apply && HasAttribute(SPELL_ATTR1_DISPEL_AURAS_ON_IMMUNITY))
    {
        target->RemoveAppliedAuras([dispelType](AuraApplication const* aurApp) -> bool
        {
            SpellInfo const* spellInfo = aurApp->GetBase()->GetSpellInfo();
            if (DispelType(spellInfo->Dispel) == dispelType)
                return true;

            return false;
        });
    }
}

void SpellInfo::ApplyMechanicImmune(Unit* target, uint32 mechanicImmunityMask, bool apply) const
{
    for (uint32 i = 0; i < MAX_MECHANIC; ++i)
        if (mechanicImmunityMask & (1 << i))
            target->ApplySpellImmune(Id, IMMUNITY_MECHANIC, i, apply);

    if (apply && HasAttribute(SPELL_ATTR1_DISPEL_AURAS_ON_IMMUNITY))
        target->RemoveAurasWithMechanic(mechanicImmunityMask, AURA_REMOVE_BY_DEFAULT, Id);
}

void SpellInfo::ApplySchoolImmune(Unit* target, uint32 schoolMask, bool apply) const
{
    target->ApplySpellImmune(Id, IMMUNITY_SCHOOL, schoolMask, apply);

    if (apply && HasAttribute(SPELL_ATTR1_DISPEL_AURAS_ON_IMMUNITY))
    {
        target->RemoveAppliedAuras([this, schoolMask](AuraApplication const* aurApp) -> bool
        {
            SpellInfo const* auraSpellInfo = aurApp->GetBase()->GetSpellInfo();
            return ((auraSpellInfo->GetSchoolMask() & schoolMask) != 0 && // Check for school mask
                CanDispelAura(auraSpellInfo) &&
                (IsPositive() != aurApp->IsPositive()) &&                     // Check spell vs aura possitivity
                !auraSpellInfo->IsPassive() &&                                // Don't remove passive auras
                auraSpellInfo->Id != Id);                                     // Don't remove self
        });
    }
}

void SpellInfo::ApplyAllSpellImmunitiesTo(Unit* target, uint8 effIndex, bool apply) const
{
    ImmunityInfo const* immuneInfo = _immunityInfo + effIndex;

    if (uint32 schoolImmunity = immuneInfo->SchoolImmuneMask)
        ApplySchoolImmune(target, schoolImmunity, apply);

    if (uint32 mechanicImmunityMask = immuneInfo->MechanicImmuneMask)
        ApplyMechanicImmune(target, mechanicImmunityMask, apply);

    if (DispelType dispelImmunity = DispelType(immuneInfo->DispelImmune))
        ApplyDispelImmune(target, dispelImmunity, apply);

    if (uint32 damageImmunity = immuneInfo->DamageSchoolMask)
        target->ApplySpellImmune(Id, IMMUNITY_DAMAGE, damageImmunity, apply);

    for (AuraType auraType : immuneInfo->AuraTypeImmune)
    {
        target->ApplySpellImmune(Id, IMMUNITY_STATE, auraType, apply);
        if (apply && HasAttribute(SPELL_ATTR1_DISPEL_AURAS_ON_IMMUNITY))
            target->RemoveAurasByType(auraType);
    }

    for (SpellEffects effectType : immuneInfo->SpellEffectImmune)
        target->ApplySpellImmune(Id, IMMUNITY_EFFECT, effectType, apply);
}

uint32 SpellInfo::GetAllowedMechanicMask() const
{
    return _allowedMechanicMask;
}


bool SpellInfo::CanSpellProvideImmunityAgainstAura(SpellInfo const* auraSpellInfo) const
{
    if (!auraSpellInfo)
        return false;

    for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
    {
        ImmunityInfo const* immuneInfo = _immunityInfo + i;

        if (!auraSpellInfo->HasAttribute(SPELL_ATTR1_UNAFFECTED_BY_SCHOOL_IMMUNE) && !auraSpellInfo->HasAttribute(SPELL_ATTR2_UNAFFECTED_BY_AURA_SCHOOL_IMMUNE))
        {
            if (uint32 schoolImmunity = immuneInfo->SchoolImmuneMask)
                if ((auraSpellInfo->SchoolMask & schoolImmunity) != 0)
                    return true;
        }

        if (uint32 mechanicImmunity = immuneInfo->MechanicImmuneMask)
            if ((mechanicImmunity & (1 << auraSpellInfo->Mechanic)) != 0)
                return true;

        if (uint32 dispelImmunity = immuneInfo->DispelImmune)
            if (auraSpellInfo->Dispel == dispelImmunity)
                return true;

        bool immuneToAllEffects = true;
        for (uint8 effIndex = 0; effIndex < MAX_SPELL_EFFECTS; ++effIndex)
        {
            uint32 effectName = auraSpellInfo->Effects[effIndex].Effect;
            if (!effectName)
                continue;

            auto spellImmuneItr = immuneInfo->SpellEffectImmune.find(static_cast<SpellEffects>(effectName));
            if (spellImmuneItr == immuneInfo->SpellEffectImmune.cend())
            {
                immuneToAllEffects = false;
                break;
            }

            if (uint32 mechanic = auraSpellInfo->Effects[effIndex].Mechanic)
            {
                if (!(immuneInfo->MechanicImmuneMask & (1 << mechanic)))
                {
                    immuneToAllEffects = false;
                    break;
                }
            }

            if (!auraSpellInfo->HasAttribute(SPELL_ATTR3_IGNORE_HIT_RESULT))
            {
                if (uint32 auraName = auraSpellInfo->Effects[effIndex].ApplyAuraName)
                {
                    bool isImmuneToAuraEffectApply = false;
                    auto auraImmuneItr = immuneInfo->AuraTypeImmune.find(static_cast<AuraType>(auraName));
                    if (auraImmuneItr != immuneInfo->AuraTypeImmune.cend())
                        isImmuneToAuraEffectApply = true;

                    if (!isImmuneToAuraEffectApply && !auraSpellInfo->IsPositiveEffect(effIndex) && !auraSpellInfo->HasAttribute(SPELL_ATTR2_UNAFFECTED_BY_AURA_SCHOOL_IMMUNE))
                    {
                        if (uint32 applyHarmfulAuraImmunityMask = immuneInfo->ApplyHarmfulAuraImmuneMask)
                            if ((auraSpellInfo->GetSchoolMask() & applyHarmfulAuraImmunityMask) != 0)
                                isImmuneToAuraEffectApply = true;
                    }

                    if (!isImmuneToAuraEffectApply)
                    {
                        immuneToAllEffects = false;
                        break;
                    }
                }
            }
        }

        if (immuneToAllEffects)
            return true;
    }

    return false;
}

// based on client Spell_C::CancelsAuraEffect
bool SpellInfo::SpellCancelsAuraEffect(SpellInfo const* auraSpellInfo, uint8 auraEffIndex) const
{
    if (!HasAttribute(SPELL_ATTR1_DISPEL_AURAS_ON_IMMUNITY))
        return false;

    if (auraSpellInfo->HasAttribute(SPELL_ATTR0_UNAFFECTED_BY_INVULNERABILITY))
        return false;

    for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
    {
        if (Effects[i].Effect != SPELL_EFFECT_APPLY_AURA)
            continue;

        uint32 const miscValue = static_cast<uint32>(Effects[i].MiscValue);
        switch (Effects[i].ApplyAuraName)
        {
        case SPELL_AURA_STATE_IMMUNITY:
            if (miscValue != auraSpellInfo->Effects[auraEffIndex].ApplyAuraName)
                continue;
            break;
        case SPELL_AURA_SCHOOL_IMMUNITY:
#ifdef LICH_KING
        case SPELL_AURA_MOD_IMMUNE_AURA_APPLY_SCHOOL:
#endif
            if (auraSpellInfo->HasAttribute(SPELL_ATTR2_UNAFFECTED_BY_AURA_SCHOOL_IMMUNE) || !(auraSpellInfo->SchoolMask & miscValue))
                continue;
            break;
        case SPELL_AURA_DISPEL_IMMUNITY:
            if (miscValue != auraSpellInfo->Dispel)
                continue;
            break;
        case SPELL_AURA_MECHANIC_IMMUNITY:
            if (miscValue != auraSpellInfo->Mechanic)
            {
                if (miscValue != auraSpellInfo->Effects[auraEffIndex].Mechanic)
                    continue;
            }
            break;
        default:
            continue;
        }

        return true;
    }

    return false;
}

SpellCastResult SpellInfo::CheckLocation(uint32 map_id, uint32 zone_id, uint32 area_id, Player const* player /*= nullptr*/, bool strict /*= true*/) const
{
    // normal case
#ifdef LICH_KING
    SpellCastResult requireArea = SPELL_FAILED_INCORRECT_AREA;
#else
    SpellCastResult requireArea = SPELL_FAILED_REQUIRES_AREA;
#endif
#ifdef LICH_KING
    if (AreaGroupId > 0)
    {
        bool found = false;
        AreaGroupEntry const* groupEntry = sAreaGroupStore.LookupEntry(AreaGroupId);
        while (groupEntry)
        {
            for (uint8 i = 0; i < MAX_GROUP_AREA_IDS; ++i)
                if (groupEntry->AreaId[i] == zone_id || groupEntry->AreaId[i] == area_id)
                    found = true;
            if (found || !groupEntry->nextGroup)
                break;
            // Try search in next group
            groupEntry = sAreaGroupStore.LookupEntry(groupEntry->nextGroup);
        }

        if (!found)
            return requireArea;
    }
#else
    // normal case
    if (AreaId && AreaId != zone_id && AreaId != area_id)
        return requireArea;
#endif

    // continent limitation (virtual continent) (only flying mounts have this check)
    if (HasAttribute(SPELL_ATTR4_CAST_ONLY_IN_OUTLAND))
    {
        if (strict)
        {
            AreaTableEntry const* areaEntry = sAreaTableStore.LookupEntry(area_id);
            if (!areaEntry)
                areaEntry = sAreaTableStore.LookupEntry(zone_id);

            if (!areaEntry || !areaEntry->IsFlyable() || !player->CanFlyInZone(map_id, zone_id, this))
                return requireArea;
        }
        else
        {
            uint32 const v_map = GetVirtualMapForMapAndZone(map_id, zone_id);
            MapEntry const* mapEntry = sMapStore.LookupEntry(v_map);
            if (!mapEntry || mapEntry->Expansion() < 1 || !mapEntry->IsContinent())
                return requireArea;
        }
    }


    MapEntry const* _mapEntry = sMapStore.LookupEntry(map_id);
    if (_mapEntry && _mapEntry->IsRaid() && HasAttribute(SPELL_ATTR6_NOT_IN_RAID_INSTANCE))
        return requireArea;

    // elixirs (all area dependent elixirs have family SPELLFAMILY_POTION, use this for speedup)
    if (SpellFamilyName == SPELLFAMILY_POTION)
    {
        if (uint32 mask = sSpellMgr->GetSpellElixirMask(Id))
        {
            if (mask & ELIXIR_BATTLE_MASK)
            {
                if (Id == 45373 && zone_id != 4075)                    // Bloodberry Elixir
                    return requireArea;
            }
            if (mask & ELIXIR_UNSTABLE_MASK)
            {
                // in the Blade's Edge Mountains Plateaus and Gruul's Lair.
                if(zone_id != 3522 && map_id != 565)
                    return requireArea;
            }
            if (mask & ELIXIR_SHATTRATH_MASK)
            {
                // in Tempest Keep, Serpentshrine Cavern, Caverns of Time: Mount Hyjal, Black Temple, Sunwell Plateau
                if (zone_id == 3607 || map_id == 534 || map_id == 564 || zone_id == 4075)
                    return SPELL_CAST_OK;

                MapEntry const* mapEntry = sMapStore.LookupEntry(map_id);
                if (!mapEntry)
                    return requireArea;

                if(mapEntry->multimap_id != 206)
                    return requireArea;
            }

            return SPELL_CAST_OK;
        }
    }

    // special cases zone check (maps checked by multimap common id)
    switch (Id)
    {
    case 46394:
    {
        if (map_id != 580)
            return requireArea;
        break;
    }
    case 23333:                                         // Warsong Flag
    case 23335:                                         // Silverwing Flag
    case 46392:                                         // Focused Assault
    case 46393:                                         // Brutal Assault
    {
        MapEntry const* mapEntry = sMapStore.LookupEntry(map_id);
        if (!mapEntry)
            return requireArea;

        if (!mapEntry->IsBattleground())
            return requireArea;

        if (zone_id != 3277) //warsong gulch
            return requireArea;
        break;
    }
    case 34976:                                         // Netherstorm Flag
    {
        MapEntry const* mapEntry = sMapStore.LookupEntry(map_id);
        if (!mapEntry)
            return requireArea;

        if (!mapEntry->IsBattleground())
            return requireArea;

        if (zone_id != 3820)
            return requireArea;

        break;
    }
    case 32724:                                         // Gold Team (Alliance)
    case 32725:                                         // Green Team (Alliance)
    case 32727:                                         // Arena Preparation
    case 35774:                                         // Gold Team (Horde)
    case 35775:                                         // Green Team (Horde)
    {
        MapEntry const* mapEntry = sMapStore.LookupEntry(map_id);
        if (!mapEntry || !mapEntry->IsBattleArena())
            return requireArea;
        break;
    }
    case 41618:                                         // Bottled Nethergon Energy
    case 41620:                                         // Bottled Nethergon Vapor
    {
        MapEntry const* mapEntry = sMapStore.LookupEntry(map_id);
        if (!mapEntry || mapEntry->multimap_id != 206)
            return requireArea;
        break;
    }
    case 41617:                                         // Cenarion Mana Salve
    case 41619:                                         // Cenarion Healing Salve
    {
        MapEntry const* mapEntry = sMapStore.LookupEntry(map_id);
        if (!mapEntry || mapEntry->multimap_id != 207)
            return requireArea;
        break;
    }
    case 40216:                                         // Dragonmaw Illusion
    case 42016:                                         // Dragonmaw Illusion
        if(area_id != 3759 && area_id != 3966 && area_id != 3939 && area_id != 3965)
            return requireArea;
        break;
    case 2584:                                          // Waiting to Resurrect
    case 22011:                                         // Spirit Heal Channel
    case 22012:                                         // Spirit Heal
    case 24171:                                         // Resurrection Impact Visual
    case 42792:                                         // Recently Dropped Flag
    case 43681:                                         // Inactive
    case 44535:                                         // Spirit Heal (mana)
    case 44521:                                         // Preparation
    {
        MapEntry const* mapEntry = sMapStore.LookupEntry(map_id);
        if (!mapEntry || !mapEntry->IsBattleground())
            return requireArea;
        break;
    }
    case 27720:
    case 27721:
    case 27722:
    case 27723:
    {
        MapEntry const* mapEntry = sMapStore.LookupEntry(map_id);
        if (!mapEntry || mapEntry->MapID == 580) //sunwell
            return requireArea;

        break;
    }
    case 40817:                                         // Quest 11026
    {
        if (area_id != 3784 && area_id != 3785)
            return requireArea;

        break;
    }
    case 32314:
    {
        if (area_id != 3616)
            return requireArea;

        break;
    }
    case 45401:
    {
        if (zone_id != 4080 && map_id != 585 && map_id != 580)
            return requireArea;
        break;
    }
    case 40310:
    case 40623:
    {
        switch (area_id) 
        {
        case 3784:
        case 3832:
        case 3786:
        case 4008:
        case 3785:
        case 3864:
        case 3865:
        case 3971:
        case 3972:
            break;
        default:
            return requireArea;
        }
        break;
    }
    }

    return SPELL_CAST_OK;
}