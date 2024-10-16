#include <systems/audio_system.h>

using namespace Halley;

class AudioSystem final : public AudioSystemBase<AudioSystem>, public IAudioSystemInterface {
public:
	void init()
	{
		getWorld().setInterface(static_cast<IAudioSystemInterface*>(this));
	}

	void onEntitiesAdded(Span<SourceFamily> es)
	{
		for (auto& e: es) {
			initSource(e);
		}
	}

	void onEntitiesRemoved(Span<SourceFamily> es)
	{
		for (auto& e: es) {
			deInitSource(e);
		}
	}

	void onEntitiesReloaded(Span<SourceFamily*> es)
	{
		for (auto& e: es) {
			initSource(*e);
		}
	}

	void update(Time t)
	{
		updateListeners(t);
		updateSources(t);
	}

	void onMessageReceived(const PlayNetworkSoundSystemMessage& msg) override
	{
		if (const auto* source = sourceFamily.tryFind(msg.emitter)) {
			getAPI().audio->postEvent(msg.event, source->audioSource.emitter);
		}
	}

	void playAudio(const String& eventId, EntityId entityId) override
	{
		if (const auto* source = sourceFamily.tryFind(entityId)) {
			getAPI().audio->postEvent(eventId, source->audioSource.emitter);
			return;
		}

		// Not a source
		auto e = getWorld().tryGetEntity(entityId);
		if (e.isValid()) {
			if (const auto* transform2d = e.tryGetComponent<Transform2DComponent>(entityId)) {
				auto pos = transform2d->getWorldPosition();
				playAudio(eventId, pos, findRegion(pos));
				return;
			}
		}

		// Fallback
		getAPI().audio->postEvent(eventId);
	}

	void playAudio(const String& event, WorldPosition position, std::optional<AudioRegionId> regionId) override
	{
		auto emitter = getAPI().audio->createEmitter(AudioPosition::makePositional(position.pos));
		if (regionId) {
			emitter->setRegion(*regionId);
		} else {
			emitter->setRegion(findRegion(position).value_or(0));
		}
		emitter->detach();
		getAPI().audio->postEvent(event, emitter);
	}

	void setVariable(EntityId entityId, const String& variableName, float value) override
	{
		if (const auto* source = sourceFamily.tryFind(entityId)) {
			source->audioSource.emitter->setVariable(variableName, value);
		}
	}

	String getSourceName(AudioEmitterId id) const override
	{
		for (const auto& source: sourceFamily) {
			if (source.audioSource.emitter && source.audioSource.emitter->getId() == id) {
				return getWorld().getEntity(source.entityId).getName();
			}
		}
		return "<unknown>";
	}

	String getRegionName(AudioRegionId id) const override
	{
		return getAPI().audio->getRegionName(id);
	}

	std::optional<AudioRegionId> findRegion(WorldPosition pos)
	{
		if (regionLookup) {
			return regionLookup(pos);
		}
		return std::nullopt;
	}

	void setRegionLookup(std::function<AudioRegionId(WorldPosition pos)> f) override
	{
		regionLookup = f;
	}

private:
	String curRegionId;
	String curRegionPreset;
	String curFloorType;
	std::function<AudioRegionId(WorldPosition pos)> regionLookup;
	
	void updateListeners(Time t)
	{
		if (t < 0.00001) {
			t = 0.00001;
		}

		AudioAPI& audio = *getAPI().audio;
		for (auto& listener: listenerFamily) {
			const auto pos = Vector3f(listener.transform2D.getGlobalPosition());
			const auto lastPos = listener.audioListener.lastPos;
			const auto deltaPos = pos - lastPos;
			const auto vel = deltaPos.length() < 15.0f ? deltaPos / static_cast<float>(t) : Vector3f();
			listener.audioListener.velAverage.add(vel);
			listener.audioListener.lastPos = pos;
			audio.setListener(AudioListenerData(lastPos, listener.audioListener.velAverage.getMean(), listener.audioListener.referenceDistance, listener.audioListener.speedOfSound, listener.audioListener.regions));
		}
	}

	AudioPosition getAudioPosition(SourceFamily& e, Vector3f vel)
	{
		return AudioPosition::makePositional(Vector3f(e.transform2D.getGlobalPosition()), AudioAttenuation(e.audioSource.rangeMin, e.audioSource.rangeMax, e.audioSource.rollOff, e.audioSource.curve), vel);
	}

	void initSource(SourceFamily& e)
	{
		AudioAPI& audio = *getAPI().audio;
		e.audioSource.emitter = getAPI().audio->createEmitter(getAudioPosition(e, {}));
		if (e.audioSource.event) {
			audio.postEvent(*e.audioSource.event, e.audioSource.emitter);
		}
	}

	void deInitSource(SourceFamily& e)
	{
		e.audioSource.emitter = {};
	}

	void updateSources(Time t)
	{
		if (t < 0.00001) {
			t = 0.00001;
		}

		for (auto& source: sourceFamily) {
			Vector3f vel;
			const auto pos = Vector3f(source.transform2D.getGlobalPosition());
			const auto lastPos = source.audioSource.lastPos;
			source.audioSource.lastPos = pos;
			source.audioSource.moved = pos != lastPos;

			if (source.velocity) {
				vel = Vector3f(source.velocity->velocity, 0);
			} else if (source.audioSource.canAutoVel) {
				vel = (pos - lastPos) / static_cast<float>(t);
			}

			source.audioSource.emitter->setPosition(getAudioPosition(source, vel));
		}
	}
};

REGISTER_SYSTEM(AudioSystem)
