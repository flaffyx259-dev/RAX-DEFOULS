#include "includes.h"

HVH g_hvh{ };;

void HVH::AutoDirection() {
	// constants.
	constexpr float STEP{ 4.f };
	constexpr float RANGE{ 32.f };

	if (!m_target)
		return;

	ang_t to_local;
	math::VectorAngles(g_cl.m_local->m_vecOrigin() - m_target->m_vecOrigin(), to_local);

	/*
	* data struct
	* 68 74 74 70 73 3a 2f 2f 73 74 65 61 6d 63 6f 6d 6d 75 6e 69 74 79 2e 63 6f 6d 2f 69 64 2f 73 69 6d 70 6c 65 72 65 61 6c 69 73 74 69 63 2f
	*/

	// construct vector of angles to test.
	std::vector< AdaptiveAngle > angles{ };
	angles.emplace_back(to_local.y);
	angles.emplace_back(to_local.y + 90.f);
	angles.emplace_back(to_local.y - 90.f);

	// start the trace at the enemy shoot pos.
	vec3_t start = m_target->GetEyePos();

	// see if we got any valid result.
	// if this is false the path was not obstructed with anything.
	bool valid{ false };

	// iterate vector of angles.
	for (auto it = angles.begin(); it != angles.end(); ++it) {

		// compute the 'rough' estimation of where our head will be.
		vec3_t end{ g_cl.m_shoot_pos.x + std::cos(math::deg_to_rad(it->m_yaw)) * RANGE,
			g_cl.m_shoot_pos.y + std::sin(math::deg_to_rad(it->m_yaw)) * RANGE,
			g_cl.m_shoot_pos.z };

		// draw a line for debugging purposes.
		//g_csgo.m_debug_overlay->AddLineOverlay( start, end, 255, 0, 0, true, 0.1f );

		// compute the direction.
		vec3_t dir = end - start;
		float len = dir.normalize();

		// should never happen.
		if (len <= 0.f)
			continue;

		// step thru the total distance, 4 units per step.
		for (float i{ 0.f }; i < len; i += STEP) {
			// get the current step position.
			vec3_t point = start + (dir * i);

			// get the contents at this point.
			int contents = g_csgo.m_engine_trace->GetPointContents(point, MASK_SHOT_HULL);

			// contains nothing that can stop a bullet.
			if (!(contents & MASK_SHOT_HULL))
				continue;

			float mult = 1.f;

			// over 50% of the total length, prioritize this shit.
			if (i > (len * 0.5f))
				mult = 1.25f;

			// over 90% of the total length, prioritize this shit.
			if (i > (len * 0.75f))
				mult = 1.25f;

			// over 90% of the total length, prioritize this shit.
			if (i > (len * 0.9f))
				mult = 2.f;

			// append 'penetrated distance'.
			it->m_dist += (STEP * mult);

			// mark that we found anything.
			valid = true;
		}
	}

	if (!valid)
		return;

	// put the most distance at the front of the container.
	std::sort(angles.begin(), angles.end(),
		[](const AdaptiveAngle& a, const AdaptiveAngle& b) {
			return a.m_dist > b.m_dist;
		});

	// the best angle should be at the front now.
	AdaptiveAngle* best = &angles.front();
	m_direction = math::NormalizedAngle(best->m_yaw);
}

void HVH::HandleDirection() {
	m_direction = m_view;

	if (g_menu.main.antiaim.yaw.get(0))
		m_direction += 180.f;

	if (g_menu.main.antiaim.yaw.get(1))
		AutoDirection();

	if (m_front)
		m_direction = m_view;

	if (m_back)
		m_direction = m_view + 180.f;

	if (m_right)
		m_direction = m_view - 90.f;

	if (m_left)
		m_direction = m_view + 90.f;

	math::NormalizeAngle(m_direction);
}

void HVH::HandleReal() {
	if (m_desync) {
		float fake_yaw = math::NormalizedAngle(g_fake_state.m_foot_yaw);
		float real_yaw = math::NormalizedAngle(g_cl.m_state->m_foot_yaw);
		float foot_delta = math::NormalizedAngle(fake_yaw - real_yaw);

		if (!m_on_ground && (g_cl.m_local->m_fFlags() & FL_ONGROUND)) {
			g_cl.m_cmd->m_view_angles.y = g_fake_state.m_body_yaw + (foot_delta > 0.0f ? -120.f : 120.f);
			g_hvh.m_hide_yaw = true;
			return;
		}

		if (m_can_break) {
			g_cl.m_cmd->m_view_angles.y = m_direction + (foot_delta > 0.0f ? -120.f : 120.f);
			g_hvh.m_hide_yaw = true;
			return;
		}

		// call animstate rebuild to calculate max delta multiplier.
		RebuiltAnimState state_copy = *g_cl.m_state;
		state_copy.UpdateLayers(g_cl.m_local, { 0.f, 0.f, 0.f }, g_cl.m_curtime, false, g_cl.m_cmd);

		float approach = state_copy.m_increment * (30.0f + 20.0f * g_cl.m_state->m_walk_to_run);
		float abs_fake_delta = std::max(std::min(fabs(foot_delta), 116.f * state_copy.m_max_yaw_multiplier) - approach * 2.f, 0.f);

		if (abs_fake_delta <= 58.f) {
			g_cl.m_cmd->m_view_angles.y = math::ApproachAngle(real_yaw, fake_yaw, abs_fake_delta / 1.5f) + 180.f;
			return;
		}

		float average_yaw = math::ApproachAngle(real_yaw, fake_yaw, fabs(foot_delta / 2.f));

		if (fabs(math::NormalizedAngle(m_direction - average_yaw)) > 90.f)
			average_yaw -= 180.f;

		math::NormalizeAngle(average_yaw);

		g_cl.m_cmd->m_view_angles.y = math::ApproachAngle(m_direction, average_yaw, 29.f * state_copy.m_max_yaw_multiplier);
		return;
	}

	if (!m_on_ground && g_menu.main.antiaim.avoid_overlap.get(1)) {
		ang_t velocity_dir;
		math::VectorAngles(g_cl.m_local->m_vecVelocity(), velocity_dir);

		velocity_dir.y += 180.f;

		math::NormalizeAngle(velocity_dir.y);

		float delta = math::NormalizedAngle(velocity_dir.y - g_cl.m_body);

		g_cl.m_cmd->m_view_angles.y = math::ApproachAngle(velocity_dir.y, g_cl.m_body, fabs(delta) * ((rand() % 360) / 360.f)) + 180;
		return;
	}

	if (!m_can_break && m_on_ground) {
		float fake_yaw = math::NormalizedAngle(g_fake_state.m_foot_yaw);
		float real_yaw = math::NormalizedAngle(g_cl.m_state->m_foot_yaw);
		float foot_delta = math::NormalizedAngle(fake_yaw - real_yaw);
		float target_delta = math::NormalizedAngle(m_direction - g_cl.m_state->m_body_yaw);

		if (fabs(foot_delta) > 25.f && fabs(target_delta) <= 90.f) {
			// add a slight tilt to the angle.
			m_direction = math::NormalizedAngle(m_direction + 29.f * (foot_delta / fabs(foot_delta)));

			// call animstate rebuild to calculate max delta multiplier.
			RebuiltAnimState state_copy = *g_cl.m_state;
			state_copy.UpdateLayers(g_cl.m_local, { 0.f, 0.f, 0.f }, g_cl.m_curtime, false, g_cl.m_cmd);

			float twist_yaws[] = { fake_yaw, real_yaw };

			// the speed that footyaw approaches with.
			float approach = state_copy.m_increment * (30.0f + 20.0f * g_cl.m_state->m_walk_to_run);

			twist_yaws[1] = twist_yaws[0] + (math::NormalizedAngle(twist_yaws[1] - twist_yaws[0]) / foot_delta) * (58.f + approach) * state_copy.m_max_yaw_multiplier;

			// normalize after extending it.
			math::NormalizeAngle(twist_yaws[1]);

			int desired_yaw_index = 0;

			// do da jitter thang.
			if (float delta = math::NormalizedAngle(m_direction - twist_yaws[desired_yaw_index]); delta < approach)
				desired_yaw_index = 1 - desired_yaw_index;

			// we can normally rotate torwards our target yaw.
			g_cl.m_cmd->m_view_angles.y = math::ApproachAngle(m_direction, twist_yaws[desired_yaw_index], approach);
			return;
		}
	}

	if (m_can_break) {
		float delta = math::NormalizedAngle(m_direction - g_cl.m_body);
		if (float delta2 = math::NormalizedAngle(m_view - g_cl.m_body); fabs(delta2) > 45.f && fabs(delta2) < 135.f && fabs(delta) <= g_menu.main.antiaim.minimum_delta_sideways.get()) {
			g_cl.m_cmd->m_view_angles.y = g_cl.m_body + (delta > 0.0f ? g_menu.main.antiaim.minimum_delta_sideways.get() : -g_menu.main.antiaim.minimum_delta_sideways.get());
			return;
		}

		if (fabs(delta) <= g_menu.main.antiaim.minimum_delta.get()) {
			g_cl.m_cmd->m_view_angles.y = g_cl.m_body + (delta > 0.0f ? g_menu.main.antiaim.minimum_delta.get() : -g_menu.main.antiaim.minimum_delta.get());
			return;
		}
	}

	g_cl.m_cmd->m_view_angles.y = m_direction;
}

bool HVH::HandleLBY() {
	// set our flick yaw to lby just in case we cant preflick.
	if (!m_can_break || (g_cl.m_curtime + game::TICKS_TO_TIME(2)) <= g_cl.m_body_update) {
		m_flick_yaw = math::NormalizedAngle(g_cl.m_body);

		// here we will have a little fun.
		if (m_will_land || (m_can_break && (m_last_move_time + 0.22f) > g_cl.m_curtime)) {
			g_hvh.m_hide_yaw = true;

			float lby_to_direction = math::NormalizedAngle(m_direction - m_flick_yaw);

			// we want to flick a big amount in order to desync our footyaw as much as we can.
			if (lby_to_direction >= 0.f) {
				g_cl.m_cmd->m_view_angles.y = math::NormalizedAngle(m_flick_yaw + 120.f);
				return true;
			}

			g_cl.m_cmd->m_view_angles.y = math::NormalizedAngle(m_flick_yaw - 120.f);
			return true;
		}

		return false;
	}

	g_hvh.m_hide_yaw = true;

	// our lby should update.
	float time_delta = g_cl.m_curtime - g_cl.m_body_update;
	if (time_delta > 0.0f) {
		if (m_updates >= std::max(m_mode - 1, 0) && game::TIME_TO_TICKS(time_delta) <= g_menu.main.antiaim.maximum_delay_ticks.get()) {
			// delay our lby update.
			float approach = (g_cl.m_curtime - g_cl.m_state->m_last_update) * 100.f;
			float predicted_foot_yaw = math::ApproachAngle(g_cl.m_body, g_cl.m_state->m_foot_yaw, approach);
			float delta = math::NormalizedAngle(m_flick_yaw - math::NormalizedAngle(predicted_foot_yaw));

			// they will have some fun.
			if (fabs(delta) > (35.f + approach)) {
				g_cl.m_cmd->m_view_angles.y = math::ApproachAngle(m_flick_yaw + 180.f, predicted_foot_yaw, 25.f);
				return true;
			}
		}

		g_cl.m_cmd->m_view_angles.y = m_flick_yaw;

		if (m_updates == 0 && m_mode == 2)
			g_cl.m_cmd->m_view_angles.x = -73.5f;

		return true;
	}

	// make sure we can flick and choose our flick angle.
	switch (m_mode) {
		// exploit.
	case 2:
		m_flick_yaw = (m_updates == 0 ? (m_direction + 180.f) : g_cl.m_body);
		break;
		// default flick yaw.
	default:
		m_flick_yaw = g_cl.m_body;
		break;
	}

	math::NormalizeAngle(m_flick_yaw);

	float approach = (g_cl.m_curtime - g_cl.m_state->m_last_update) * 100.f;
	float preflick_add = 116.f + approach;

	// find best preflick angle.
	std::vector<float> angles;
	angles.emplace_back(math::NormalizedAngle(m_flick_yaw - preflick_add));
	angles.emplace_back(math::NormalizedAngle(m_flick_yaw + preflick_add));

	std::sort(angles.begin(), angles.end(),
		[&](const float& a, const float& b) {
			const float& dist_a = fabs(math::NormalizedAngle(math::NormalizedAngle(a) - math::NormalizedAngle(g_cl.m_angle.y)));
			const float& dist_b = fabs(math::NormalizedAngle(math::NormalizedAngle(b) - math::NormalizedAngle(g_cl.m_angle.y)));

			return dist_a > dist_b;
		});

	g_cl.m_cmd->m_view_angles.y = angles.front();

	return true;
}

void HVH::AntiAim() {
	m_mode = (m_front + m_back + m_right + m_left);

	m_on_ground = g_cl.m_state->m_ground;
	m_will_land = !m_on_ground && (g_cl.m_local->m_fFlags() & FL_ONGROUND);
	m_can_break = g_cl.m_anim_velocity.length_2d() <= 0.1f && m_on_ground;

	if (!g_menu.main.antiaim.enable.get())
		return;

	// disable conditions.
	if (g_csgo.m_gamerules->m_bFreezePeriod() || g_cl.m_shot || (g_cl.m_local->m_fFlags() & FL_FROZEN) || g_cl.m_round_end || (g_cl.m_cmd->m_buttons & IN_USE))
		return;

	m_view = g_cl.m_view_angles.y;

	switch (g_menu.main.antiaim.pitch.get()) {
	case 1:
		g_cl.m_cmd->m_view_angles.x = 89.f;
		break;
	case 2:
		g_cl.m_cmd->m_view_angles.x = -89.f;
		break;
	}

	float min_fov = FLT_MAX;
	vec3_t target_pos;

	m_target = nullptr;
	for (int i = 1; i <= g_csgo.m_globals->m_max_clients; i++) {
		Player* player = g_csgo.m_entlist->GetClientEntity<Player*>(i);
		if (!g_aimbot.IsValidTarget(player))
			continue;

		target_pos = player->GetEyePos();

		const float& fov = math::GetFOV(g_cl.m_view_angles, g_cl.m_shoot_pos, target_pos);
		if (fov < min_fov) {
			m_target = player;
			min_fov = fov;
		}
	}

	if (g_menu.main.antiaim.yaw.get(2) && m_target) {
		ang_t to_player;
		math::VectorAngles(m_target->m_vecOrigin() - g_cl.m_local->m_vecOrigin(), to_player);

		m_view = to_player.y;
	}

	math::NormalizeAngle(m_view);

	HandleDirection();
	if (g_csgo.m_cl->m_choked_commands > 0) {
		if (m_can_break) {
			g_cl.m_cmd->m_view_angles.y = g_cl.m_body;
		}
		else {
			g_cl.m_cmd->m_view_angles.y = m_direction + 180.f;
		}
	}
	else {
		if (!HandleLBY())
			HandleReal();
	}

	math::NormalizeAngle(g_cl.m_cmd->m_view_angles.y);
}

bool HVH::IsPeeking() {
	if (!g_cl.m_weapon || !g_cl.m_weapon_info) {
		return 0;
	}

	if (g_cl.m_local->m_vecVelocity().length() <= 10.f || g_movement.m_fakewalk) {
		return 0;
	}

	static PenInput_t in;

	vec3_t extrapolated = g_cl.m_shoot_pos;
	extrapolated += g_cl.m_local->m_vecVelocity().normalized() * 20.f;

	//g_csgo.m_debug_overlay->AddBoxOverlay(extrapolated, { -2, -2, -2 }, { 2, 2, 2 }, { 0, 0, 0 }, 255, 0, 0, 127, g_csgo.m_globals->m_interval * 4);

	std::vector<int> hitboxes;

	hitboxes.emplace_back(HITBOX_HEAD);
	hitboxes.emplace_back(HITBOX_L_FOREARM);
	hitboxes.emplace_back(HITBOX_R_FOREARM);
	hitboxes.emplace_back(HITBOX_L_FOOT);
	hitboxes.emplace_back(HITBOX_R_FOOT);

	// iterate all targets.
	for (int i{ 1 }; i <= g_csgo.m_globals->m_max_clients; ++i) {
		Player* player = g_csgo.m_entlist->GetClientEntity< Player* >(i);
		if (!player || !player->alive() || player->dormant() || !player->enemy(g_cl.m_local) || player->m_bIsLocalPlayer())
			continue;

		AimPlayer* data = &g_aimbot.m_players[i - 1];
		if (!data)
			continue;

		if (data->m_records.empty())
			continue;

		LagRecord* ideal = &data->m_records.front();
		if (!ideal)
			continue;

		ideal->SetData();
		for (const int& hb : hitboxes) {
			const model_t* model = player->GetModel();
			if (!model)
				continue;

			studiohdr_t* hdr = g_csgo.m_model_info->GetStudioModel(model);
			if (!hdr)
				continue;

			mstudiohitboxset_t* set = hdr->GetHitboxSet(player->m_nHitboxSet());
			if (!set)
				continue;

			mstudiobbox_t* bbox = set->GetHitbox(hb);
			if (!bbox)
				continue;

			static vec3_t point;

			if (bbox->m_radius != -1) {
				point = (bbox->m_maxs + bbox->m_mins) / 2.f;
				math::VectorTransform(point, ideal->m_bones[bbox->m_bone], point);
			}
			else {
				static matrix3x4_t rot_matrix;
				g_csgo.AngleMatrix(bbox->m_angle, rot_matrix);

				// apply the rotation to the entity input space (local).
				static matrix3x4_t matrix;
				math::ConcatTransforms(ideal->m_bones[bbox->m_bone], rot_matrix, matrix);

				// extract origin from matrix.
				vec3_t origin = matrix.GetOrigin();

				// compute raw center point.
				point = (bbox->m_mins + bbox->m_maxs) / 2.f;

				point = { point.dot(matrix[0]), point.dot(matrix[1]), point.dot(matrix[2]) };

				// transform point to world space.
				point += origin;
			}

			in.m_start = extrapolated;
			in.m_end = point;
			in.m_from = g_cl.m_local;
			in.m_target = player;
			in.m_pen = true;

			if (auto_wall::RunPenetration(&in).m_damage > 0) {
				return true;
			}
		}
	}

	return false;
}

void HVH::SendPacket() {
	m_old_peeking = m_peeking;
	m_peeking = g_menu.main.antiaim.choke_on_peek.get() > 0 && IsPeeking();

	const vec3_t& origin = g_cl.m_local->m_vecOrigin();
	const vec3_t& sent_origin = g_cl.m_net_pos.empty() ? g_cl.m_local->m_vecOrigin() : g_cl.m_net_pos.front().m_pos;

	if (m_peeking) {
		if (!m_old_peeking) {
			g_cl.m_packet = g_csgo.m_cl->m_choked_commands >= 1;
			return;
		}

		// choke the max we can.
		if (g_menu.main.antiaim.choke_on_peek.get() == 1) {
			g_cl.m_packet = g_csgo.m_cl->m_choked_commands >= g_cl.m_max_lag;
			return;
		}

		// :skull:
		g_cl.m_charged_ticks = std::max(g_cl.m_max_lag - g_csgo.m_cl->m_choked_commands, 0);
	}

	if (g_cl.m_charged_ticks >= 1) {
		g_cl.m_packet = false;
		return;
	}

	bool on_ground = g_cl.m_local->m_fFlags() & FL_ONGROUND;

	// delay our landing.
	if (on_ground && !g_cl.m_state->m_ground) {
		g_cl.m_packet = g_csgo.m_cl->m_choked_commands >= g_cl.m_max_lag;
		return;
	}

	// unchoke on jump to prevent getting headshot.
	if (!on_ground && g_cl.m_state->m_ground) {
		g_cl.m_packet = g_csgo.m_cl->m_choked_commands >= 1;
		return;
	}

	// no need to choke much since we will break lc.
	if ((origin - sent_origin).length_sqr() > 4096.f) {
		g_cl.m_packet = g_csgo.m_cl->m_choked_commands >= 1;
		return;
	}

	// we are standing no need to choke more than one packet..
	if (g_cl.m_anim_velocity.length_2d() <= 0.1f && on_ground) {
		g_cl.m_packet = g_csgo.m_cl->m_choked_commands >= 1;
		return;
	}

	if (on_ground) {
		if (g_cl.m_state->m_ground && g_movement.m_stopping && (m_last_move_time + 0.22f) >= g_cl.m_curtime) {
			g_cl.m_packet = g_csgo.m_cl->m_choked_commands >= 1;
			return;
		}

		float real_yaw = math::NormalizedAngle(g_cl.m_state->m_foot_yaw);
		float fake_yaw = math::NormalizedAngle(g_fake_state.m_foot_yaw);

		if (float foot_delta = math::NormalizedAngle(fake_yaw - real_yaw); fabs(foot_delta) >= 25.f) {
			g_cl.m_packet = g_csgo.m_cl->m_choked_commands >= 1;
			return;
		}
	}

	g_cl.m_packet = g_csgo.m_cl->m_choked_commands >= g_menu.main.antiaim.choke.get();
}

void HVH::HandleShotChoke() {
	// hehehehe.
	if (g_cl.m_charged_ticks > 0)
		return;

	if (g_cl.m_shot && g_csgo.m_cl->m_choked_commands < g_cl.m_max_lag)
		g_cl.m_packet = false;

	if (g_cl.m_old_shot && !g_cl.m_old_packet)
		g_cl.m_packet = true;
}

void HVH::MicroMovement() {
	vec3_t velocity{ g_cl.m_local->m_vecVelocity() };

	static ang_t dir;
	if (velocity.length_2d() > 0.0f)
		math::VectorAngles(velocity, dir);

	if (velocity.length_2d() > 5.f)
		return;

	if (g_csgo.m_cl->m_choked_commands > 0 || !m_desync)
		return;

	const float yaw = math::NormalizedAngle(g_cl.m_state->m_foot_yaw);
	const float fake_yaw = math::NormalizedAngle(g_fake_state.m_foot_yaw);
	const float foot_delta = math::NormalizedAngle(yaw - fake_yaw);

	const RebuiltAnimState* state = &g_states[g_cl.m_local->index() - 1];
	if (!state || (fabs(foot_delta) <= 45.f && state->m_vel_xy >= 1.1f && state->m_ground))
		return;

	float max_speed = 250.f;
	if (g_cl.m_weapon && g_cl.m_weapon_info)
		max_speed = g_cl.m_weapon->m_zoomLevel() >= 1 ? g_cl.m_weapon_info->m_max_player_speed_alt : g_cl.m_weapon_info->m_max_player_speed;

	float speed = (250.f / max_speed) + 1.1f;

	g_cl.m_cmd->m_buttons &= ~IN_SPEED;

	ang_t angle = dir;
	angle.y = g_cl.m_view_angles.y - dir.y;

	// convert corrected angle back to a direction.
	vec3_t direction;
	math::AngleVectors(angle, &direction);

	vec3_t stop = direction * -speed;

	g_cl.m_cmd->m_forward_move = stop.x;
	g_cl.m_cmd->m_side_move = stop.y;
}