#pragma once

class RebuiltAnimState {
public:
	Player* m_player;

	C_AnimationLayer m_layers[13];

	animstate_pose_param_cache_t m_pose_cache[20];
	float                        m_poses[20];

	ang_t m_angle;

	aimmatrix_transition_t m_stand_walk_aim;
	aimmatrix_transition_t m_stand_run_aim;
	aimmatrix_transition_t m_crouch_run_aim;

	float m_max_yaw_multiplier;

	float m_body_update;

	float m_speed_portion_run;
	float m_speed_portion_walk;
	float m_speed_portion_crouch;

	int m_walk_to_run_state;
	float m_walk_to_run;

	float m_foot_yaw, m_foot_yaw_last, m_body_yaw;
	bool m_adjust;

	float m_last_update;
	float m_increment;

	vec3_t m_pos, m_acceleration, m_target_acceleration;
	vec3_t m_old_pos;

	float m_acceleration_weight;

	vec3_t m_abs_vel, m_vel, m_vel_normalized, m_vel_normalized_non_zero, m_vel_last;
	float m_vel_xy, m_vel_z, m_vel_test_time;
	float m_max_speed;
	float m_duration_still;
	float m_duration_moving;

	float m_primary_cycle, m_move_weight, m_target_move_weight, m_stutter_step, m_move_yaw, m_move_yaw_ideal, m_move_yaw_to_ideal, m_air_smooth;
	int m_move_state;

	bool m_strafe_changed;
	float m_strafe_duration, m_strafe_weight, m_strafe_cycle, m_ground_height;
	int m_strafe_sequence;

	float m_duck_additive;
	float m_duck_amount;

	bool m_moving;

	bool m_ground;

	bool m_ladder;

	float m_ladder_weight;
	float m_ladder_speed;

	float m_spawn_time;

	bool m_landing;
	float m_landing_multiplier;

	bool m_jumping;
	float m_duration_in_air;

	Weapon* m_weapon;
	Weapon* m_old_weapon;
	bool m_deploy_rate_limiting;

	/* extra stuff */
	bool m_aa_flick;
	int m_buttons;

	/* anim event stuff */
	bool m_player_ducked;
	bool m_player_running;
public:
	__forceinline void Reset() {
		std::memset(this, 0, sizeof(RebuiltAnimState));
	}

	__forceinline void Correct(CCSGOPlayerAnimState* state) const {
		if (!state)
			return;

		state->m_weapon = m_weapon;
		state->m_landing = m_landing;
		state->m_land_anim_multiplier = m_landing_multiplier;
		state->m_ladder_weight = m_ladder_weight;
		state->m_ladder_speed = m_ladder_speed;
		state->m_duck_additional = m_duck_additive;
		state->m_anim_duck_amount = m_duck_amount;
		state->m_velocity = m_vel;
		state->m_walk_to_run_transition_state = m_walk_to_run_state;
		state->m_walk_run_transition = m_walk_to_run;
		state->m_speed_as_portion_of_run_top_speed = m_speed_portion_run;
		state->m_speed_as_portion_of_walk_top_speed = m_speed_portion_walk;
		state->m_speed_as_portion_of_crouch_top_speed = m_speed_portion_crouch;
		state->m_duration_in_air = m_duration_in_air;
		state->m_duration_still = m_duration_still;
		state->m_duration_moving = m_duration_moving;
		state->m_duration_strafing = m_strafe_duration;
		state->m_strafe_change_cycle = m_strafe_cycle;
		state->m_strafe_change_weight = m_strafe_weight;
		state->m_strafe_changing = m_strafe_changed;
		state->m_move_weight = m_move_weight;
		state->m_stutter_step = m_stutter_step;
		state->m_primary_cycle = m_primary_cycle;
		state->m_in_air_smooth_value = m_air_smooth;
		state->m_last_update_time = m_last_update;
	}

	__forceinline int SelectSequenceFromActivity(int activity) const
	{
		int cur_sequence = -1;

		switch (activity)
		{
		case 978:
		{
			cur_sequence = 223;
		}
		break;
		case 979:
		{
			cur_sequence = 4;
		}
		break;
		case 980:
		{
			cur_sequence = 5;
		}
		break;
		case 985:
		{
			cur_sequence = 15 + (int)m_player_running;
			if (m_player_ducked)
				cur_sequence = 17 + (int)m_player_running;
		}
		break;
		case 981:
		{
			cur_sequence = 8;
			if (m_old_weapon != m_weapon)
				cur_sequence = 9;
		}
		break;
		case 986:
		{
			cur_sequence = 15 + (int)m_player_running;
			if (m_player_ducked)
				cur_sequence = 17 + (int)m_player_running;
		}
		break;
		case 988:
		{
			cur_sequence = 20;
			if (m_player_running)
				cur_sequence = 22;

			if (m_player_ducked)
			{
				cur_sequence = 21;
				if (m_player_running)
					cur_sequence = 19;
			}
		}
		break;
		case 989:
		{
			cur_sequence = 23;
			if (m_player_ducked)
				cur_sequence = 24;
		}
		break;
		case 987:
		{
			cur_sequence = 13;
		}
		break;
		}

		return cur_sequence;
	}

	__forceinline void ModifyEyePosition(vec3_t& pos, BoneArray* bones) const {
		if (!m_player || !bones)
			return;

		if (!m_landing && (m_duck_amount == 0.f || std::isnan(m_duck_amount)) && m_player->GetGroundEntity())
			return;

		int nHeadBone = m_player->LookupBone(XOR("head_0"));

		if (nHeadBone != -1) {
			vec3_t vecHeadPos(
				bones[nHeadBone][0][3],
				bones[nHeadBone][1][3],
				bones[nHeadBone][2][3]);
			vecHeadPos.z += 1.7f;

			if (pos.z > vecHeadPos.z) {
				float flLerp = (abs(pos.z - vecHeadPos.z) - 4.f) / 6.f;
				flLerp = std::min(std::max(flLerp, 0.f), 1.f);

				float flLerpSqr = flLerp * flLerp;
				float flLerpCube = flLerp * flLerp * flLerp;
				pos.z = ((vecHeadPos.z - pos.z) * ((flLerpSqr * 3.f) - (flLerpCube * 2.f))) + pos.z;
			}
		}
	}

	// funny stuff valve.
	__forceinline void UpdateActivityModifiers() {
		m_player_ducked = m_duck_amount > 0.55f;
		m_player_running = m_speed_portion_walk > 0.25f;
	}

	void UpdateLayers(Player* player, const ang_t& angle, const float& curtime, bool apply_state, CUserCmd* cmd = nullptr);
	void SetAnimation(Player* player) const;
	void DoAnimStateEvent(CUserCmd* cmd, int event);
};

extern RebuiltAnimState g_fake_state;
extern RebuiltAnimState g_rendered_state;
extern RebuiltAnimState g_states[64];