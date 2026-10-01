#pragma once

class ShotRecord;

struct OverrideData
{
	float m_yaw;
	vec3_t m_start, m_end;
	Player* m_player;
};

class Resolver {
public:
	bool          m_override, m_override_update;
	ang_t         m_override_angle;

	OverrideData  m_override_data;
public:
	float AutoDirection(Player* player, std::vector<AdaptiveAngle> angles = {});
	float FindBestYaw(LagRecord* record, MoveData_t* move_data);

	void MatchShot(AimPlayer* data, LagRecord* current, LagRecord* previous);
	void HandleModes(AimPlayer* data, LagRecord* current, LagRecord* previous);
	void ResolveAngles(AimPlayer* data, LagRecord* current, LagRecord* previous);
	void ResolveMove(AimPlayer* data, LagRecord* current, LagRecord* previous);
	void ResolveStand(AimPlayer* data, LagRecord* current, LagRecord* previous);
	bool HandleUpdates(AimPlayer* data, LagRecord* current, LagRecord* previous);
	void ResolveAir(AimPlayer* data, LagRecord* current, LagRecord* previous);
	void Override();
};

extern Resolver g_resolver;