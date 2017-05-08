#pragma once
#include "api/halley_api_internal.h"
#include "halley/net/connection/network_service.h"

namespace Halley {
	class DummyNetworkAPI : public NetworkAPIInternal
	{
	public:
		void init() override;
		void deInit() override;

		std::unique_ptr<NetworkService> createService(int port) override;
	};

	class DummyNetworkService : public NetworkService
	{
	public:
		void update() override;
		void setAcceptingConnections(bool accepting) override;
		std::shared_ptr<IConnection> tryAcceptConnection() override;
		std::shared_ptr<IConnection> connect(String address, int port) override;
	};
}
