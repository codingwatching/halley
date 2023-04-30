#include "halley/maths/colour_gradient.h"
#include "halley/bytes/byte_serializer.h"
#include "halley/bytes/config_node_serializer.h"

using namespace Halley;

ColourGradient::ColourGradient()
{
	makeDefault();
}

ColourGradient::ColourGradient(const ConfigNode& node)
{
	if (node.getType() == ConfigNodeType::Map) {
		positions = node["positions"].asVector<float>({});
		colours = node["colours"].asVector<Colour4f>({});
	} else {
		makeDefault();
	}
}

ConfigNode ColourGradient::toConfigNode() const
{
	ConfigNode::MapType result;
	result["positions"] = positions;
	result["colours"] = colours;
	return result;
}

void ColourGradient::makeDefault()
{
	positions.clear();
	positions.push_back(0);
	positions.push_back(1);
	colours.clear();
	colours.push_back(Colour4f(1, 1, 1, 1));
	colours.push_back(Colour4f(1, 1, 1, 1));
}

bool ColourGradient::operator==(const ColourGradient& other) const
{
	return colours == other.colours;
}

bool ColourGradient::operator!=(const ColourGradient& other) const
{
	return colours != other.colours;
}

void ColourGradient::serialize(Serializer& s) const
{
	s << colours;
}

void ColourGradient::deserialize(Deserializer& s)
{
	s >> colours;
}

Colour4f ColourGradient::evaluate(float val) const
{
	// Before first point
	if (val < positions.front()) {
		return colours.front();
	}

	// Between two positions
	for (size_t i = 1; i < positions.size(); ++i) {
		const float prevX = positions[i - 1];
		const float nextX = positions[i];

		if (val >= prevX && val < nextX) {
			const float t = (val - prevX) / (nextX - prevX);
			assert(t >= 0.0f);
			assert(t <= 1.0f);
			return lerp(colours[i - 1], colours[i], t);
		}
	}

	// After last point
	return colours.back();
}
