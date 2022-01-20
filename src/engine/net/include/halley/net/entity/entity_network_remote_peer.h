#pragma once

#include "halley/data_structures/hash_map.h"
#include "halley/entity/entity.h"
#include "../session/network_session.h"
#include "halley/entity/entity_factory.h"
#include "halley/time/halleytime.h"

namespace Halley {
	class EntityNetworkSession;
	struct EntityId;
    class EntityData;

    class EntityNetworkRemotePeer {
    public:
        EntityNetworkRemotePeer(EntityNetworkSession& parent, NetworkSession::PeerId peerId);

        NetworkSession::PeerId getPeerId() const;
        void sendEntities(Time t, gsl::span<const std::pair<EntityId, uint8_t>> entityIds);
        void receiveEntities();

        void destroy(World& world);
        bool isAlive() const;

    private:
        using NetworkEntityId = uint16_t;

        enum class EntityHeaderType : uint8_t {
	        Create,
        	Update,
        	Destroy
        };
    	
    	struct EntityHeader { // Inefficiency: this occupies 4 bytes, but only needs 3
            NetworkEntityId entityId;
            EntityHeaderType type;

            EntityHeader() = default;
    		EntityHeader(EntityHeaderType type, NetworkEntityId entityId) : entityId(entityId), type(type) {}
    	};
        
        class OutboundEntity {
        public:
            bool alive = true;
            Time timeSinceSend = 0;
            NetworkEntityId networkId = 0;
            EntityData data;
        };

        class InboundEntity {
        public:
            EntityId worldId;
            EntityData data;
        };

        EntityNetworkSession* parent = nullptr;
        NetworkSession::PeerId peerId;
    	bool alive = true;
    	
        HashMap<EntityId, OutboundEntity> outboundEntities;
        HashMap<NetworkEntityId, InboundEntity> inboundEntities;

    	HashSet<NetworkEntityId> allocatedOutboundIds;
        uint16_t nextId = 0;

        void createEntity(EntityRef entity);
        void updateEntity(Time t, OutboundEntity& remote, EntityRef entity);
        void destroyEntity(OutboundEntity& remote);
        uint16_t assignId();
    };
}