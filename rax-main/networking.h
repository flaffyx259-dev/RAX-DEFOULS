#pragma once

struct Packet {
	// cheat detection.
	int m_hash;
	// network angle.
	int m_yaw;
	// user data.
	char m_username[ 12 ];

	__forceinline bool IsRax( ) const {
		return m_hash == HASH_DEBUG || m_hash == HASH_RELEASE;
	}

	__forceinline void Reset( ) {
		m_hash = 0;
	}
};

// what in the actual FUCK 
struct _String_t {
	char data[ 16 ]{ };
	uint32_t current_len = 0;
	uint32_t max_len = 15;
};

struct VoiceData {
	uint32_t m_xuid_low{ };
	uint32_t m_xuid_high{ };
	int32_t m_format; //0x0020
	int32_t m_sequence_bytes; //0x0024
	uint32_t m_section_number; //0x0028
	uint32_t m_uncompressed_sample_offset; //0x002C
};

struct CCLCMsg_VoiceData {
	uint32_t m_vtable; //0x0000
	char pad_0004[ 4 ]; //0x0004
	uint32_t m_vtable2; //0x0008
	char pad_000C[ 8 ]; //0x000C
	void* m_data; //0x0014
	uint32_t m_xuid_low{ };
	uint32_t m_xuid_high{ };
	int32_t m_format; //0x0020
	int32_t m_sequence_bytes; //0x0024
	uint32_t m_section_number; //0x0028
	uint32_t m_uncompressed_sample_offset; //0x002C
	int32_t m_cached_size; //0x0030

	uint32_t m_flags; //0x0034
	uint8_t m_no_stack_overflow[ 0xFF ];

	__forceinline uint8_t* GetRawData( ) {
		return ( uint8_t* )this;
	}

	__forceinline void SetData( VoiceData* data ) {
		m_xuid_low = data->m_xuid_low;
		m_xuid_high = data->m_xuid_high;
		m_sequence_bytes = data->m_sequence_bytes;
		m_section_number = data->m_section_number;
		m_uncompressed_sample_offset = data->m_uncompressed_sample_offset;
	}
};

struct CSVCMsg_VoiceData {
	char pad_0000[ 8 ]; //0x0000
	int32_t client; //0x0008
	int32_t audible_mask; //0x000C
	uint32_t xuid_low{ };
	uint32_t xuid_high{ };
	void* voide_data_; //0x0018
	int32_t proximity; //0x001C
	//int32_t caster; //0x0020
	int32_t format; //0x0020
	int32_t sequence_bytes; //0x0024
	uint32_t section_number; //0x0028
	uint32_t uncompressed_sample_offset; //0x002C

	__forceinline VoiceData GetData( ) const {
		VoiceData cdata;
		cdata.m_xuid_low = xuid_low;
		cdata.m_xuid_high = xuid_high;
		cdata.m_sequence_bytes = sequence_bytes;
		cdata.m_section_number = section_number;
		cdata.m_uncompressed_sample_offset = uncompressed_sample_offset;
		return cdata;
	}
};

class Networking {
private:
	Packet m_players[ 64 ];
public:
	void SendData( );
	void RecieveData( const CSVCMsg_VoiceData* msg );
	Packet* GetUserData( int index );
};

extern Networking g_networking;