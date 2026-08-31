#pragma once
#include "Protocol.pb.h"
#include "Network/SendBuffer.h"
#include "Network/ServerSession.h"

using namespace Craft;

// jinja2 템플릿 엔진을 활용한 코드 자동화용 템플릿(클라이언트 용)

using PacketHandlerFunc = std::function<bool(const Session*, BYTE*, int32)>;
extern PacketHandlerFunc GPacketHandler[UINT16_MAX];

// PKT enum 자동화
enum : uint16
{
{%- for pkt in parser.total_pkt %}
	PKT_{{pkt.name}} = {{pkt.id}},
{%- endfor %}
};

bool Handle_INVALID(const Session* session, BYTE* buffer, int32 len);

// PKT handle 함수 자동 선언, 선언부만 만들어주기 때문에 정의부를 따로 생성해야 함

{%- for pkt in parser.recv_pkt %}
bool Handle_{{pkt.name}}(const Session* session, Protocol::{{pkt.name}}& pkt);
{%- endfor %}

// PacketHandler 클래스 자동화
class {{output}}
{
public:
	static void Init()
	{
		for (int32 i = 0; i < UINT16_MAX; ++i)
			GPacketHandler[i] = Handle_INVALID;

		// Handler 함수 등록 자동화
{%- for pkt in parser.recv_pkt %}
		GPacketHandler[PKT_{{pkt.name}}] = [](const Session* session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::{{pkt.name}}>(Handle_{{pkt.name}}, session, buffer, len); };
{%- endfor %}

	}

	static bool HandlePacket(const Session* session, BYTE* buffer, int32 len)
	{
		PacketHeader* header = reinterpret_cast<PacketHeader*>(buffer);
		return GPacketHandler[header->id](session, buffer, len);
	}

	// sendbuffer 작성 자동화, 선언부만 만들어주기 때문에 정의부를 따로 생성해야 함
{%- for pkt in parser.send_pkt %}
	static BYTE* MakeSendBuffer(Protocol::{{pkt.name}}& pkt, OUT int32& size) {return MakeSendBuffer(pkt, PKT_{{pkt.name}}, size); }
{%- endfor %}


private:
	template<typename PacketType, typename ProcessFunc>
	static bool HandlePacket(ProcessFunc func, const Session* session, BYTE* buffer, uint32 len)
	{
		PacketType pkt;
		if (pkt.ParseFromArray(buffer + sizeof(PacketHeader), len - sizeof(PacketHeader)) == false)
			return false;

		return func(session, pkt);
	}

	template<typename T>
	static BYTE* MakeSendBuffer(T& pkt, uint16 pktId, OUT int32& size)
	{
		uint16 dataSize = static_cast<uint16>(pkt.ByteSizeLong());
		uint16 packetSize = dataSize + sizeof(PacketHeader);

		BYTE* bufferChunk = LBufferChunk->Open(packetSize);
		PacketHeader* header = reinterpret_cast<PacketHeader*>(bufferChunk);
		header->size = packetSize;
		header->id = pktId;
		ASSERT_CRASH(pkt.SerializeToArray((&header[1]), dataSize));
		LBufferChunk->Close(packetSize);

		size = packetSize;

		return bufferChunk;
	}
};

