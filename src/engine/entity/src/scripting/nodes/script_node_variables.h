#pragma once
#include "scripting/script_environment.h"

namespace Halley {
	class ScriptVariable final : public ScriptNodeTypeBase<void> {
	public:
		String getId() const override { return "variable"; }
		String getName() const override { return "Variable"; }
		String getIconName(const ScriptGraphNode& node) const override { return "script_icons/variable.png"; }
		ScriptNodeClassification getClassification() const override { return ScriptNodeClassification::Variable; }

		String getLabel(const ScriptGraphNode& node) const override;
		String getShortDescription(const World* world, const ScriptGraphNode& node, const ScriptGraph& graph, ScriptPinId elementIdx) const override;
		gsl::span<const PinType> getPinConfiguration(const ScriptGraphNode& node) const override;
		Vector<SettingType> getSettingTypes() const override;
		std::pair<String, Vector<ColourOverride>> getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const override;

		ConfigNode doGetData(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pinN) const override;
		void doSetData(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pinN, ConfigNode data) const override;

	private:
		ScriptVariableScope getScope(const ScriptGraphNode& node) const;
	};
	
	class ScriptLiteral final : public ScriptNodeTypeBase<void> {
	public:
		String getId() const override { return "literal"; }
		String getName() const override { return "Literal"; }
		String getLabel(const ScriptGraphNode& node) const override;
		String getShortDescription(const World* world, const ScriptGraphNode& node, const ScriptGraph& graph, ScriptPinId elementIdx) const override;
		String getIconName(const ScriptGraphNode& node) const override { return "script_icons/literal.png"; }
		gsl::span<const PinType> getPinConfiguration(const ScriptGraphNode& node) const override;
		Vector<SettingType> getSettingTypes() const override;
		ScriptNodeClassification getClassification() const override { return ScriptNodeClassification::Variable; }
		std::pair<String, Vector<ColourOverride>> getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const override;

		ConfigNode doGetData(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pinN) const override;

	private:
		ConfigNode getConfigNode(const ScriptGraphNode& node) const;
	};

	class ScriptComparison final : public ScriptNodeTypeBase<void> {
	public:
		String getId() const override { return "comparison"; }
		String getName() const override { return "Comparison"; }
		String getIconName(const ScriptGraphNode& node) const override { return "script_icons/comparison.png"; }
		ScriptNodeClassification getClassification() const override { return ScriptNodeClassification::Variable; }
		
		String getLabel(const ScriptGraphNode& node) const override;
		String getShortDescription(const World* world, const ScriptGraphNode& node, const ScriptGraph& graph, ScriptPinId elementIdx) const override;
		gsl::span<const PinType> getPinConfiguration(const ScriptGraphNode& node) const override;
		Vector<SettingType> getSettingTypes() const override;
		std::pair<String, Vector<ColourOverride>> getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const override;

		ConfigNode doGetData(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pinN) const override;
	};
	
	class ScriptArithmetic final : public ScriptNodeTypeBase<void> {
	public:
		String getId() const override { return "arithmetic"; }
		String getName() const override { return "Arithmetic"; }
		String getIconName(const ScriptGraphNode& node) const override { return "script_icons/arithmetic.png"; }
		ScriptNodeClassification getClassification() const override { return ScriptNodeClassification::Variable; }

		String getLabel(const ScriptGraphNode& node) const override;
		String getShortDescription(const World* world, const ScriptGraphNode& node, const ScriptGraph& graph, ScriptPinId elementIdx) const override;
		gsl::span<const PinType> getPinConfiguration(const ScriptGraphNode& node) const override;
		Vector<SettingType> getSettingTypes() const override;
		std::pair<String, Vector<ColourOverride>> getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const override;

		ConfigNode doGetData(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pinN) const override;
	};
	
	class ScriptSetVariable final : public ScriptNodeTypeBase<void> {
	public:
		String getId() const override { return "setVariable"; }
		String getName() const override { return "Variable Set"; }
		String getLabel(const ScriptGraphNode& node) const override;
		String getIconName(const ScriptGraphNode& node) const override { return "script_icons/set_variable.png"; }
		gsl::span<const PinType> getPinConfiguration(const ScriptGraphNode& node) const override;
		ScriptNodeClassification getClassification() const override { return ScriptNodeClassification::Action; }
		Vector<SettingType> getSettingTypes() const override;
		std::pair<String, Vector<ColourOverride>> getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const override;

		Result doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node) const override;
	};

	class ScriptHoldVariableData : public ScriptStateData<ScriptHoldVariableData> {
	public:
		ConfigNode prevValue;

		ScriptHoldVariableData() = default;
		ScriptHoldVariableData(const ConfigNode& node);
		ConfigNode toConfigNode(const EntitySerializationContext& context) override;
	};

	class ScriptHoldVariable final : public ScriptNodeTypeBase<ScriptHoldVariableData> {
	public:
		String getId() const override { return "holdVariable"; }
		String getName() const override { return "Variable Hold"; }
		String getLabel(const ScriptGraphNode& node) const override;
		String getIconName(const ScriptGraphNode& node) const override { return "script_icons/set_variable.png"; }
		gsl::span<const PinType> getPinConfiguration(const ScriptGraphNode& node) const override;
		ScriptNodeClassification getClassification() const override { return ScriptNodeClassification::Action; }
		bool hasDestructor() const override { return true; }
		Vector<SettingType> getSettingTypes() const override;
		std::pair<String, Vector<ColourOverride>> getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const override;

		void doInitData(ScriptHoldVariableData& data, const ScriptGraphNode& node, const EntitySerializationContext& context, const ConfigNode& nodeData) const override;
		Result doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node, ScriptHoldVariableData& curData) const override;
		void doDestructor(ScriptEnvironment& environment, const ScriptGraphNode& node, ScriptHoldVariableData& curData) const override;
	};

	class ScriptEntityIdToData final : public ScriptNodeTypeBase<void> {
	public:
		String getId() const override { return "convEntityIdToData"; }
		String getName() const override { return "Conv EntityId->Data"; }
		String getIconName(const ScriptGraphNode& node) const override { return "script_icons/convEntityIdToData.png"; }
		ScriptNodeClassification getClassification() const override { return ScriptNodeClassification::Variable; }
		
		gsl::span<const PinType> getPinConfiguration(const ScriptGraphNode& node) const override;
		String getShortDescription(const World* world, const ScriptGraphNode& node, const ScriptGraph& graph, ScriptPinId elementIdx) const override;
		std::pair<String, Vector<ColourOverride>> getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const override;

		ConfigNode doGetData(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pinN) const override;
	};

	class ScriptDataToEntityId final : public ScriptNodeTypeBase<void> {
	public:
		String getId() const override { return "convDataToEntityId"; }
		String getName() const override { return "Conv Data->EntityId"; }
		String getIconName(const ScriptGraphNode& node) const override { return "script_icons/convDataToEntityId.png"; }
		ScriptNodeClassification getClassification() const override { return ScriptNodeClassification::Variable; }
		
		gsl::span<const PinType> getPinConfiguration(const ScriptGraphNode& node) const override;
		String getShortDescription(const World* world, const ScriptGraphNode& node, const ScriptGraph& graph, ScriptPinId elementIdx) const override;
		std::pair<String, Vector<ColourOverride>> getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const override;

		EntityId doGetEntityId(ScriptEnvironment& environment, const ScriptGraphNode& node, ScriptPinId pinN) const override;
	};
	
}