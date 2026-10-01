#include "includes.h"

Chams g_chams{ };;

Chams::model_type_t Chams::GetModelType(const ModelRenderInfo_t& info) {
	// model name.
	//const char* mdl = info.m_model->m_name;

	std::string mdl{ info.m_model->m_name };

	//static auto int_from_chars = [ mdl ]( size_t index ) {
	//	return *( int* )( mdl + index );
	//};

	// little endian.
	//if( int_from_chars( 7 ) == 'paew' ) { // weap
	//	if( int_from_chars( 15 ) == 'om_v' && int_from_chars( 19 ) == 'sled' )
	//		return model_type_t::arms;
	//
	//	if( mdl[ 15 ] == 'v' )
	//		return model_type_t::view_weapon;
	//}

	//else if( int_from_chars( 7 ) == 'yalp' ) // play
	//	return model_type_t::player;

	if (mdl.find(XOR("player")) != std::string::npos && info.m_index >= 1 && info.m_index <= 64)
		return model_type_t::player;

	return model_type_t::invalid;
}

bool Chams::IsInViewPlane(const vec3_t& world) {
	float w;

	const VMatrix& matrix = g_csgo.m_engine->WorldToScreenMatrix();

	w = matrix[3][0] * world.x + matrix[3][1] * world.y + matrix[3][2] * world.z + matrix[3][3];

	return w > 0.001f;
}

void Chams::SetColor(Color col, IMaterial* mat) {
	if (mat)
		mat->ColorModulate(col);

	else
		g_csgo.m_render_view->SetColorModulation(col);
}

void Chams::SetAlpha(float alpha, IMaterial* mat) {
	if (mat)
		mat->AlphaModulate(alpha);

	else
		g_csgo.m_render_view->SetBlend(alpha);
}

void Chams::SetupMaterial(IMaterial* mat, Color col, bool z_flag) {
	SetColor(col);

	// mat->SetFlag( MATERIAL_VAR_HALFLAMBERT, flags );
	mat->SetFlag(MATERIAL_VAR_ZNEARER, z_flag);
	mat->SetFlag(MATERIAL_VAR_NOFOG, z_flag);
	mat->SetFlag(MATERIAL_VAR_IGNOREZ, z_flag);

	g_csgo.m_studio_render->ForcedMaterialOverride(mat);
}

IMaterial* Chams::GetMaterial(int entity) {
	switch (entity) {
	case 0: {
		if (g_menu.main.players.chams_local.get(2))
			return debugdrawflat;

		if (g_menu.main.players.chams_local.get(3))
			return metallic;
	} break;
	case 1: {
		if (g_menu.main.players.chams_enemy.get(3))
			return debugdrawflat;

		if (g_menu.main.players.chams_enemy.get(4))
			return metallic;
	} break;
	case 2: {
		if (g_menu.main.players.chams_friendly.get(3))
			return debugdrawflat;

		if (g_menu.main.players.chams_friendly.get(4))
			return metallic;
	} break;
	}

	return debugambientcube;
}

void Chams::init() {
	std::ofstream("csgo\\materials\\rax_shine.vmt") << R"#("VertexLitGeneric" 
	{
					"$basetexture"				"vgui/white_additive"
					"$ignorez"					"0"
					"$phong"					"1"
					"$BasemapAlphaPhongMask"    "1"
					"$phongexponent"			"15"
					"$normalmapalphaenvmask"	"1"
					"$envmap"					"env_cubemap"
					"$envmaptint"				"[0.0 0.0 0.0]"
					"$phongboost"				"[0.6 0.6 0.6]"
					"phongfresnelranges"		"[0.5 0.5 1.0]"
					"$nofog"					"1"
					"$model"					"1"
					"$nocull"					"0"
					"$selfillum"				"1"
					"$halflambert"				"1"
					"$znearer"					"0"
					"$flat"						"0"	
					"$rimlight"					"1"
					"$rimlightexponent"			"2"
					"$rimlightboost"			"0"
		})#";

	metallic = g_csgo.m_material_system->FindMaterial(XOR("rax_shine"), XOR("Model textures"));
	metallic->IncrementReferenceCount();

	// find stupid materials.
	debugambientcube = g_csgo.m_material_system->FindMaterial(XOR("debug/debugambientcube"), XOR("Model textures"));
	debugambientcube->IncrementReferenceCount();

	debugdrawflat = g_csgo.m_material_system->FindMaterial(XOR("debug/debugdrawflat"), XOR("Model textures"));
	debugdrawflat->IncrementReferenceCount();
}

bool Chams::OverridePlayer(int index) {
	Player* player = g_csgo.m_entlist->GetClientEntity< Player* >(index);
	if (!player)
		return false;

	// always skip the local player in DrawModelExecute.
	// this is because if we want to make the local player have less alpha
	// the static props are drawn after the players and it looks like aids.
	// therefore always process the local player in scene end.
	if (player->m_bIsLocalPlayer())
		return true;

	// see if this player is an enemy to us.
	bool enemy = g_cl.m_local && player->enemy(g_cl.m_local);

	// we have chams on enemies.
	if (enemy && g_menu.main.players.chams_enemy.get(0))
		return true;

	// we have chams on friendly.
	else if (!enemy && g_menu.main.players.chams_friendly.get(0))
		return true;

	return false;
}

bool Chams::DrawModel(uintptr_t ctx, const DrawModelState_t& state, const ModelRenderInfo_t& info, matrix3x4_t* bone) {
	// store and validate model type.
	model_type_t type = GetModelType(info);
	if (type == model_type_t::invalid)
		return true;

	// is a valid player.
	if (type == model_type_t::player) {
		// do not cancel out our own calls from SceneEnd
		// also do not cancel out calls from the glow.
		if (!m_running && !g_csgo.m_studio_render->m_pForcedMaterial && OverridePlayer(info.m_index))
			return false;
	}

	return true;
}

void Chams::SceneEnd() {
	// store and sort ents by distance.
	if (SortPlayers()) {
		// iterate each player and render them.
		for (const auto& p : m_players)
			RenderPlayer(p);
	}

	// restore.
	g_csgo.m_studio_render->ForcedMaterialOverride(nullptr);
	g_csgo.m_render_view->SetColorModulation(colors::white);
	g_csgo.m_render_view->SetBlend(1.f);
}

void Chams::RenderBacktrack(Player* player) {
	if (!g_menu.main.players.chams_enemy.get(2))
		return;

	AimPlayer* data = &g_aimbot.m_players[player->index() - 1];
	if (!data || data->m_records.empty())
		return;

	LagRecord* last = nullptr;
	for (LagRecord& record : data->m_records) {
		if (record.m_lagcomp[LC_LOST_TRACK])
			break;

		if (record.m_lagcomp[LC_TIME_DECREMENT])
			continue;

		if (!record.ValidTime())
			continue;

		last = &record;
	}

	if (!last || last->m_origin.dist_to(player->m_vecAbsOrigin()) < 15.f)
		return;

	BoneArray backup_bones[128];
	player->GetBones(backup_bones);

	SetAlpha(g_menu.main.players.chams_enemy_history.get().a() / 255.f);
	SetupMaterial(debugdrawflat, g_menu.main.players.chams_enemy_history.get(), true);

	player->SetBones(last->m_bones);

	SetAlpha(g_menu.main.players.chams_enemy_history.get().a() / 255.f);
	SetupMaterial(debugdrawflat, g_menu.main.players.chams_enemy_history.get(), true);

	player->DrawModel();

	player->SetBones(backup_bones);
}

void Chams::RenderPlayer(Player* player) {
	// prevent recruisive model cancelation.
	m_running = true;

	// restore.
	g_csgo.m_studio_render->ForcedMaterialOverride(nullptr);
	g_csgo.m_render_view->SetColorModulation(colors::white);
	g_csgo.m_render_view->SetBlend(1.f);

	// this is the local player.
	// we always draw the local player manually in drawmodel.
	if (player->m_bIsLocalPlayer()) {
		float alpha_mp = 1.f;
		if (g_menu.main.players.chams_local_scope.get() && player->m_bIsScoped())
			alpha_mp *= 0.5f;

		IMaterial* material = GetMaterial(0);

		if (g_menu.main.players.chams_local.get(0)) {
			SetAlpha(g_menu.main.players.chams_local_col.get().a() * alpha_mp / 255.f);
			SetupMaterial(material, g_menu.main.players.chams_local_col.get(), false);

			player->DrawModel();
		}
		else {
			SetAlpha(alpha_mp);

			player->DrawModel();
		}

		if (g_menu.main.players.chams_local.get(1)/* && g_fake_state.m_ground && g_fake_state.m_vel_xy <= 0.1f*/) {
			const float update_fade = std::max(std::min((g_cl.m_state->m_body_update - g_cl.m_curtime) / 1.1f, 1.f), 0.f);

			BoneArray backup_bones[128];
			player->GetBones(backup_bones);

			player->SetBones(g_cl.m_fake_bones);

			SetAlpha(g_menu.main.players.chams_fake_col.get().a() * alpha_mp * update_fade / 255.f);
			SetupMaterial(material, g_menu.main.players.chams_fake_col.get(), false);

			player->DrawModel();

			player->SetBones(backup_bones);
		}
	}

	// check if is an enemy.
	bool enemy = g_cl.m_local && player->enemy(g_cl.m_local);

	IMaterial* material = GetMaterial(enemy ? 1 : 2);

	if (enemy && g_menu.main.players.chams_enemy.get(0)) {
		RenderBacktrack(player);

		if (g_menu.main.players.chams_enemy.get(1)) {
			SetAlpha(g_menu.main.players.chams_enemy_invis.get().a() / 255.f);
			SetupMaterial(material, g_menu.main.players.chams_enemy_invis.get(), true);

			player->DrawModel();
		}

		SetAlpha(g_menu.main.players.chams_enemy_vis.get().a() / 255.f);
		SetupMaterial(material, g_menu.main.players.chams_enemy_vis.get(), false);

		player->DrawModel();
	}

	else if (!enemy && g_menu.main.players.chams_friendly.get(0)) {
		if (g_menu.main.players.chams_friendly.get(1)) {

			SetAlpha(g_menu.main.players.chams_friendly_invis.get().a() / 255.f);
			SetupMaterial(material, g_menu.main.players.chams_friendly_invis.get(), true);

			player->DrawModel();
		}

		SetAlpha(g_menu.main.players.chams_friendly_vis.get().a() / 255.f);
		SetupMaterial(material, g_menu.main.players.chams_friendly_vis.get(), false);

		player->DrawModel();
	}

	m_running = false;
}

bool Chams::SortPlayers() {
	// lambda-callback for std::sort.
	// to sort the players based on distance to the local-player.
	static auto distance_predicate = [](Entity* a, Entity* b) {
		vec3_t local = g_cl.m_local->GetAbsOrigin();

		// note - dex; using squared length to save out on sqrt calls, we don't care about it anyway.
		float len1 = (a->GetAbsOrigin() - local).length_sqr();
		float len2 = (b->GetAbsOrigin() - local).length_sqr();

		return len1 < len2;
		};

	// reset player container.
	m_players.clear();

	// find all players that should be rendered.
	for (int i{ 1 }; i <= g_csgo.m_globals->m_max_clients; ++i) {
		// get player ptr by idx.
		Player* player = g_csgo.m_entlist->GetClientEntity< Player* >(i);

		// validate.
		if (!player || !player->IsPlayer() || !player->alive() || player->dormant())
			continue;

		// do not draw players occluded by view plane.
		if (!IsInViewPlane(player->WorldSpaceCenter()))
			continue;

		// this player was not skipped to draw later.
		// so do not add it to our render list.
		if (!OverridePlayer(i))
			continue;

		m_players.push_back(player);
	}

	// any players?
	if (m_players.empty())
		return false;

	// sorting fixes the weird weapon on back flickers.
	// and all the other problems regarding Z-layering in this shit game.
	std::sort(m_players.begin(), m_players.end(), distance_predicate);

	return true;
}