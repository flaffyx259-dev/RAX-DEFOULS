#include "includes.h"

LagCompensation g_lagcomp{};;

LagRecord* LagCompensation::StartPrediction(AimPlayer* data) {
	if (data->m_records.size() <= 1)
		return nullptr;

	LagRecord* current = &data->m_records[0];
	LagRecord* previous = &data->m_records[1];

	// they wont update before our shot registers.
	if (current->m_server_tick + (current->m_server_tick - previous->m_server_tick) > g_cl.m_arrival_tick) {
		// ouf we cant hit any recent record.
		if (current->m_lagcomp[LC_LOST_TRACK] &&
			current->m_lagcomp[LC_TIME_DECREMENT] &&
			!current->ValidTime())
			return nullptr;

		return current;
	}

	LagRecord* predicted = &data->m_predicted.first;
	LagRecord* prev_predicted = &data->m_predicted.second;

	// we recieved a new update.
	if (predicted->m_server_tick != current->m_server_tick) {
		int choked_ticks = current->m_server_tick - previous->m_server_tick;
		int latency_ticks = g_cl.m_latency[INetChannel::FLOW_OUTGOING] + g_cl.m_charged_ticks;
		bool is_lagcompensated = !current->m_lagcomp[LC_LOST_TRACK] && !current->m_lagcomp[LC_TIME_DECREMENT];

		int ticks_to_predict = 0;
		while (ticks_to_predict < latency_ticks)
			ticks_to_predict += choked_ticks;

		if (ticks_to_predict) {
			// copy over newest data.
			std::memcpy(predicted, current, sizeof(LagRecord));

			// predict each movement tick.
			int predicted_ticks = 0;

			data->m_player->GetAnimLayers(m_layers);
			data->m_player->GetPoseParameters(m_poses);
			data->m_player->GetAnimState(&m_state);

			// reset this since we are gonna change our matrices.
			predicted->m_safe_count = 0;

			while (predicted_ticks < ticks_to_predict) {
				std::memcpy(prev_predicted, predicted, sizeof(LagRecord));

				PlayerMove(predicted);

				if (predicted_ticks % choked_ticks == 0)
					UpdatePredictedAnimations(data, predicted, prev_predicted, choked_ticks);

				++predicted_ticks;
			}

			data->m_player->SetAbsOrigin(m_abs_origin);
			data->m_player->SetAbsVelocity(m_abs_velocity);

			data->m_player->m_vecOrigin() = m_origin;
			data->m_player->m_vecVelocity() = m_velocity;
			data->m_player->m_flDuckAmount() = m_duck_amount;
			data->m_player->m_flDuckSpeed() = m_duck_speed;
			data->m_player->m_fFlags() = m_flags;

			data->m_player->SetAnimLayers(m_layers);
			data->m_player->SetPoseParameters(m_poses);
			data->m_player->SetAnimState(&m_state);

			g_csgo.m_globals->m_curtime = m_curtime;
			g_csgo.m_globals->m_frametime = m_frametime;

			predicted->m_lagcomp[LC_LOST_TRACK] = (predicted->m_origin - current->m_origin).length_sqr() > 4096.f;
		}
		// freak ass choking.
		else
			return nullptr;
	}

	// something went wrong.
	if (!predicted->m_predicted)
		return nullptr;

	// they are standing.
	if (current->m_abs_velocity.length_2d() <= 0.1f)
		return nullptr;

	// welp i guess no predicting.
	if (predicted->m_lagcomp[LC_LOST_TRACK] &&
		predicted->m_lagcomp[LC_TIME_DECREMENT] &&
		!predicted->ValidTime())
		return nullptr;

	// boi got predicted.
	return predicted;
}

void LagCompensation::PlayerMove(LagRecord* record) {
	vec3_t                start, end, normal;
	CGameTrace            trace;
	CTraceFilterWorldOnly filter;

	// predict gravity.
	if (!(record->m_flags & FL_ONGROUND))
		record->m_velocity.z -= g_csgo.sv_gravity->GetFloat() * g_csgo.m_globals->m_interval;

	// define trace start.
	start = record->m_origin;

	// move trace end one tick into the future using predicted velocity.
	end = start + (record->m_velocity * g_csgo.m_globals->m_interval);

	// trace.
	g_csgo.m_engine_trace->TraceRay(Ray(start, end, record->m_mins, record->m_maxs), CONTENTS_SOLID, &filter, &trace);

	// we hit shit
	// we need to fix hit.
	if (trace.m_fraction != 1.f) {

		// fix sliding on planes.
		for (int i{}; i < 2; ++i) {
			record->m_velocity -= trace.m_plane.m_normal * record->m_velocity.dot(trace.m_plane.m_normal);

			float adjust = record->m_velocity.dot(trace.m_plane.m_normal);
			if (adjust < 0.f)
				record->m_velocity -= (trace.m_plane.m_normal * adjust);

			start = trace.m_endpos;
			end = start + (record->m_velocity * (g_csgo.m_globals->m_interval * (1.f - trace.m_fraction)));

			g_csgo.m_engine_trace->TraceRay(Ray(start, end, record->m_mins, record->m_maxs), CONTENTS_SOLID, &filter, &trace);
			if (trace.m_fraction == 1.f)
				break;
		}
	}

	// set new final origin.
	start = end = record->m_origin = trace.m_endpos;

	// move endpos 2 units down.
	// this way we can check if we are in/on the ground.
	end.z -= 2.f;

	// trace.
	g_csgo.m_engine_trace->TraceRay(Ray(start, end, record->m_mins, record->m_maxs), CONTENTS_SOLID, &filter, &trace);

	// strip onground flag.
	record->m_flags &= ~FL_ONGROUND;

	// add back onground flag if we are onground.
	if (trace.m_fraction != 1.f && trace.m_plane.m_normal.z > 0.7f)
		record->m_flags |= FL_ONGROUND;
}

void LagCompensation::UpdatePredictedAnimations(AimPlayer* data, LagRecord* predicted, LagRecord* previous, int update_ticks) {
	CCSGOPlayerAnimState* state = data->m_player->m_PlayerAnimState();
	if (!state)
		return;

	predicted->m_resolver_mode = RESOLVE_PREDICTED;

	std::memcpy(predicted->m_safe_bones[predicted->m_safe_count].m_bones, previous->m_bones, sizeof(BoneArray) * 128);
	++predicted->m_safe_count;

	// store backup data.
	m_curtime = g_csgo.m_globals->m_curtime;
	m_frametime = g_csgo.m_globals->m_frametime;

	m_origin = data->m_player->m_vecOrigin();
	m_abs_origin = data->m_player->m_vecAbsOrigin();
	m_velocity = data->m_player->m_vecVelocity();
	m_abs_velocity = data->m_player->m_vecAbsVelocity();
	m_duck_amount = data->m_player->m_flDuckAmount();
	m_duck_speed = data->m_player->m_flDuckSpeed();
	m_flags = data->m_player->m_fFlags();

	// set our animations
	data->m_player->m_fFlags() = previous->m_flags;

	// TODO: predict these.
	data->m_player->m_flDuckAmount() = predicted->m_duck;
	data->m_player->m_flDuckSpeed() = predicted->m_duck_speed;

	// predict simulation time.
	predicted->m_time += game::TICKS_TO_TIME(update_ticks);
	// predict the recieved tick.
	predicted->m_server_tick += game::TICKS_TO_TIME(update_ticks);

	predicted->m_choke = update_ticks;

	if (predicted->m_choke >= 2) {
		const float fraction = 1.f / predicted->m_choke;

		predicted->m_abs_origin = math::Lerp(fraction, previous->m_origin, predicted->m_origin);
		predicted->m_abs_velocity = math::Lerp(fraction, previous->m_velocity, predicted->m_velocity);
		predicted->m_anim_time = previous->m_time + g_csgo.m_globals->m_interval;
	}

	data->m_player->m_iEFlags() &= ~(EFL_DIRTY_ABSTRANSFORM | EFL_DIRTY_ABSVELOCITY);

	data->m_player->SetAbsOrigin(predicted->m_abs_origin);
	data->m_player->SetAbsVelocity(predicted->m_abs_velocity);
	data->m_player->SetAnimLayers(previous->m_layers);

	g_csgo.m_globals->m_curtime = predicted->m_anim_time;
	g_csgo.m_globals->m_frametime = g_csgo.m_globals->m_interval;

	update_clientside_animation::allow = true;
	game::UpdateAnimationState(state, predicted->m_angles);
	update_clientside_animation::allow = false;

	data->m_player->SetAbsOrigin(predicted->m_origin);

	g_bones.Setup(data->m_player, BONE_USED_BY_ANYTHING, predicted->m_time, predicted->m_bones);
	predicted->UpdateBounds();

	predicted->m_predicted = true;
}