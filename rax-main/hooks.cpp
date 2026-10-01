#include "includes.h"
#include "minhook/minhook.h"

Hooks                g_hooks{ };;

void Force_proxy(CRecvProxyData* data, Address ptr, Address out) {
	// convert to ragdoll.
	Ragdoll* ragdoll = ptr.as< Ragdoll* >();

	// get ragdoll owner.
	Player* player = ragdoll->GetPlayer();

	// we only want this happening to noobs we kill.
	if (g_menu.main.misc.ragdoll_force.get() && g_cl.m_local && player && player->enemy(g_cl.m_local)) {
		// get m_vecForce.
		vec3_t vel = { data->m_Value.m_Vector[0], data->m_Value.m_Vector[1], data->m_Value.m_Vector[2] };

		// give some speed to all directions.
		vel *= 1000.f;

		// boost z up a bit.
		if (vel.z <= 1.f)
			vel.z = 2.f;

		vel.z *= 2.f;

		// don't want crazy values for this... probably unlikely though?
		math::clamp(vel.x, std::numeric_limits< float >::lowest(), std::numeric_limits< float >::max());
		math::clamp(vel.y, std::numeric_limits< float >::lowest(), std::numeric_limits< float >::max());
		math::clamp(vel.z, std::numeric_limits< float >::lowest(), std::numeric_limits< float >::max());

		// set new velocity.
		data->m_Value.m_Vector[0] = vel.x;
		data->m_Value.m_Vector[1] = vel.y;
		data->m_Value.m_Vector[2] = vel.z;
	}

	if (g_hooks.m_Force_original)
		g_hooks.m_Force_original(data, ptr, out);
}

//void __vectorcall update_animstate::hook(CCSGOPlayerAnimState* this_, void* a, float b, float yaw, float pitch, void* unk)
//{
//	if (!allow)
//		return;
//
//	original(this_, a, b, yaw, pitch, unk);
//}

void __fastcall call_post_data_updates::hook(void* ecx, void* edx) {

	CEntityReadInfo* u = reinterpret_cast<CEntityReadInfo*>(ecx);
	if (!u)
		return original(ecx, edx);

	original(ecx, edx);

	for (int i = 0; i < u->m_nPostDataUpdateCalls; i++)
	{
		CPostDataUpdateCall* pCall = &u->m_PostDataUpdateCalls[i];
		if (!pCall || pCall->m_iEnt < 1 || pCall->m_iEnt > 64)
			continue;

		AimPlayer* data = &g_aimbot.m_players[pCall->m_iEnt - 1];
		if (!data)
			continue;

		Player* player = g_csgo.m_entlist->GetClientEntity<Player*>(pCall->m_iEnt);

		if (player->m_bIsLocalPlayer())
			g_cl.m_local->GetAnimLayers(g_cl.m_layers);

		data->OnNetUpdate(player);
	}
}

ang_t* __fastcall get_eye_angles::hook(void* ecx, void* edx) {
	Player* pl = (Player*)ecx;

	static auto returnaddr1 = pattern::find(g_csgo.m_client_dll, XOR("8B CE F3 0F 10 00 8B 06 F3 0F 11 45 ? FF 90 ? ? ? ? F3 0F 10 55 ?"));
	static auto returnaddr2 = pattern::find(g_csgo.m_client_dll, XOR("F3 0F 10 55 ? 51 8B 8E ? ? ? ?"));
	static auto returnaddr3 = pattern::find(g_csgo.m_client_dll, XOR("8B 55 0C 8B C8 E8 ? ? ? ? 83 C4 08 5E 8B E5"));

	if (!pl || pl != g_cl.m_local)
		return original(ecx, edx);

	if ((_ReturnAddress() != returnaddr1 && _ReturnAddress() != returnaddr2 && _ReturnAddress() != returnaddr3))
		return original(ecx, edx);

	return &g_cl.m_angle;
}

void __fastcall extra_bones_processing::hook(void* ecx, void* edx, int a2, int a3, int a4, int a5, int a6, int a7) {
	return;
}

bool __fastcall setup_bones::hook(void* ecx, void* edx, BoneArray* bone_to_world_out, int max_bones, int bone_mask, float curtime) {
	Player* player = reinterpret_cast<Player*>(reinterpret_cast<uintptr_t>(ecx) - 0x4);
	if (!player || !player->IsPlayer() || !player->alive())
		return original(ecx, edx, bone_to_world_out, max_bones, bone_mask, curtime);

	if (allow) {
		player->InvalidateBoneCache();
		const bool& ret = original(ecx, edx, bone_to_world_out, max_bones, bone_mask, curtime);

		return ret;
	}

	if (bone_to_world_out && max_bones > 0 && max_bones <= 128)
		player->GetBones(bone_to_world_out);

	return true;
}

void __fastcall interpolate_server_entities::hook() {
	original();

	for (int i{ 1 }; i <= g_csgo.m_globals->m_max_clients; ++i) {
		Player* player = g_csgo.m_entlist->GetClientEntity< Player* >(i);
		if (!player || !player->alive() || player->dormant())
			continue;

		CBoneCache& cache = player->m_BoneCache();
		if (!cache.m_pCachedBones)
			continue;

		int backup_effects = player->m_fEffects();

		// apply local player animation fix.
		if (player->m_bIsLocalPlayer()) {
			g_cl.HandleRenderedAnimation(g_cl.m_state);
			g_cl.SetAnimations(&g_rendered_state);

			setup_bones::allow = true;

			if (!g_menu.main.misc.smooth_local.get())
				player->m_fEffects() |= EF_NOINTERP;

			player->SetupBones(cache.m_pCachedBones, cache.m_CachedBoneCount, BONE_USED_BY_ANYTHING, g_csgo.m_globals->m_curtime);

			setup_bones::allow = false;

			player->m_fEffects() = backup_effects;

			g_cl.SetAnimations(&g_fake_state);
			g_bones.Setup(player, BONE_USED_BY_ANYTHING, g_csgo.m_globals->m_curtime, g_cl.m_fake_bones);
			continue;
		}

		player->m_fEffects() |= EF_NOINTERP;

		setup_bones::allow = true;
		player->SetupBones(cache.m_pCachedBones, cache.m_CachedBoneCount, BONE_USED_BY_ANYTHING, g_csgo.m_globals->m_curtime);
		setup_bones::allow = false;

		player->m_fEffects() = backup_effects;
	}
}

void __fastcall modify_eye_position::hook(CCSGOPlayerAnimState* state, uintptr_t edx, vec3_t& position) {
	if (!state->m_player || state->m_player != g_cl.m_local)
		return;

	static uintptr_t calc_view_address = pattern::find(g_csgo.m_client_dll, XOR("8B 06 8B CE FF 90 ? ? ? ? 85 C0 74 4E")).as<uintptr_t>(); /* C_CSPlayer::CalcView */
	if ((uintptr_t)_ReturnAddress() == calc_view_address)
		return;

	position = g_cl.m_shoot_pos - g_cl.m_origin + state->m_player->m_vecAbsOrigin();
}

void __fastcall packet_start::hook(void* _this, int edx, int nIncomingSequence, int nOutgoingAcknowledged) {
	if (!g_csgo.m_engine->IsInGame() || !g_cl.m_local || !g_cl.m_local->alive())
		return original(_this, edx, nIncomingSequence, nOutgoingAcknowledged);

	if (sequences.empty())
		return original(_this, edx, nIncomingSequence, nOutgoingAcknowledged);

	for (auto it = sequences.begin(); it != sequences.end(); ) {
		if (*it == nOutgoingAcknowledged) {
			it = sequences.erase(it);
			return original(_this, edx, nIncomingSequence, nOutgoingAcknowledged);
		}

		++it;
	}
}

bool get_data_map::prepare_data_map(datamap2_t* map)
{
	static auto build_flattened_chains = pattern::find(g_csgo.m_client_dll, XOR("E8 ? ? ? ? 8B D7 8B 7D ? 8B CF")).as<void(__thiscall*)(datamap2_t*)>();

	if (map && !map->m_base) {
		build_flattened_chains(map);

		map = map->m_base;
		return true;
	}

	return false;
}

datamap2_t* __fastcall get_data_map::hook(Player* player, void* edx) {
	// no clue if this is needed but Local Prediction Doesn't Care About Other Players Anyway?
	if (g_cl.m_local != player)
		return original(player);

	static datamap2_t ourMap;

	if (!ourMap.m_base) {
		std::memcpy(&ourMap, original(player), sizeof datamap2_t);

		static std::unique_ptr<typedescription2_t[]> data(new typedescription2_t[ourMap.m_fields + 1]);

		std::memcpy(data.get(), ourMap.m_desc, ourMap.m_fields * sizeof typedescription2_t);

		typedescription2_t m_flVelocityModifier = {};
		m_flVelocityModifier.m_type = FIELD_FLOAT;
		m_flVelocityModifier.m_name = XOR("m_flVelocityModifier");
		m_flVelocityModifier.m_offset = g_entoffsets.m_flVelocityModifier;
		m_flVelocityModifier.m_size = 1;
		m_flVelocityModifier.m_flags = 0x100; // FTYPEDESC_INSENDTABLE
		m_flVelocityModifier.m_size_in_bytes = 4;
		m_flVelocityModifier.m_tolerance = 0.1f;

		std::memcpy(&data.get()[ourMap.m_fields], &m_flVelocityModifier, sizeof typedescription2_t);

		ourMap.m_desc = data.get();

		ourMap.m_fields += 1;

		ourMap.m_optimized_map = nullptr;

		prepare_data_map(&ourMap);
	}

	return &ourMap;
}

void __fastcall maintain_sequence_transitions::hook(void* _this, void* boneSetup, float flCycle, void* pos, void* q) {
	return;
}

bool __fastcall voice_data::hook(void* ecx, void* edx, const CSVCMsg_VoiceData& msg) {
	g_networking.RecieveData(&msg);

	return original(ecx, edx, msg);
}

void __fastcall update_clientside_animation::hook(Player* player) {
	if (!player)
		return;

	if (!player->alive())
		return original(player);

	if (!allow)
		return;

	original(player);
}

void __fastcall on_latch_interpolated_vars::hook(Entity* entity, void* edx, int flags) {
	return original(entity, flags);
}

__forceinline std::string to_lowercase(const std::string& str) {
	std::string lower = str;
	std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
	return lower;
}

void* __fastcall cmd_execute_command::hook(int target, CCommand* cmd) {
	int count = cmd->ArgC();
	const char* command = cmd->Arg(0);

	if (!strcmp(command, "whitelist")) {
		g_cl.print("Whitelist commands:\n");
		g_csgo.m_cvar->ConsoleColorPrintf(colors::white, "» wl <name>   - Add player to whitelist\n");
		g_csgo.m_cvar->ConsoleColorPrintf(colors::white, "» unwl <name> - Remove player from whitelist\n");
		g_csgo.m_cvar->ConsoleColorPrintf(colors::white, "» wl_list     - Show all players\n");
		g_csgo.m_cvar->ConsoleColorPrintf(colors::white, "» wl_clear    - Clear whitelist\n\n");
		g_csgo.m_cvar->ConsoleColorPrintf(colors::white, "Note: Commands work with partial names and are not case sensitive\n");
		return 0;
	}

	if (!strcmp(command, "wl")) {
		if (count < 2) {
			g_cl.print("Usage: wl <name>\n");
			return 0;
		}

		std::string target_name;
		for (int i = 1; i < count; i++) {
			target_name += cmd->Arg(i);
			if (i < count - 1) target_name += " ";
		}

		bool found = false;
		for (int i = 1; i <= g_csgo.m_globals->m_max_clients; i++) {
			Player* player = g_csgo.m_entlist->GetClientEntity<Player*>(i);
			if (!player || !player->IsPlayer())
				continue;

			player_info_t info;
			if (!g_csgo.m_engine->GetPlayerInfo(i, &info))
				continue;

			if (strstr(to_lowercase(info.m_name).c_str(), to_lowercase(target_name).c_str())) {
				AimPlayer* data = &g_aimbot.m_players[i - 1];
				if (!data)
					continue;

				data->m_white_listed = true;
				found = true;

				g_cl.print("Whitelisted \"[%s]\"\n", info.m_name);
				break;
			}
		}

		if (!found)
			g_cl.print("Player not found\n");

		return 0;
	}

	if (!strcmp(command, "unwl")) {
		if (count < 2) {
			g_cl.print("Usage: unwl <name>\n");
			return 0;
		}

		std::string target_name;
		for (int i = 1; i < count; i++) {
			target_name += cmd->Arg(i);
			if (i < count - 1) target_name += " ";
		}

		bool found = false;
		for (int i = 1; i <= g_csgo.m_globals->m_max_clients; i++) {
			Player* player = g_csgo.m_entlist->GetClientEntity<Player*>(i);
			if (!player || !player->IsPlayer())
				continue;

			player_info_t info;
			if (!g_csgo.m_engine->GetPlayerInfo(i, &info))
				continue;

			if (strstr(to_lowercase(info.m_name).c_str(), to_lowercase(target_name).c_str())) {
				AimPlayer* data = &g_aimbot.m_players[i - 1];
				if (!data)
					continue;

				data->m_white_listed = false;
				found = true;

				g_cl.print("Un-whitelisted \"[%s]\"\n", info.m_name);
				break;
			}
		}

		if (!found)
			g_cl.print("Player not found\n");

		return 0;
	}

	if (!strcmp(command, "wl_list")) {
		g_cl.print("Player List:\n");

		bool found = false;
		for (int i = 1; i <= g_csgo.m_globals->m_max_clients; i++) {
			Player* player = g_csgo.m_entlist->GetClientEntity<Player*>(i);
			if (!player || !player->IsPlayer())
				continue;

			player_info_t info;
			if (!g_csgo.m_engine->GetPlayerInfo(i, &info))
				continue;

			AimPlayer* data = &g_aimbot.m_players[i - 1];
			if (!data)
				continue;

			found = true;
			g_csgo.m_cvar->ConsoleColorPrintf(colors::white, "» %s - %s\n", info.m_name, data->m_white_listed ? XOR("Whitelisted") : XOR("Not Whitelisted"));
		}

		if (!found)
			g_csgo.m_cvar->ConsoleColorPrintf(colors::white, "No players found");
		return 0;
	}

	if (!strcmp(command, "wl_clear")) {
		for (int i = 0; i < 64; i++) {
			AimPlayer* data = &g_aimbot.m_players[i];
			if (data) {
				data->m_white_listed = false;
			}
		}

		g_cl.print("Whitelist was cleared\n");
		return 0;
	}

	return original(target, cmd);
}

void Hooks::init() {
	if (g_csgo.valid_hwid() != g_csgo.m_users[0].m_hwid)
		return;

	// hook wndproc.
	m_old_wndproc = (WNDPROC)g_winapi.SetWindowLongA(g_csgo.m_game->m_hWindow, GWL_WNDPROC, util::force_cast<LONG>(Hooks::WndProc));

	// setup normal VMT hooks.
	m_panel.init(g_csgo.m_panel);
	m_panel.add(IPanel::PAINTTRAVERSE, util::force_cast(&Hooks::PaintTraverse));

	m_client.init(g_csgo.m_client);
	m_client.add(CHLClient::LEVELINITPREENTITY, util::force_cast(&Hooks::LevelInitPreEntity));
	m_client.add(CHLClient::LEVELINITPOSTENTITY, util::force_cast(&Hooks::LevelInitPostEntity));
	m_client.add(CHLClient::LEVELSHUTDOWN, util::force_cast(&Hooks::LevelShutdown));
	//m_client.add( CHLClient::INKEYEVENT, util::force_cast( &Hooks::IN_KeyEvent ) );
	m_client.add(CHLClient::FRAMESTAGENOTIFY, util::force_cast(&Hooks::FrameStageNotify));

	m_engine.init(g_csgo.m_engine);
	m_engine.add(IVEngineClient::ISCONNECTED, util::force_cast(&Hooks::IsConnected));
	m_engine.add(IVEngineClient::ISHLTV, util::force_cast(&Hooks::IsHLTV));

	m_client_mode.init(g_csgo.m_client_mode);
	m_client_mode.add(IClientMode::SHOULDDRAWPARTICLES, util::force_cast(&Hooks::ShouldDrawParticles));
	m_client_mode.add(IClientMode::SHOULDDRAWFOG, util::force_cast(&Hooks::ShouldDrawFog));
	m_client_mode.add(IClientMode::OVERRIDEVIEW, util::force_cast(&Hooks::OverrideView));
	m_client_mode.add(IClientMode::DOPOSTSPACESCREENEFFECTS, util::force_cast(&Hooks::DoPostScreenSpaceEffects));

	m_surface.init(g_csgo.m_surface);
	//m_surface.add( ISurface::GETSCREENSIZE, util::force_cast( &Hooks::GetScreenSize ) );
	m_surface.add(ISurface::LOCKCURSOR, util::force_cast(&Hooks::LockCursor));
	m_surface.add(ISurface::ONSCREENSIZECHANGED, util::force_cast(&Hooks::OnScreenSizeChanged));

	m_model_render.init(g_csgo.m_model_render);
	m_model_render.add(IVModelRender::DRAWMODELEXECUTE, util::force_cast(&Hooks::DrawModelExecute));

	m_render_view.init(g_csgo.m_render_view);
	m_render_view.add(IVRenderView::SCENEEND, util::force_cast(&Hooks::SceneEnd));

	m_shadow_mgr.init(g_csgo.m_shadow_mgr);
	m_shadow_mgr.add(IClientShadowMgr::COMPUTESHADOWDEPTHTEXTURES, util::force_cast(&Hooks::ComputeShadowDepthTextures));

	m_view_render.init(g_csgo.m_view_render);
	m_view_render.add(CViewRender::ONRENDERSTART, util::force_cast(&Hooks::OnRenderStart));
	m_view_render.add(CViewRender::RENDERVIEW, util::force_cast(&Hooks::RenderView));
	m_view_render.add(CViewRender::RENDER2DEFFECTSPOSTHUD, util::force_cast(&Hooks::Render2DEffectsPostHUD));
	m_view_render.add(CViewRender::RENDERSMOKEOVERLAY, util::force_cast(&Hooks::RenderSmokeOverlay));

	m_match_framework.init(g_csgo.m_match_framework);
	m_match_framework.add(CMatchFramework::GETMATCHSESSION, util::force_cast(&Hooks::GetMatchSession));

	m_material_system.init(g_csgo.m_material_system);
	m_material_system.add(IMaterialSystem::OVERRIDECONFIG, util::force_cast(&Hooks::OverrideConfig));


	static void* hookable_cl = (void*)((uintptr_t)g_csgo.m_cl + 0x8);

	//static auto update_animstate_target = pattern::find(g_csgo.m_client_dll, XOR("55 8B EC 83 E4 F8 83 EC 18 56 57 8B F9 F3")).as<void*>();
	static auto call_post_data_updates_target = pattern::find(g_csgo.m_engine_dll, XOR("55 8B EC 83 EC ? 53 56 8B 35 ? ? ? ? 8B D9 8B CE")).as<void*>();
	static auto create_move_target = (void*)util::get_method(g_csgo.m_client, CHLClient::CREATEMOVE);
	static auto get_eye_ang_target = pattern::find(g_csgo.m_client_dll, XOR("56 8B F1 85 F6 74 32")).as<void*>();
	static auto do_extra_bones_processing_target = pattern::find(g_csgo.m_client_dll, XOR("55 8B EC 83 E4 F8 81 EC FC 00 00 00 53 56 8B F1 57")).as<void*>();
	static auto setup_bones_target = pattern::find(g_csgo.m_client_dll, XOR("55 8B EC 83 E4 F0 B8 ? ? ? ? E8 ? ? ? ? 56 57 8B F9")).as<void*>();
	static auto interpolate_server_entities_target = pattern::find(g_csgo.m_client_dll, XOR("55 8B EC 83 EC 1C 8B 0D ? ? ? ? 53")).as<void*>();
	static auto modify_eye_position_target = pattern::find(g_csgo.m_client_dll, XOR("55 8B EC 83 E4 F8 83 EC 58 56 57 8B F9 83 7F 60")).as<void*>();
	static auto send_data_gram_target = pattern::find(g_csgo.m_engine_dll, XOR("55 8B EC 83 E4 F0 B8 ? ? ? ? E8 ? ? ? ? 56 57 8B F9 89 7C 24 18")).as<void*>();
	static auto process_packet_target = pattern::find(g_csgo.m_engine_dll, XOR("55 8B EC 83 E4 C0 81 EC B4 00 00 00 53 56 57 8B 7D 08 8B D9")).as<void*>();
	static auto packet_start_target = (void*)util::get_method(hookable_cl, 5);
	static auto get_data_map_target = pattern::find(g_csgo.m_client_dll, XOR("B8 ? ? ? ? C3 CC CC CC CC CC CC CC CC CC CC B8 ? ? ? ? C3 CC CC CC CC CC CC CC CC CC CC 55 8B EC A1 ? ? ? ? 56 68 ? ? ? ? 8B 08 8B 01 FF 50 ? 85 C0 75 ? 33 F6 EB ? 8D 70 ? 83 E6 ? 89 46 ? 68 ? ? ? ? 6A ? 56 E8 ? ? ? ? 83 C4 ? 85 F6 74 ? 8B CE E8 ? ? ? ? 8B F0 85 F6 74 ? FF 75 ? 8B 16 8B CE FF 75 ? FF 92 ? ? ? ? 8D 46 ? 5E 5D C3 33 C0 5E 5D C3 CC CC CC CC CC CC CC CC CC CC CC CC CC CC 56 8B F1 E8 ? ? ? ? C7 06 ? ? ? ? C7 46")).as<void*>();
	static auto maintain_sequence_transitions_target = pattern::find(g_csgo.m_client_dll, XOR("53 8B DC 83 EC 08 83 E4 F8 83 C4 04 55 8B 6B 04 89 6C 24 04 8B EC 83 EC 18 56 57 8B F9 F3 0F 11 55 FC 80 BF F0 09 00 00 00")).as<void*>();
	static auto run_command_target = (void*)util::get_method(g_csgo.m_prediction, CPrediction::RUNCOMMAND);
	static auto in_prediction_target = (void*)util::get_method(g_csgo.m_prediction, CPrediction::INPREDICTION);
	static auto voice_data_target = util::get_method(hookable_cl, 24);
	static auto update_clientside_animation_target = pattern::find(g_csgo.m_client_dll, XOR("55 8B EC 51 56 8B F1 80 BE E1")).as<void*>();
	static auto on_latch_interpolated_vars_target = pattern::find(g_csgo.m_client_dll, XOR("55 8B EC 83 EC ? 53 56 8B F1 57 80 BE EA 02 00 00 00")).as<void*>();
	static auto cmd_execute_command = pattern::find(g_csgo.m_engine_dll, XOR("55 8B EC 83 EC 08 53 56 57 8B FA 89 4D FC"));;

	MH_Initialize();
	MH_CreateHook(cmd_execute_command, cmd_execute_command::hook, (void**)&cmd_execute_command::original);
	MH_CreateHook(voice_data_target, voice_data::hook, (void**)&voice_data::original);
	//MH_CreateHook( update_animstate_target, update_animstate::hook, ( void** )&update_animstate::original );
	MH_CreateHook(call_post_data_updates_target, call_post_data_updates::hook, (void**)&call_post_data_updates::original);
	MH_CreateHook(create_move_target, create_move::create_move_proxy, (void**)&create_move::original);
	MH_CreateHook(get_eye_ang_target, get_eye_angles::hook, (void**)&get_eye_angles::original);
	MH_CreateHook(do_extra_bones_processing_target, extra_bones_processing::hook, (void**)&extra_bones_processing::original);
	MH_CreateHook(setup_bones_target, setup_bones::hook, (void**)&setup_bones::original);
	MH_CreateHook(interpolate_server_entities_target, interpolate_server_entities::hook, (void**)&interpolate_server_entities::original);
	MH_CreateHook(modify_eye_position_target, modify_eye_position::hook, (void**)&modify_eye_position::original);
	MH_CreateHook(send_data_gram_target, send_data_gram::hook, (void**)&send_data_gram::original);
	MH_CreateHook(process_packet_target, process_packet::hook, (void**)&process_packet::original);
	MH_CreateHook(packet_start_target, packet_start::hook, (void**)&packet_start::original);
	MH_CreateHook(get_data_map_target, get_data_map::hook, (void**)&get_data_map::original);
	MH_CreateHook(maintain_sequence_transitions_target, maintain_sequence_transitions::hook, (void**)&maintain_sequence_transitions::original);
	MH_CreateHook(run_command_target, run_command::hook, (void**)&run_command::original);
	MH_CreateHook(in_prediction_target, in_prediction::hook, (void**)&in_prediction::original);
	MH_CreateHook(update_clientside_animation_target, update_clientside_animation::hook, (void**)&update_clientside_animation::original);
	MH_CreateHook(on_latch_interpolated_vars_target, on_latch_interpolated_vars::hook, (void**)&on_latch_interpolated_vars::original);
	MH_EnableHook(nullptr);

	g_netvars.SetProxy(HASH("DT_CSRagdoll"), HASH("m_vecForce"), Force_proxy, m_Force_original);
}