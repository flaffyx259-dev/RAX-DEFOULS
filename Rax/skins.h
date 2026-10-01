#pragma once

constexpr int sequence_default_draw = 0;
constexpr int sequence_default_idle1 = 1;
constexpr int sequence_default_idle2 = 2;
constexpr int sequence_default_heavy_miss1 = 9;
constexpr int sequence_default_heavy_hit1 = 10;
constexpr int sequence_default_heavy_backstab = 11;
constexpr int sequence_default_lookat01 = 12;

constexpr int sequence_butterfly_draw = 0;
constexpr int sequence_butterfly_draw2 = 1;
constexpr int sequence_butterfly_lookat01 = 13;
constexpr int sequence_butterfly_lookat03 = 15;

constexpr int sequence_falchion_idle1 = 1;
constexpr int sequence_falchion_heavy_miss1 = 8;
constexpr int sequence_falchion_heavy_miss1_noflip = 9;
constexpr int sequence_falchion_lookat01 = 12;
constexpr int sequence_falchion_lookat02 = 13;

constexpr int sequence_push_idle1 = 1;
constexpr int sequence_push_heavy_miss2 = 11;


class CAttribute_String
{
public:
	virtual ~CAttribute_String( );

	int32_t FieldSet;
	char** Value;

private:
	int32_t pad0[ 2 ];
};

struct cmd_context_t {
	bool		m_needs_processing{ };
	CUserCmd	m_user_cmd{ };
	int			m_cmd_number{ };
};

template < typename Key, typename Value >
struct Node_t {
	int previous_id;    //0x0000
	int next_id;        //0x0004
	void* _unknown_ptr; //0x0008
	int _unknown;       //0x000C
	Key key;            //0x0010
	Value value;        //0x0014
};

template < typename Key, typename Value >
struct Head_t {
	Node_t< Key, Value >* memory; //0x0000
	int allocation_count;         //0x0004
	int grow_size;                //0x0008
	int start_element;            //0x000C
	int next_available;           //0x0010
	int _unknown;                 //0x0014
	int last_element;             //0x0018
};

union attribute_data_union_t
{
	bool bValue;
	int iValue;
	float fValue;
	vec3_t* vValue;
	CAttribute_String* szValue;
};

class CEconItemAttributeDefinition;
class ISchemaAttributeType
{
public:
	virtual ~ISchemaAttributeType( );
	virtual int GetTypeUniqueIdentifier( );
	virtual void LoadEconAttributeValue( );
	virtual void ConvertEconAttributeValueToByteStream( );
	virtual bool BConvertStringToEconAttributeValue( const CEconItemAttributeDefinition*, const char*, attribute_data_union_t* );
};

struct attr_type_t
{
	char* name;
	ISchemaAttributeType* type;
};

struct AlternateIconData_t
{
private:
	int32_t pad0;

public:
	CUtlString icon_path;
	CUtlString icon_path_large;

private:
	CUtlString pad1[ 2 ];
};

struct kill_eater_score_type_t
{
	int id;
	char* type_name;
	char* model_attribute;
	char* level_data;
	bool use_level_data;
};

class CEconItemRarityDefinition
{
public:
	int value;
	int color;
	CUtlString name;
	CUtlString loc_key;
	CUtlString loc_key_weapon;
	CUtlString loot_list;
	CUtlString recycle_list;
	CUtlString drop_sound;
	CUtlString next_rarity;

private:
	int32_t pad0[ 2 ];

public:
	float weight;
};

class CEconItemQualityDefinition
{
public:
	int value;
	char* name;
	int weight;
	bool canSupportSet;
	char* hexColor;
};

class CEconItemAttributeDefinition
{
public:
	virtual ~CEconItemAttributeDefinition( );

	KeyValues* kv;
	uint16_t id;
	ISchemaAttributeType* attribute_type;
	bool hidden;
	bool force_output_description;
	bool stored_as_integer;
	bool instance_data;
	int asset_class_export;
	int asset_class_bucket;
	int effect_type;
	int description_format;
	char* description_string;
	char* description_tag;
	char* armory_desc;
	int score;
	char* name;
	char* attribute_class;

private:
	int32_t pad0;
};

class CEconCraftingRecipeDefinition
{
public:
	virtual ~CEconCraftingRecipeDefinition( );

	int definitionindex; // Don't ask me why Valve sometimes uses dword and sometimes word.
	CUtlString name;
	CUtlString n_A;
	CUtlString desc_inputs;
	CUtlString desc_outputs;
	CUtlString di_A;
	CUtlString di_B;
	CUtlString di_C;
	CUtlString do_A;
	CUtlString do_B;
	CUtlString do_C;
	wchar_t name_localized[ 64 ];
	wchar_t requires_produces_localized[ 512 ];
	bool all_same_class;
	bool all_same_slot;
	int add_class_usage_to_output;
	int add_slot_usage_to_output;
	int add_set_to_output;
	bool always_known;
	bool premium_only;
	int category;
	int filter;

private: // Some vectors and other things
	int32_t pad0[ 47 ];
};

struct item_list_entry_t
{
	int item;
	int paintkit;

private:
	int32_t pad0[ 5 ];
};

struct itemset_attrib_t;

class CEconItemSetDefinition
{
public:
	virtual ~CEconItemSetDefinition( );

	char* key_name;
	char* name;
	char* unlocalized_name;
	char* set_description;
	CUtlVector<item_list_entry_t> items;
	int store_bundle;
	bool is_collection;
	bool is_hidden_set;
	CUtlVector<itemset_attrib_t> attributes;
};

class CEconLootListDefinition
{
public:
	virtual ~CEconLootListDefinition( );

private:
	int32_t pad0[ 5 ]; // a vector

public:
	char* name;
	CUtlVector<item_list_entry_t> items;
	int hero;
	bool public_list_contents;
	bool contains_stickers_autographed_by_proplayers;
	bool will_produce_stattrak;
	float totalweights;
	CUtlVector<float> weights;

private:
	int32_t pad1[ 6 ]; // a vector + something else
};

class CPaintKit
{
public:
	int id;

	CUtlString name;
	CUtlString description_string;
	CUtlString description_tag;
	CUtlString pattern;
	CUtlString normal;
	CUtlString logo_material;

	bool pad0;

	int rarity;
	int style;

	Color color0;
	Color color1;
	Color color2;
	Color color3;

	Color logocolor0;
	Color logocolor1;
	Color logocolor2;
	Color logocolor3;

	float wear_default;
	float wear_remap_min;
	float wear_remap_max;

	uint8_t seed;

	uint8_t phongexponent;
	uint8_t phongalbedoboost;
	uint8_t phongintensity;

	float pattern_scale;
	float pattern_offset_y_start;
	float pattern_offset_y_end;
	float pattern_offset_x_start;
	float pattern_offset_x_end;
	float pattern_rotate_start;
	float pattern_rotate_end;

	float logo_scale;
	float logo_offset_x;
	float logo_offset_y;
	float logo_rotation;

	bool ignore_weapon_size_scale;
	int view_model_exponent_override_size;
	bool only_first_material;
	bool use_normal;

	CUtlString vmt_path;
	KeyValues* vmt_overrides;
};

class CStickerKit
{
public:
	int id;

	int item_rarity;

	CUtlString name;
	CUtlString description_string;
	CUtlString item_name;
	CUtlString sticker_material;
	CUtlString sticker_material_nodrips;
	CUtlString image_inventory;

	int tournament_event_id;
	int tournament_team_id;
	int tournament_player_id;
	bool cannot_trade;

	float rotate_start;
	float rotate_end;

	float scale_min;
	float scale_max;

	float wear_min;
	float wear_max;

	CUtlString image_inventory2;
	CUtlString image_inventory_large;

	KeyValues* kv;
};

class CEconColorDefinition
{
public:
	char* name;
	char* color_name;
	char* hex_color;
};

class CEconGraffitiTintDefinition
{
public:
	int id;
	char* name;
	char* hex_color;

	uint8_t b;
	uint8_t g;
	uint8_t r;
};

class CEconMusicDefinition
{
public:
	int id;
	char* name;
	char* loc_name;
	char* loc_description;
	char* pedestal_display_model;
	char* image_inventory;

private:
	int32_t pad0[ 2 ];
};

class CEconQuestDefinition
{
public:
	int id;
	char* name;
	char* mapgroup;
	char* map;
	char* gamemode;
	char* quest_reward;
	char* expression;
	char* expr_bonus;
	CCopyableUtlVector<unsigned int> points;

	enum
	{
		is_an_event = 1,
		event_allow_individual_maps
	} flags;

	int difficulty;
	int operational_points;
	int xp_reward;
	int xp_bonus_percent;
	int target_team;
	int required_event;

private:
	int32_t pad0;

public:
	int gametype; // gamemode & 15 | ( mapgroup << 8 )
	char* loc_name;
	char* loc_shortname;
	char* loc_description;
	char* loc_huddescription;
	KeyValues* string_tokens;
	char* loc_bonus;
	char* quest_icon;
};

class CEconCampaignDefinition
{
public:
	int id;

private:
	int32_t pad0;

public:
	char* loc_name;
	char* loc_description;

	class CEconCampaignNodeDefinition
	{
	public:
		class CEconCampaignNodeStoryBlockDefinition
		{
		public:
			char* content_file;
			char* character_name;
			char* expression;
			char* description;
		};
		CUtlVector<CEconCampaignNodeStoryBlockDefinition*> story_blocks;

		int id;
		int quest_index;
		int parentid;
		CUtlVector<unsigned int> questunlocks;
	};
	CUtlHashMapLarge<int, CEconCampaignNodeDefinition> quests;
	CUtlHashMapLarge<int, CEconCampaignNodeDefinition> quests2; // ???

	int season_number;
};

class CSkirmishModeDefinition
{
public:
	int id;
	char* name;
	char* gamemode;
	int igamemode;
	char* server_exec;
	char* loc_name;
	char* loc_rules;
	char* loc_description;
	char* loc_details;
	char* icon;
};

class CProPlayerData
{
public:
	KeyValues* kv;
	int id;
	CUtlString name;
	CUtlString code;
	int dob;
	CUtlString geo;
};

class CItemLevelingDefinition
{
public:
	int id;
	int score;
	char* rank_name;

private:
	int32_t pad0[ 2 ];
};

class CPipRankData
{
public:
	class PipRankInfo
	{
	private:
		int32_t pad0;

	public:
		int8_t id;
		int8_t pips;
		int8_t winstreak;
		int8_t loss;
	};

	CUtlVector<PipRankInfo> rank_info;
};

struct static_attrib_t // Get a CEconItemAttributeDefinition* with m_Attributes[static_attrib_t.id]
{
	uint16_t id;
	attribute_data_union_t value;
	bool force_gc_to_generate;
};

struct StickerData_t
{
	char viewmodel_geometry[ 128 ];
	char viewmodel_material[ 128 ];
	vec3_t worldmodel_decal_pos;
	vec3_t worldmodel_decal_end;
	char worldmodel_decal_bone[ 32 ];
};

struct codecontrolledbodygroupdata_t;
struct attachedmodel_t;
struct attachedparticle_t;
class CEconStyleInfo;
class AssetInfo
{
private:
	int32_t pad0;

public:
	CUtlMap<const char*, int, unsigned short> player_bodygroups;
	CUtlMap<const char*, codecontrolledbodygroupdata_t, unsigned short> code_controlled_bodygroup;
	int skin;
	bool use_per_class_bodygroups;
	CUtlVector<attachedmodel_t> attached_models;
	CUtlVector<attachedparticle_t> attached_particles;
	char* custom_sounds[ 10 ];
	char* material_override;
	int muzzle_flash;

private:
	int32_t pad1;

public:
	char* particle_effect;
	char* particle_snapshot;
	char* sounds[ 18 ];
	char* primary_ammo;
	char* secondary_ammo;
	char* weapon_type;
	char* addon_location;
	char* eject_brass_effect;
	char* muzzle_flash_effect_1st_person;
	char* muzzle_flash_effect_1st_person_alt;
	char* muzzle_flash_effect_3rd_person;
	char* muzzle_flash_effect_3rd_person_alt;
	char* heat_effect;
	char* player_animation_extension;
	vec3_t grenade_smoke_color;
	int vm_bodygroup_override;
	int vm_bodygroup_state_override;
	int wm_bodygroup_override;
	int wm_bodygroup_state_override;
	bool skip_model_combine;
	CUtlVector<const char*> animation_modifiers;
	CUtlVector<CEconStyleInfo*> styles;
};

struct WeaponPaintableMaterial_t
{
	char Name[ 128 ];
	char OrigMat[ 128 ];
	char FolderName[ 128 ];
	int ViewmodelDim;
	int WorldDim;
	float WeaponLength;
	float UVScale;
	bool BaseTextureOverride;
	bool MirrorPattern;
};

class IEconTool;
class CCStrike15ItemSystem;
class CCStrike15ItemDefinition
{
public:
	virtual uint16_t GetDefinitionIndex( ) const = 0;
	virtual const char* GetPrefabName( ) const = 0;
	virtual const char* GetItemBaseName( ) const = 0;
	virtual const char* GetItemTypeName( ) const = 0;
	virtual const char* GetItemDesc( ) const = 0;
	virtual const char* GetInventoryImage( ) const = 0;
	virtual const char* GetBasePlayerDisplayModel( ) const = 0;
	virtual const char* GetWorldDisplayModel( ) const = 0;
	virtual const char* GetExtraWearableModel( ) const = 0;
	virtual int GetLoadoutSlot( ) const = 0;
	virtual KeyValues* GetRawDefinition( ) const = 0;
	virtual int GetHeroID( ) const = 0;
	virtual int GetRarity( ) const = 0;
	virtual CUtlVector<int>* GetItemSets( ) const = 0;
	virtual int GetBundleItemCount( ) const = 0;
	virtual void* GetBundleItem( int ) const = 0;
	virtual bool IsBaseItem( ) const = 0;
	virtual bool IsPublicItem( ) const = 0;
	virtual bool IsBundle( ) const = 0;
	virtual bool IsPackBundle( ) const = 0;
	virtual bool IsPackItem( ) const = 0;
	virtual void* BInitVisualBlockFromKV( KeyValues*, void*, void* ) = 0;
	virtual void* BInitFromKV( KeyValues*, void*, void* ) = 0;
	virtual void* BInitFromTestItemKVs( int, KeyValues*, void* ) = 0;
	virtual void* GeneratePrecacheModelStrings( bool, void* ) const = 0;
	virtual void* GeneratePrecacheSoundStrings( void* ) const = 0;
	virtual void* GeneratePrecacheEffectStrings( void* ) const = 0;
	virtual void* CopyPolymorphic( const CCStrike15ItemDefinition* ) = 0;
	virtual int GetItemTypeID( ) const = 0;
	virtual bool IsDefaultSlotItem( ) const = 0;
	virtual bool IsPreviewableInStore( ) const = 0;
	virtual int GetBundleItemPaintKitID( int ) const = 0;
	virtual const char* GetWorldDroppedModel( ) const = 0;
	virtual const char* GetHolsteredModel( ) const = 0;
	virtual const char* GetZoomInSound( ) const = 0;
	virtual const char* GetZoomOutSound( ) const = 0;
	virtual const char* GetIconDisplayModel( ) const = 0;
	virtual const char* GetBuyMenuDisplayModel( ) const = 0;
	virtual const char* GetPedestalDisplayModel( ) const = 0;
	virtual const char* GetMagazineModel( ) const = 0;
	virtual const char* GetScopeLensMaskModel( ) const = 0;
	virtual const char* GetUidModel( ) const = 0;
	virtual const char* GetStatTrakModelByType( unsigned int ) const = 0;
	virtual int GetNumSupportedStickerSlots( ) const = 0;
	virtual const char* GetStickerSlotModelBySlotIndex( unsigned int ) const = 0;
	virtual void* GetStickerSlotWorldProjectionStartBySlotIndex( unsigned int ) const = 0;
	virtual void* GetStickerSlotWorldProjectionEndBySlotIndex( unsigned int ) const = 0;
	virtual void* GetStickerWorldModelBoneParentNameBySlotIndex( unsigned int ) const = 0;
	virtual void* GetStickerSlotMaterialBySlotIndex( unsigned int ) const = 0;
	virtual const char* GetIconDefaultImage( ) const = 0;
	virtual void* GetParticleFile( ) const = 0;
	virtual void* GetParticleSnapshotFile( ) const = 0;
	virtual bool IsRecent( ) const = 0;
	virtual bool IsContentStreamable( ) const = 0;
	virtual void* IgnoreInCollectionView( ) const = 0;
	virtual void* GeneratePrecacheModelStrings( bool, void* ) = 0;

public:
	KeyValues* kv;
	uint16_t id;
	CUtlVector<uint16_t> associated_items;

private:
	int32_t pad0[ 2 ];

public:
	uint8_t min_ilevel;
	uint8_t max_ilevel;
	uint8_t item_rarity;
	uint8_t item_quality;
	uint8_t forced_item_quality;
	uint8_t default_drop_quality;
	uint8_t default_drop_quantity;
	CUtlVector<static_attrib_t> attributes;
	uint8_t popularity_seed;
	KeyValues* portraits;
	char* item_name;
	bool propername;
	char* item_type_name;
	int type_id; // crc32( item_type_name )
	char* item_description;
	int expiration_date;
	int creation_date;
	char* model_inventory;
	char* image_inventory;
	CUtlVector<const char*> image_inventory_overlay;
	int image_inventory_pos_x;
	int image_inventory_pos_y;
	int image_inventory_size_w;
	int image_inventory_size_h;
	char* model_player;

private:
	bool pad1;

public:
	bool hide_bodygroups_deployed_only;
	char* model_world;
	char* model_dropped;
	char* model_holstered;
	char* extra_wearable;

private:
	int32_t pad2[ 20 ];

public:
	CUtlVector<StickerData_t> stickers;
	char* icon_default_image;
	bool attach_to_hands;
	bool attach_to_hands_vm_only;
	bool flip_viewmodel;
	bool act_as_wearable;
	CUtlVector<int> item_sets;
	AssetInfo* visuals;

private:
	int32_t pad3;

public:
	bool allow_purchase_standalone;
	char* brass_eject_model;
	char* zoom_in_sound;
	char* zoom_out_sound;

	IEconTool* tool;

private:
	int32_t pad4[ 3 ];

public:
	int sound_material;
	bool disable_style_selector;

private:
	int32_t pad5[ 8 ];

public:
	char* particle_file;
	char* particle_snapshot;
	char* loot_list_name;

private:
	int32_t pad6[ 5 ];

public:
	CUtlVector<WeaponPaintableMaterial_t> paint_data;

	struct
	{
		ang_t* camera_angles;
		vec3_t* camera_offset;
		float camera_fov;
		LightDesc_t* lights[ 4 ];

	private:
		int32_t pad0;
	} *inventory_image_data;

	char* item_class;
	char* item_logname;
	char* item_iconname;
	char* name;
	bool hidden;
	bool show_in_armory;
	bool baseitem;
	bool default_slot_item;
	bool import;
	bool one_per_account_cdkey;

private:
	int32_t pad7;

public:
	char* armory_desc;
	CCStrike15ItemDefinition* armory_remapdef;
	CCStrike15ItemDefinition* store_remapdef;
	char* armory_remap;
	char* store_remap;
	char* class_token_id;
	char* slot_token_id;
	int drop_type;
	int holiday_restriction;
	int subtype;

private:
	int32_t pad8[ 4 ];

public:
	CUtlMap<unsigned int, const char*, unsigned short> alternate_icons;

private:
	int32_t pad9[ 9 ];

public:
	bool not_developer;
	bool ignore_in_collection_view;

	// This is where CCStrike15ItemDefinition begins
	int item_sub_position;
	int item_gear_slot;
	int item_gear_slot_position;
	int anim_slot;
	char* model_player_per_class[ 4 ];
	int class_usage[ 4 ];

private:
	int32_t pad10[ 2 ];
};

class CCStrike15ItemSchema
{
public:
	virtual ~CCStrike15ItemSchema( );

	CEconItemAttributeDefinition* GetAttributeDefinitionByName( const char* Name ) const;

private:
	int32_t pad0[ 11 ];

public:
	KeyValues* kv;

private:
	int32_t pad1;

public:
	uint16_t first_valid_class;
	uint16_t last_valid_class;
	uint16_t first_valid_item_slot;
	uint16_t last_valid_item_slot;
	int num_item_presets;
	int item_level_min;
	int item_level_max;

private:
	int32_t pad2;

public:
	CUtlVector<attr_type_t> attribute_types;
	CUtlHashMapLarge<int, CEconItemRarityDefinition> rarities;
	CUtlHashMapLarge<int, CEconItemQualityDefinition> qualities;

private:
	int32_t pad3[ 18 ];

public:
	CUtlHashMapLarge<int, CCStrike15ItemDefinition*> items;
	CCStrike15ItemDefinition* m_DefaultItem; // The first item
	CUtlVector<CEconItemAttributeDefinition*> attributes;
	CUtlHashMapLarge<int, CEconCraftingRecipeDefinition*> recipes;
	CUtlMap<const char*, CEconItemSetDefinition> item_sets;
	CUtlHashMapLarge<int, const char*> revolving_loot_lists;
	CUtlHashMapLarge<int, const char*> quest_reward_loot_lists;
	CUtlMap<const char*, CEconLootListDefinition> client_loot_lists;

private:
	int32_t pad4[ 14 ];

public:
	CUtlHashMapLarge<int, AlternateIconData_t> alternate_icons2;

private:
	int32_t pad5[ 18 ];

public:
	CUtlHashMapLarge<int, CPaintKit*> paint_kits;
	CUtlHashMapLarge<int, CStickerKit*> sticker_kits;
	CUtlMap<const char*, CStickerKit*> sticker_kits_namekey;

private:
	int32_t pad6[ 24 ];

public:
	CUtlMap<const char*, KeyValues*> prefabs;
	CUtlVector<CEconColorDefinition*> colors;
	CUtlVector<CEconGraffitiTintDefinition*> graffiti_tints;

private:
	int32_t pad7[ 20 ];

public:
	CUtlHashMapLarge<int, CEconMusicDefinition*> music_definitions;
	CUtlHashMapLarge<int, CEconQuestDefinition*> quest_definitions;
	CUtlHashMapLarge<int, CEconCampaignDefinition*> campaign_definitions;

private:
	int32_t pad8[ 27 ];

public:
	CUtlHashMapLarge<int, CSkirmishModeDefinition*> skirmish_modes;
	CUtlHashMapLarge<unsigned int, CProPlayerData*> pro_players;

private:
	int32_t pad9[ 53 ];

public:
	CUtlMap<unsigned int, kill_eater_score_type_t, unsigned short> kill_eater_score_types;
	CUtlMap<const char*, CUtlVector<CItemLevelingDefinition>*> item_levels;
	CPipRankData skirmish_rank_info;

private:
	int32_t pad10[ 43 ];

public: // This is where CCStrike15ItemSchema begins
	CUtlVector<const char*> m_ClassUsabilityStrings;
	CUtlVector<const char*> m_LoadoutStrings;
	CUtlVector<const char*> m_LoadoutStringsSubPositions;
	CUtlVector<const char*> m_LoadoutStringsForDisplay;

private:
	int32_t pad[ 5 ];

public:
	CUtlVector<const char*> m_LoadoutStringsOrder; // idk how to name this. 1st = primary, 2nd = secondary, 3rd = melee, 4th = grenade, 5th = item
};

struct KnifeData {
	int         m_id;
	std::string m_model, m_world_model, m_name;
	int         m_model_index, m_world_model_index;

	__forceinline KnifeData( ) : m_id{}, m_model{}, m_world_model{}, m_name{}, m_model_index{}, m_world_model_index{} { }

	__forceinline void setup( int id, const std::string& model, const std::string& world_model, const std::string& name ) {
		m_id = id;
		m_model = model;
		m_name = name;
		m_model_index = g_csgo.m_model_info->GetModelIndex( model.data( ) );
		m_world_model_index = g_csgo.m_model_info->GetModelIndex( world_model.data( ) );
	}
};

struct GloveData {
	int         m_id;
	std::string m_model, m_world_model;
	int         m_model_index, m_world_model_index;

	__forceinline GloveData( ) : m_id{}, m_model{}, m_model_index{} { }

	__forceinline void setup( int id, const std::string& model, const std::string& world_model ) {
		m_id = id;
		m_model = model;
		m_world_model = world_model;
		m_model_index = g_csgo.m_model_info->GetModelIndex( model.data( ) );
		m_world_model_index = g_csgo.m_model_info->GetModelIndex( world_model.data( ) );
	}
};

class Skins {
public:
	enum knives_t {
		BAYONET = 1,
		BOWIE,
		BUTTERFLY,
		FALCHION,
		FLIP,
		GUT,
		HUNTSMAN,
		KARAMBIT,
		M9,
		DAGGER,
		KNIFE_MAX
	};

	enum gloves_t {
		BLOODHOUND = 1,
		SPORTY,
		DRIVER,
		HANDWRAP,
		MOTO,
		SPECIALIST,
		GLOVE_MAX
	};

public:
	KnifeData   m_knife_data[ knives_t::KNIFE_MAX ];
	GloveData   m_glove_data[ gloves_t::GLOVE_MAX ];
	int         m_last_seq;
	CBaseHandle m_glove_handle;
	bool        m_update;
	float       m_update_time;

public:
	void load( );
	void think( );
	void UpdateItem( Weapon* item );
	void UpdateAnimations( Entity* ent );
};

extern Skins g_skins;
