#pragma once

struct PenInput_t {
	vec3_t  m_start;
	vec3_t  m_end;
	Player* m_from;
	Player* m_target;
	bool    m_pen;

	PenInput_t() {
		Reset();
	}

	void Reset() {
		m_start = { 0, 0, 0 };
		m_end = { 0, 0, 0 };
		m_from = nullptr;
		m_target = nullptr;
		m_pen = false;
	}
};

struct PenOutput_t {
	float m_damage;
	int   m_hitgroup;
	int   m_hitbox;
	bool  m_visible;

	PenOutput_t() {
		Reset();
	}

	void Reset() {
		m_damage = 0.f;
		m_hitgroup = -1;
		m_hitbox = -1;
		m_visible = false;
	}
};


namespace auto_wall {
	float DistanceToRay(vec3_t pos, vec3_t rayStart, vec3_t rayEnd);
	void UTIL_ClipTraceToPlayers(vec3_t vecAbsStart, vec3_t vecAbsEnd, unsigned int mask, ITraceFilter* filter, CGameTrace* tr, Player* pTargetEntity);
	void ScaleDamage(Player* player, const float& armorRatio, float& currentDamage, const int& hitGroup);
	bool IsBreakableEntity(Entity* pEnt);
	static bool TraceToExit(vec3_t start, vec3_t dir, vec3_t& end, CGameTrace& trEnter, CGameTrace& trExit, float flStepSize, float flMaxDistance);
	bool HandleBulletPenetration(float& flPenetration,
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
		float& fCurrentDamage);
	bool FireBullet(vec3_t srcPos, vec3_t endPos, Weapon* pWeapon, WeaponInfo* pWeaponInfo, Player* pTargetEntitybool, PenOutput_t* pOutput, bool retFirstDamage = false);

	/* use functions */
	PenOutput_t RunPenetration(const PenInput_t* input);
}