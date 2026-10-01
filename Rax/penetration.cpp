#include "includes.h"

#define MAX_PENETRATION_DISTANCE 90 // this is 7.5 feet

float auto_wall::DistanceToRay(vec3_t pos, vec3_t rayStart, vec3_t rayEnd) {
	vec3_t dir = rayEnd - rayStart;
	float length = dir.length();
	dir.normalize();

	float rangeAlong = dir.dot(pos - rayStart);

	static float range;

	if (rangeAlong < 0.0f) {
		range = -(pos - rayStart).length();
	}
	else if (rangeAlong > length) {
		range = -(pos - rayEnd).length();
	}
	else {
		vec3_t onRay = rayStart + (dir * rangeAlong);
		range = (pos - onRay).length();
	}

	return range;
}

void auto_wall::UTIL_ClipTraceToPlayers(vec3_t vecAbsStart, vec3_t vecAbsEnd, unsigned int mask, ITraceFilter* filter, CGameTrace* tr, Player* pTargetEntity) {
	CGameTrace playerTrace;
	Ray ray{ vecAbsStart, vecAbsEnd };
	float smallestFraction = tr->m_fraction;
	constexpr float maxRange = 60.0f;

	if (pTargetEntity) {
		if (!pTargetEntity || !pTargetEntity->alive())
			return;

		if (filter && !filter->ShouldHitEntity(pTargetEntity, mask))
			return;

		float range = DistanceToRay(pTargetEntity->WorldSpaceCenter(), vecAbsStart, vecAbsEnd);
		if (range < 0.0f || range > maxRange)
			return;

		g_csgo.m_engine_trace->ClipRayToEntity(ray, mask | CONTENTS_HITBOX, pTargetEntity, &playerTrace);
		if (playerTrace.m_fraction < smallestFraction) {
			tr = &playerTrace;
			smallestFraction = playerTrace.m_fraction;
		}

		return;
	}

	for (int entIndex = 0; entIndex < g_csgo.m_globals->m_max_clients; ++entIndex) {
		Player* player = g_csgo.m_entlist->GetClientEntity<Player*>(entIndex);
		if (!player || !player->alive())
			continue;

		if (filter && !filter->ShouldHitEntity(player, mask))
			continue;

		float range = DistanceToRay(player->WorldSpaceCenter(), vecAbsStart, vecAbsEnd);
		if (range < 0.0f || range > maxRange)
			continue;

		g_csgo.m_engine_trace->ClipRayToEntity(ray, mask | CONTENTS_HITBOX, player, &playerTrace);
		if (playerTrace.m_fraction < smallestFraction) {
			tr = &playerTrace;
			smallestFraction = playerTrace.m_fraction;
		}
	}
}

void auto_wall::ScaleDamage(Player* player, const float& armorRatio, float& currentDamage, const int& hitGroup) {
	bool hasHeavyArmor = player->m_bHasHeavyArmor();
	int armorValue = player->m_ArmorValue();

	//Does the person have armor on for the hitbox checked?
	auto IsArmored = [&]()->bool
		{
			switch (hitGroup)
			{
			case HITGROUP_HEAD:
				return player->m_bHasHelmet(); //force-convert it to a bool via (!!)
			case HITGROUP_GENERIC:
			case HITGROUP_CHEST:
			case HITGROUP_STOMACH:
			case HITGROUP_LEFTARM:
			case HITGROUP_RIGHTARM:
				return true;
			default:
				return false;
			}
		};

	switch (hitGroup) {
	case HITGROUP_HEAD:
		currentDamage *= hasHeavyArmor ? 2.f : 4.f; //Heavy Armor does 1/2 damage
		break;
	case HITGROUP_STOMACH:
		currentDamage *= 1.25f;
		break;
	case HITGROUP_LEFTLEG:
	case HITGROUP_RIGHTLEG:
		currentDamage *= 0.75f;
		break;
	default:
		break;
	}

	if (armorValue > 0 && IsArmored()) {
		float bonusValue = 1.f, armorBonusRatio = 0.5f, armorRatioHalf = armorRatio * 0.5f;

		//Damage gets modified for heavy armor users
		if (hasHeavyArmor) {
			armorBonusRatio = 0.33f;
			armorRatioHalf *= 0.5f;
			bonusValue = 0.33f;
		}

		auto NewDamage = currentDamage * armorRatioHalf;

		if (hasHeavyArmor)
			NewDamage *= 0.85f;

		if (((currentDamage - (currentDamage * armorRatioHalf)) * (bonusValue * armorBonusRatio)) > armorValue)
			NewDamage = currentDamage - (armorValue / armorBonusRatio);

		currentDamage = NewDamage;
	}
}

bool auto_wall::IsBreakableEntity(Entity* pEnt) {
	if (!pEnt)
		return false;

	if (!pEnt || pEnt->index() == 0)
		return false;

	static auto isBreakable = pattern::find(g_csgo.m_client_dll, XOR("55 8B EC 51 56 8B F1 85 F6 74 68")).as<uintptr_t>();
	static uintptr_t uTakeDamage = *(uintptr_t*)(isBreakable + 0x26);
	const uintptr_t uTakeDamageBackup = *(uint8_t*)((uintptr_t)pEnt + uTakeDamage);

	const ClientClass* pClientClass = pEnt->GetClientClass();
	if (pClientClass) {
		const char* name = pClientClass->m_pNetworkName;

		// CBreakableSurface, CBaseDoor, ...
		if (name[1] != 'F'
			|| name[4] != 'c'
			|| name[5] != 'B'
			|| name[9] != 'h') {
			*(uint8_t*)((uintptr_t)pEnt + uTakeDamage) = 2; /*DAMAGE_YES*/
		}
	}

	using fnIsBreakable = bool(__thiscall*)(Entity*);
	const bool bResult = ((fnIsBreakable)isBreakable)(pEnt);
	*(uint8_t*)((uintptr_t)pEnt + uTakeDamage) = uTakeDamageBackup;

	return bResult;
}

bool auto_wall::TraceToExit(vec3_t start, vec3_t dir, vec3_t& end, CGameTrace& trEnter, CGameTrace& trExit, float flStepSize, float flMaxDistance) {
	float flDistance = 0;
	int nStartContents = 0;
	static CTraceFilter filter;

	while (flDistance <= flMaxDistance) {
		flDistance += flStepSize;
		end = start + (dir * flDistance);

		vec3_t vecTrEnd = end - (dir * flStepSize);
		int nCurrentContents = g_csgo.m_engine_trace->GetPointContents(end, MASK_SHOT);

		if (nStartContents == 0)
			nStartContents = nCurrentContents;

		if ((nCurrentContents & MASK_SHOT_HULL) && (!(nCurrentContents & CONTENTS_HITBOX) || (nCurrentContents == nStartContents)))
			continue;

		g_csgo.m_engine_trace->TraceRay(Ray(end, vecTrEnd), MASK_SHOT, (ITraceFilter*)&filter, &trExit);
		if (trExit.m_startsolid && (trExit.m_surface.m_flags & SURF_HITBOX)) {
			filter.m_skip = trExit.m_entity;
			g_csgo.m_engine_trace->TraceRay(Ray(end, start), MASK_SHOT_HULL, (ITraceFilter*)&filter, &trExit);
			if (trExit.DidHit() && trExit.m_startsolid == false) {
				end = trExit.m_endpos;
				return true;
			}

			continue;
		}
		else if (trExit.DidHit() && trExit.m_startsolid == false) {
			bool bStartIsNodraw = (trEnter.m_surface.m_flags & SURF_NODRAW);
			bool bExitIsNodraw = (trExit.m_surface.m_flags & SURF_NODRAW);
			if (bExitIsNodraw && IsBreakableEntity(trExit.m_entity) && IsBreakableEntity(trEnter.m_entity)) {
				end = trExit.m_endpos;
				return true;
			}
			else if (bExitIsNodraw == false || (bStartIsNodraw && bExitIsNodraw)) {
				vec3_t vecNormal = trExit.m_plane.m_normal;
				float flDot = dir.dot(vecNormal);
				if (flDot <= 1.f) {
					end = end - (dir * (flStepSize * trExit.m_fraction));
					return true;
				}
				continue;
			}
		}
		else if (trEnter.DidHitNonWorldEntity() && IsBreakableEntity(trEnter.m_entity)) {
			trExit = trEnter;
			trExit.m_endpos = start + dir;
			return true;
		}
	}

	return false;
}

bool auto_wall::HandleBulletPenetration(float& flPenetration,
	const int& iEnterMaterial,
	CGameTrace& tr,
	vec3_t& vecDir,
	surfacedata_t* pSurfaceData,
	float flPenetrationModifier,
	float flDamageModifier,
	float flPenetrationPower,
	int& nPenetrationCount,
	vec3_t& vecSrc,
	float flDistance,
	float flCurrentDistance,
	float& fCurrentDamage)
{
	static ConVar* ff_damage_reduction_bullets = g_csgo.m_cvar->FindVar(HASH("ff_damage_reduction_bullets"));
	static ConVar* ff_damage_bullet_penetration = g_csgo.m_cvar->FindVar(HASH("ff_damage_bullet_penetration"));

	bool bIsNodraw = (tr.m_surface.m_flags & SURF_NODRAW);
	bool bIsGrate = (tr.m_contents & CONTENTS_GRATE);

	if (nPenetrationCount == 0 && !bIsGrate && !bIsNodraw
		&& iEnterMaterial != CHAR_TEX_GLASS && iEnterMaterial != CHAR_TEX_GRATE)
		return true;

	if (flPenetration <= 0.f || nPenetrationCount <= 0)
		return true;

	static vec3_t penetrationEnd;

	CGameTrace exitTr;
	if (!TraceToExit(tr.m_endpos, vecDir, penetrationEnd, tr, exitTr, 4.f, MAX_PENETRATION_DISTANCE)) {
		if ((g_csgo.m_engine_trace->GetPointContents(tr.m_endpos, MASK_SHOT_HULL) & MASK_SHOT_HULL) == 0) {
			return true;
		}
	}

	surfacedata_t* pExitSurfaceData = g_csgo.m_phys_props->GetSurfaceData(exitTr.m_surface.m_surface_props);
	float flDamLostPercent = 0.16f;
	if (bIsGrate || bIsNodraw || iEnterMaterial == CHAR_TEX_GLASS || iEnterMaterial == CHAR_TEX_GRATE) {
		if (iEnterMaterial == CHAR_TEX_GLASS || iEnterMaterial == CHAR_TEX_GRATE) {
			flPenetrationModifier = 3.0f;
			flDamLostPercent = 0.05f;
		}
		else
			flPenetrationModifier = 1.0f;

		flDamageModifier = 0.99f;
	}
	else if (iEnterMaterial == CHAR_TEX_FLESH && ff_damage_reduction_bullets->GetFloat() == 0.f && tr.m_entity) {
		if (tr.m_entity->GetClientClass()->m_ClassID == CCSPlayer) {
			Player* hitEntity = (Player*)tr.m_entity;
			if (hitEntity->m_iTeamNum() == g_cl.m_local->m_iTeamNum()) {
				if (ff_damage_bullet_penetration->GetFloat() == 0.f) {
					flPenetrationModifier = 0.f;
					return true;
				}

				flPenetrationModifier = ff_damage_bullet_penetration->GetFloat();
				flDamageModifier = ff_damage_bullet_penetration->GetFloat();
			}
		}
	}
	else {
		float flExitPenetrationModifier = pExitSurfaceData->m_game.m_penetration_modifier;
		float flExitDamageModifier = pExitSurfaceData->m_game.m_damage_modifier;
		flPenetrationModifier = (flPenetrationModifier + flExitPenetrationModifier) / 2;
		flDamageModifier = (flDamageModifier + flExitDamageModifier) / 2;
	}

	if (iEnterMaterial == pExitSurfaceData->m_game.m_material) {
		if (pExitSurfaceData->m_game.m_material == CHAR_TEX_WOOD || pExitSurfaceData->m_game.m_material == CHAR_TEX_CARDBOARD) {
			flPenetrationModifier = 3.f;
		}
		else if (pExitSurfaceData->m_game.m_material == CHAR_TEX_PLASTIC) {
			flPenetrationModifier = 2.f;
		}
	}

	float flTraceDistance = (exitTr.m_endpos - tr.m_endpos).length();

	float flPenMod = std::max(0.f, (1.f / flPenetrationModifier));

	float flPercentDamageChunk = fCurrentDamage * flDamLostPercent;
	float flPenWepMod = flPercentDamageChunk + std::max(0.f, (3.f / flPenetrationPower) * 1.25f) * (flPenMod * 3.f);

	float flLostDamageObject = ((flPenMod * (flTraceDistance * flTraceDistance)) / 24.f);
	float flTotalLostDamage = flPenWepMod + flLostDamageObject;

	fCurrentDamage -= std::max(0.f, flTotalLostDamage);
	if (fCurrentDamage < 1.f)
		return true;

	flCurrentDistance += flTraceDistance;

	vecSrc = exitTr.m_endpos;
	flDistance = (flDistance - flCurrentDistance) * 0.5f;

	nPenetrationCount--;
	return false;
}

bool auto_wall::FireBullet(vec3_t srcPos, vec3_t endPos, Weapon* pWeapon, WeaponInfo* pWeaponInfo, Player* pTargetEntity, PenOutput_t* pOutput, bool retFirstDamage) {
	if (!pWeapon)
		return false;

	if (!pWeaponInfo)
		return false;

	/* set to base damage */
	pOutput->m_damage = static_cast<float>(pWeaponInfo->m_damage);

	int nPenetrationCount = 4;
	float flCurrentDistance = 0.f;

	float flPenetrationPower = pWeaponInfo->m_penetration;
	float flPenetrationDistance = 3000.f;
	float flDamageModifier = 0.5f;
	float flPenetrationModifier = 1.f;

	CGameTrace trace;
	static CTraceFilterSimple_game filter{};
	filter.m_pass_ent1 = g_cl.m_local;

	vec3_t vecDir = (endPos - srcPos).normalized();
	vec3_t vecEnd;

	while (pOutput->m_damage > 0.f && nPenetrationCount > 0) {
		vecEnd = srcPos + vecDir * (pWeaponInfo->m_range - flCurrentDistance);

		g_csgo.m_engine_trace->TraceRay(Ray(srcPos, vecEnd), MASK_SHOT, (ITraceFilter*)&filter, &trace); {
			// Check for player hitboxes extending outside their collision bounds
			UTIL_ClipTraceToPlayers(srcPos, vecEnd + vecDir * 40.0f, MASK_SHOT, (ITraceFilter*)&filter, &trace, pTargetEntity);
		}

		if (trace.m_fraction == 1.0f) /* we didnt hit anything so we should just break the loop */
			break;

		/* calculate the damage based on the distance the bullet has travelled */
		flCurrentDistance += trace.m_fraction * (pWeaponInfo->m_range - flCurrentDistance);
		pOutput->m_damage *= pow(pWeaponInfo->m_range_modifier, (flCurrentDistance / 500.f));

		pOutput->m_visible = false;
		if (trace.m_entity && trace.m_entity->IsPlayer()) {
			Player* hitEntity = (Player*)trace.m_entity;
			if (hitEntity && hitEntity->alive()) {
				if (pTargetEntity && pTargetEntity != hitEntity)
					break;

				pOutput->m_hitbox = trace.m_hitbox;
				pOutput->m_hitgroup = trace.m_hitgroup;
				pOutput->m_visible = (nPenetrationCount == 4);
				ScaleDamage(hitEntity, pWeaponInfo->m_armor_ratio, pOutput->m_damage, pOutput->m_hitgroup);
				return true;
			}
		}

		/* MATERIAL DETECTION */
		surfacedata_t* pSurfaceData = g_csgo.m_phys_props->GetSurfaceData(trace.m_surface.m_surface_props);

		flPenetrationModifier = pSurfaceData->m_game.m_penetration_modifier;
		flDamageModifier = pSurfaceData->m_game.m_damage_modifier;

		if ((flCurrentDistance > flPenetrationDistance && pWeaponInfo->m_penetration > 0.f) || flPenetrationModifier < 0.1f)
			break; /* break since we cant shoot through anything anymore */

		bool bulletStopped = HandleBulletPenetration(pWeaponInfo->m_penetration, pSurfaceData->m_game.m_material, trace, vecDir, pSurfaceData, flPenetrationModifier,
			flDamageModifier, flPenetrationPower, nPenetrationCount, srcPos, pWeaponInfo->m_range,
			flCurrentDistance, pOutput->m_damage);

		if (bulletStopped)
			break;

		if (retFirstDamage)
			return true;
	}

	return false;
}

PenOutput_t auto_wall::RunPenetration(const PenInput_t* input) {
	Weapon* weapon = nullptr;
	WeaponInfo* weaponInfo = nullptr;

	if (input->m_from == g_cl.m_local) {
		weapon = g_cl.m_weapon;
		weaponInfo = g_cl.m_weapon_info;
	}
	else {
		weapon = input->m_from->GetActiveWeapon();
		if (!weapon)
			return {};

		weaponInfo = weapon->GetWpnData();
		if (!weaponInfo)
			return {};
	}

	PenOutput_t output{};
	if (!FireBullet(input->m_start, input->m_end, weapon, weaponInfo, input->m_target, &output, !input->m_pen))
		return {};

	output.m_damage = (int)floorf(output.m_damage);
	return output;
}