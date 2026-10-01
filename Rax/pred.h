#pragma once

enum {
	ACCURACY_MOVEMENT_LADDER,
	ACCURACY_MOVEMENT_INAIR,
	ACCURACY_MOVEMENT_DUCKING,
	ACCURACY_MOVEMENT_STAND
};

class InputPrediction {
public:
	float m_curtime;
	float m_frametime;
	CMoveData m_move_data;
	bool m_first_time_predicted;
	bool m_in_prediction;

	float m_accuracy_penalty;
	float m_inaccuracy;
	float m_spread;

	float m_forward_move;
	float m_side_move;
public:
	float GetRecoveryTime( );
	float CalculateInaccuracy( const float& accuracy_penalty, const float& length2d, const float& z, const bool& primary );
	float CalculateWantedAccuracyPenalty( const bool& primary );

	void post_think( Player* player );
	void start_command( Player* player, CUserCmd* pCmd );
	void finish_command( Player* player );
	void run_prethink( Player* player );
	void run_think( Player* player );

	void update( );

	void repredict( );
	void run( );
	void restore( ) const;
};

struct RestoreVars {
	void Reset( )
	{
		m_aimPunchAngle = { 0.f, 0.f, 0.f };
		m_aimPunchAngleVel = { 0.f, 0.f, 0.f };
		m_viewPunchAngle = { 0.f, 0.f, 0.f };

		m_vecViewOffset = { 0.f, 0.f, 0.f };
		m_vecBaseVelocity = { 0.f, 0.f, 0.f };
		m_vecVelocity = { 0.f, 0.f, 0.f };
		m_vecAbsVelocity = { 0.f, 0.f, 0.f };
		m_vecOrigin = { 0.f, 0.f, 0.f };
		m_vecAbsOrigin = { 0.f, 0.f, 0.f };

		m_flFallVelocity = 0.0f;
		m_flVelocityModifier = 0.0f;
		m_flDuckAmount = 0.0f;
		m_flDuckSpeed = 0.0f;
		m_surfaceFriction = 0.0f;

		m_fAccuracyPenalty = 0.0f;
		m_flRecoilIndex = 0.f;
		m_fFlags = 0;

		m_GroundEntity = 0;
		m_MoveType = 0;
		m_nTickBase = 0;
	}

	bool is_filled = false;

	ang_t m_aimPunchAngle = { };
	ang_t m_aimPunchAngleVel = { };
	ang_t m_viewPunchAngle = { };

	vec3_t m_vecViewOffset = { };
	vec3_t m_vecBaseVelocity = { };
	vec3_t m_vecVelocity = { };
	vec3_t m_vecAbsVelocity = { };
	vec3_t m_vecOrigin = { };
	vec3_t m_vecAbsOrigin = { };

	float m_flFallVelocity = 0.0f;
	float m_flVelocityModifier = 0.0f;
	float m_flDuckAmount = 0.0f;
	float m_flDuckSpeed = 0.0f;
	float m_surfaceFriction = 0.0f;

	float m_fAccuracyPenalty = 0.0f;
	float m_flRecoilIndex = 0;

	int m_MoveType = 0;
	int m_nTickBase = 0;
	int m_fFlags = 0;

	EHANDLE m_GroundEntity = 0;
};

class RestoreData {
public:
	std::array<RestoreVars, 150> m_vars;

	__forceinline RestoreVars* GetVars( int command ) {
		return &m_vars[ command % 150 ];
	}
public:
	void Reset();
	void Setup( Player* player, int command );
	void Apply( Player* player, int command );
};

extern InputPrediction g_input_pred;
extern RestoreData g_restore_data;