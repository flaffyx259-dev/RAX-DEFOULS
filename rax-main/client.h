#pragma once

class Sequence {
public:
	float m_time;
	int   m_state;
	int   m_seq;

public:
	__forceinline Sequence() : m_time{}, m_state{}, m_seq{} {};
	__forceinline Sequence(float time, int state, int seq) : m_time{ time }, m_state{ state }, m_seq{ seq } {};
};

class NetPos {
public:
	float  m_time;
	vec3_t m_pos;

public:
	__forceinline NetPos() : m_time{}, m_pos{} {};
	__forceinline NetPos(float time, vec3_t pos) : m_time{ time }, m_pos{ pos } {};
};

class RebuiltAnimState;

class Client {
public:
	// hack thread.
	static ulong_t __stdcall init(void* arg);

	void SetupShootPos(float pitch = 0.f);

	void StartMove(CUserCmd* cmd);
	void EndMove(CUserCmd* cmd);
	void BackupPlayers(bool restore);
	void DoMove();
	void DrawHUD();
	void HandleRenderedAnimation(const RebuiltAnimState* rebuilt_state);
	void UpdateInformation(RebuiltAnimState* rebuilt_state, CUserCmd* cmd, const float& curtime);
	void SetAnimations(const RebuiltAnimState* rebuilt_state) const;
	void KillFeed();

	void OnPaint();
	void OnMapload();
	void OnTick(CUserCmd* cmd);

	// debugprint function.
	void print(const std::string text, ...);

	// check if we are able to fire this tick.
	bool CanFireWeapon();
	void UpdateRevolverCock();
	void UpdateIncomingSequences();
	bool PredictLand(int ticks);

public:
	BoneArray        m_fake_bones[128];

	// local player variables.
	Player*          m_local;
	bool	         m_processing;
	vec3_t	         m_origin;
	vec3_t	         m_shoot_pos;
	bool	         m_player_fire;
	bool	         m_shot;
	bool	         m_old_shot;
	C_AnimationLayer m_layers[13];

	// active weapon variables.
	Weapon* m_weapon;
	int         m_weapon_id;
	WeaponInfo* m_weapon_info;
	int         m_weapon_type;
	bool        m_weapon_fire;

	// revolver variables.
	int	 m_revolver_cock;
	int	 m_revolver_query;
	bool m_revolver_fire;

	// general game varaibles.
	bool     m_round_end;
	Stage_t	 m_stage;
	int	     m_max_lag;
	int	     m_old_lag;
	bool     m_packet;
	bool	 m_old_packet;
	float	 m_lerp;
	float    m_latency[2];
	int      m_latency_ticks[2];
	int      m_width, m_height;

	// usercommand variables.
	CUserCmd* m_cmd;
	RebuiltAnimState* m_state;
	int	      m_tick;
	int	      m_buttons;
	int       m_old_buttons;
	ang_t     m_view_angles;
	ang_t	  m_strafe_angles;
	vec3_t	  m_forward_dir;
	int       m_arrival_tick;

	PenOutput_t m_pen_data;

	std::deque< Sequence > m_sequences;
	std::deque< NetPos >   m_net_pos;

	// animation variables.
	bool   m_ground;
	vec3_t m_anim_velocity;
	ang_t  m_angle;
	ang_t  m_radar;
	float  m_foot_yaw;
	float  m_body;
	float  m_body_update;
	bool   m_lagcomp;
	float  m_curtime;
	float  m_last_update;
	int    m_charged_ticks;

	int    m_taps;
};

extern Client g_cl;