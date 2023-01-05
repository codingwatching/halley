#pragma once
#include "halley/editor_extensions/entity_validator.h"

namespace Halley {
	class EntityList;
	class EntityValidator;
	class SceneEditorWindow;

	class EntityValidatorUI : public UIWidget {
	public:
		EntityValidatorUI(String id, UIFactory& factory);

		void onMakeUI() override;

		void setValidator(EntityValidator* validator);
		void setEntity(EntityData& entity, IEntityEditor& entityEditor, Resources& gameResources);
		void unloadEntity();
		void refresh();
		void setSceneEditorWindow(SceneEditorWindow* sceneEditor);

		static void setSeverity(UIWidget& widget, UIFactory& factory, IEntityValidator::Severity severity);

	private:
		UIFactory& factory;
		SceneEditorWindow* sceneEditorWindow = nullptr;

		EntityValidator* validator = nullptr;
		IEntityEditor* entityEditor = nullptr;
		Resources* gameResources = nullptr;

		EntityData* curEntity = nullptr;
		EntityData curEntityInstance;
		const EntityTree* curTree = nullptr;
		bool isPrefab = false;

		Vector<IEntityValidator::Result> curResultSet;
	};

	class EntityValidatorListUI : public UIWidget {
	public:
		EntityValidatorListUI(String id, UIFactory& factory);
		
		void onMakeUI() override;
		void update(Time t, bool moved) override;

		void setList(std::weak_ptr<EntityList> entityList);
		void setInvalidEntities(Vector<std::pair<int, IEntityValidator::Severity>> entities);

	private:
		UIFactory& factory;
		std::weak_ptr<EntityList> entityList;
		Vector<int> invalidEntities;
		std::shared_ptr<UILabel> description;

		void move(int delta);
	};
}
