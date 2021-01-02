#include "scene_editor_window.h"
#include "choose_asset_window.h"
#include "entity_editor.h"
#include "entity_list.h"
#include "halley/entity/entity_factory.h"
#include "halley/entity/prefab_scene_data.h"
#include "halley/entity/world.h"
#include "halley/tools/file/filesystem.h"
#include "halley/tools/project/project.h"
#include "halley/file_formats/yaml_convert.h"
#include "halley/ui/ui_factory.h"
#include "scene_editor_canvas.h"
#include "scene_editor_game_bridge.h"
#include "src/ui/project_window.h"
using namespace Halley;

SceneEditorWindow::SceneEditorWindow(UIFactory& factory, Project& project, const HalleyAPI& api, ProjectWindow& projectWindow)
	: UIWidget("scene_editor", {}, UISizer())
	, api(api)
	, uiFactory(factory)
	, project(project)
	, projectWindow(projectWindow)
	, gameBridge(std::make_shared<SceneEditorGameBridge>(api, uiFactory.getResources(), uiFactory, project))
{
	makeUI();

	project.withDLL([&] (DynamicLibrary& dll)
	{
		dll.addReloadListener(*this);
	});
}

SceneEditorWindow::~SceneEditorWindow()
{
	unloadScene();

	project.withDLL([&] (DynamicLibrary& dll)
	{
		dll.removeReloadListener(*this);
	});
}

void SceneEditorWindow::makeUI()
{
	add(uiFactory.makeUI("ui/halley/scene_editor_window"), 1);
	
	canvas = getWidgetAs<SceneEditorCanvas>("canvas");
	canvas->setSceneEditorWindow(*this);
	canvas->setGameBridge(*gameBridge);
	
	entityList = getWidgetAs<EntityList>("entityList");
	entityList->setSceneEditorWindow(*this);

	entityEditor = getWidgetAs<EntityEditor>("entityEditor");
	entityEditor->setSceneEditorWindow(*this);

	toolMode = getWidgetAs<UIList>("toolMode");
	
	setModified(false);

	setHandle(UIEventType::ListSelectionChanged, "entityList_list", [=] (const UIEvent& event)
	{
		onEntitySelected(event.getStringData());
	});

	setHandle(UIEventType::ListSelectionChanged, "toolMode", [=] (const UIEvent& event)
	{
		if (toolModeTimeout == 0) {
			setTool(fromString<SceneEditorTool>(event.getStringData()));
			toolModeTimeout = 2;
		}
	});

	setHandle(UIEventType::ListAccept, "entityList_list", [=](const UIEvent& event)
	{
		panCameraToEntity(event.getStringData());
	});

	setHandle(UIEventType::ButtonClicked, "saveButton", [=] (const UIEvent& event)
	{
		saveScene();
	});

	setHandle(UIEventType::ButtonClicked, "addEntity", [=] (const UIEvent& event)
	{
		addNewEntity();
	});

	setHandle(UIEventType::ButtonClicked, "addPrefab", [=] (const UIEvent& event)
	{
		addNewPrefab();
	});

	setHandle(UIEventType::ButtonClicked, "removeEntity", [=] (const UIEvent& event)
	{
		removeEntity();
	});
}

void SceneEditorWindow::onAddedToRoot()
{
	getRoot()->registerKeyPressListener(shared_from_this());
}

void SceneEditorWindow::loadScene(const String& name)
{
	unloadScene();
	assetPath = project.getAssetsSrcPath() / project.getImportAssetsDatabase().getPrimaryInputFile(AssetType::Scene, name);

	if (!name.isEmpty()) {
		loadScene(AssetType::Scene, *project.getGameResources().get<Scene>(name));
	}
}

void SceneEditorWindow::loadPrefab(const String& name)
{
	unloadScene();
	assetPath = project.getAssetsSrcPath() / project.getImportAssetsDatabase().getPrimaryInputFile(AssetType::Prefab, name);

	if (!name.isEmpty()) {
		loadScene(AssetType::Prefab, *project.getGameResources().get<Prefab>(name));
	}
}

void SceneEditorWindow::loadScene(AssetType assetType, const Prefab& origPrefab)
{
	gameBridge->initializeInterfaceIfNeeded();
	if (gameBridge->isLoaded()) {
		auto& interface = gameBridge->getInterface();
		auto& world = interface.getWorld();

		// Load prefab
		prefab = origPrefab.clone();
		origPrefabAssetType = assetType;

		// Spawn scene
		entityFactory = std::make_shared<EntityFactory>(world, project.getGameResources());
		auto sceneCreated = entityFactory->createScene(prefab, true);
		interface.spawnPending();

		// Setup editors
		sceneData = std::make_shared<PrefabSceneData>(*prefab, entityFactory, world, project.getGameResources());
		entityEditor->setECSData(project.getECSData());
		entityEditor->addFieldFactories(interface.getComponentEditorFieldFactories());
		entityList->setSceneData(sceneData);

		setTool(SceneEditorTool::Translate);

		// Show root
		if (!sceneCreated.getEntities().empty()) {
			panCameraToEntity(sceneCreated.getEntities().at(0).getInstanceUUID().toString());
		}
		currentEntityScene = sceneCreated;

		// Custom UI
		setCustomUI(gameBridge->makeCustomUI());

		// Console
		setupConsoleCommands();

		// Done
		gameBridge->onSceneLoaded(*prefab);
	}
}

void SceneEditorWindow::unloadScene()
{
	setCustomUI({});

	currentEntityId = "";
	if (gameBridge->isLoaded()) {
		auto& interface = gameBridge->getInterface();
		auto& world = interface.getWorld();
		const auto& cameraIds = interface.getCameraIds();
		for (auto& e: world.getTopLevelEntities()) {
			if (std::find(cameraIds.begin(), cameraIds.end(), e.getEntityId()) == cameraIds.end()) {
				world.destroyEntity(e);
			}
		}
		world.spawnPending();
		gameBridge->unload();
	}
	entityFactory.reset();
	sceneData.reset();
	currentEntityScene.reset();
	entityEditor->unloadEntity();
	entityEditor->resetFieldFactories();
}

void SceneEditorWindow::update(Time t, bool moved)
{
	if (toolModeTimeout > 0) {
		--toolModeTimeout;
	}

	if (currentEntityScene && entityFactory) {
		if (currentEntityScene->needsUpdate()) {
			currentEntityScene->updateOnEditor(*entityFactory);
		}
	}
}

bool SceneEditorWindow::onKeyPress(KeyboardKeyPress key)
{
	if (key.is(KeyCode::S, KeyMods::Ctrl)) {
		saveScene();
		return true;
	}

	if (key.is(KeyCode::F1)) {
		toggleConsole();
		return true;
	}

	return false;
}

void SceneEditorWindow::onUnloadDLL()
{
	unloadScene();
}

void SceneEditorWindow::onLoadDLL()
{
	if (prefab) {
		loadScene(origPrefabAssetType, *prefab);
	}
}

void SceneEditorWindow::selectEntity(const String& id)
{
	entityList->select(id);
}

void SceneEditorWindow::selectEntity(const std::vector<UUID>& candidates)
{
	const auto tree = sceneData->getEntityTree();
	for (auto& c: candidates) {
		const auto found = tree.contains(c.toString());
		if (found) {
			entityList->select(c.toString());
			break;
		}
	}
	entityList->select("");
}

void SceneEditorWindow::onEntitySelected(const String& id)
{
	decayTool();
	
	String actualId = id;
	if (actualId.isEmpty()) {
		const auto& tree = sceneData->getEntityTree();
		if (tree.entityId.isEmpty()) {
			if (tree.children.empty()) {
				EntityData empty;
				entityEditor->loadEntity("", empty, nullptr, false, project.getGameResources());
				currentEntityId = "";
				return;
			} else {
				actualId = tree.children[0].entityId;
			}
		} else {
			actualId = tree.entityId;
		}
	}

	auto& entityData = sceneData->getEntityNodeData(actualId).data;
	const Prefab* prefabData = nullptr;
	const String prefabName = entityData.getPrefab();
	if (!prefabName.isEmpty()) {
		prefabData = getGamePrefab(prefabName).get();
	}
	
	entityEditor->loadEntity(actualId, entityData, prefabData, false, project.getGameResources());
	gameBridge->setSelectedEntity(UUID(actualId), entityData);
	currentEntityId = actualId;
}

void SceneEditorWindow::panCameraToEntity(const String& id)
{
	gameBridge->showEntity(UUID(id));
}

void SceneEditorWindow::saveScene()
{
	setModified(false);

	const auto strData = prefab->toYAML();
	auto data = gsl::as_bytes(gsl::span<const char>(strData.c_str(), strData.length()));
	FileSystem::writeFile(assetPath, data);
	project.notifyAssetFileModified(assetPath);
}

void SceneEditorWindow::markModified()
{
	setModified(true);
}

void SceneEditorWindow::onEntityAdded(const String& id, const String& parentId, const String& afterSiblingId)
{
	auto& data = sceneData->getEntityNodeData(id).data;
	entityList->onEntityAdded(id, parentId, afterSiblingId, data);
	sceneData->reloadEntity(parentId.isEmpty() ? id : parentId);
	onEntitySelected(id);

	gameBridge->onEntityAdded(UUID(id), data);
	
	markModified();
}

void SceneEditorWindow::onEntityRemoved(const String& id, const String& parentId)
{
	gameBridge->onEntityRemoved(UUID(id));

	entityList->onEntityRemoved(id, parentId);
	sceneData->reloadEntity(parentId.isEmpty() ? id : parentId);
	onEntitySelected(parentId);

	markModified();
}

void SceneEditorWindow::onEntityModified(const String& id)
{
	if (!id.isEmpty()) {
		const auto& data = sceneData->getEntityNodeData(id).data;

		entityList->onEntityModified(id, data);

		sceneData->reloadEntity(id);
		
		gameBridge->onEntityModified(UUID(id), data);
	}

	markModified();
}

void SceneEditorWindow::onEntityMoved(const String& id)
{
	if (currentEntityId == id) {
		onEntitySelected(id);
	}

	gameBridge->onEntityMoved(UUID(id), sceneData->getEntityNodeData(id).data);
	
	markModified();
}

void SceneEditorWindow::onComponentRemoved(const String& name)
{
	if (name == curComponentName) {
		decayTool();
	}
}

void SceneEditorWindow::onFieldChangedByGizmo(const String& componentName, const String& fieldName)
{
	entityEditor->onFieldChangedByGizmo(componentName, fieldName);
	onEntityModified(currentEntityId);
}

void SceneEditorWindow::setTool(SceneEditorTool tool)
{
	if (curTool != tool) {
		setTool(tool, "", "", ConfigNode());
	}
}

void SceneEditorWindow::setTool(SceneEditorTool tool, const String& componentName, const String& fieldName, ConfigNode options)
{
	options = gameBridge->onToolSet(tool, componentName, fieldName, std::move(options));

	curTool = tool;
	curComponentName = componentName;
	
	setToolUI(canvas->setTool(tool, componentName, fieldName, options));
	
	toolMode->setItemActive("polygon", tool == SceneEditorTool::Polygon);
	toolMode->setItemActive("vertex", tool == SceneEditorTool::Vertex);
	toolMode->setSelectedOptionId(toString(tool));
}

std::shared_ptr<const Prefab> SceneEditorWindow::getGamePrefab(const String& id) const
{
	if (project.getGameResources().exists<Prefab>(id)) {
		return project.getGameResources().get<Prefab>(id);
	}
	return {};
}

void SceneEditorWindow::copyEntityToClipboard(const String& id)
{
	const auto clipboard = api.system->getClipboard();
	if (clipboard) {
		clipboard->setData(copyEntity(id));
	}
}

void SceneEditorWindow::pasteEntityFromClipboard(const String& referenceId)
{
	const auto clipboard = api.system->getClipboard();
	if (clipboard) {
		auto clipboardData = clipboard->getStringData();
		if (clipboardData) {
			pasteEntity(clipboardData.value(), referenceId);
		}
	}
}

String SceneEditorWindow::copyEntity(const String& id)
{
	const auto entityData = sceneData->getEntityNodeData(id);
	return serializeEntity(entityData.data);
}

void SceneEditorWindow::pasteEntity(const String& stringData, const String& referenceId)
{
	auto data = deserializeEntity(stringData);
	if (data) {
		assignUUIDs(data.value());
		addEntity(referenceId, false, std::move(data.value()));
	}
}

void SceneEditorWindow::duplicateEntity(const String& id)
{
	pasteEntity(copyEntity(id), findParent(id));
}

void SceneEditorWindow::openEditPrefabWindow(const String& name)
{
	projectWindow.openAsset(AssetType::Prefab, name);
}

const std::shared_ptr<ISceneData>& SceneEditorWindow::getSceneData() const
{
	return sceneData;
}

void SceneEditorWindow::addNewEntity()
{
	EntityData data;
	data.setInstanceUUID(UUID::generate());
	addEntity(std::move(data));
}

void SceneEditorWindow::addNewPrefab()
{
	getRoot()->addChild(std::make_shared<ChooseAssetTypeWindow>(uiFactory, AssetType::Prefab, "", project.getGameResources(), [=] (std::optional<String> result)
	{
		if (result) {
			addNewPrefab(result.value());
		}
	}));
}

void SceneEditorWindow::addNewPrefab(const String& prefabName)
{
	const auto prefab = getGamePrefab(prefabName);
	if (prefab) {
		const auto& entityData = prefab->getEntityData();
		auto components = std::vector<std::pair<String, ConfigNode>>();

		// Clone transform components
		for (const auto& kv: entityData.getComponents()) {
			if (kv.first == "Transform2D" || kv.first == "Transform3D") {
				components.emplace_back(kv);
			}
		}

		EntityData data;
		data.setInstanceUUID(UUID::generate());
		data.setPrefab(prefabName);
		data.setComponents(components);
		addEntity(std::move(data));
	}
}

void SceneEditorWindow::addEntity(EntityData data)
{
	addEntity(currentEntityId, false, std::move(data));
}

void SceneEditorWindow::addEntity(const String& referenceEntity, bool childOfReference, EntityData data)
{
	if (referenceEntity.isEmpty()) {
		addEntity("", referenceEntity, std::move(data));
	} else {
		const bool isScene = sceneData->getEntityNodeData("").data.isSceneRoot();
		
		const auto& ref = sceneData->getEntityNodeData(referenceEntity);
		const bool canBeSibling = !ref.parentId.isEmpty() || isScene;
		const bool canBeChild = ref.data.getPrefab().isEmpty();
		if (!canBeChild && !canBeSibling) {
			return;
		}
		
		const bool addAsChild = (childOfReference && canBeChild) || !canBeSibling;
		const String& parentId = addAsChild ? referenceEntity : ref.parentId;
		const String& siblingId = addAsChild ? "" : referenceEntity;

		addEntity(parentId, siblingId, std::move(data));
	}
}

void SceneEditorWindow::addEntity(const String& parentId, const String& afterSibling, EntityData data)
{
	EntityData& parentData = sceneData->getEntityNodeData(parentId).data;
	if (parentData.getPrefab().isEmpty() && (parentId != "" || parentData.isSceneRoot())) {
		auto& seq = parentData.getChildren();
		const auto uuid = data.getInstanceUUID().toString();
		auto insertPos = std::find_if(seq.begin(), seq.end(), [&] (const EntityData& node) -> bool
		{
			return node.getInstanceUUID().toString() == afterSibling;
		});		
		if (insertPos != seq.end()) {
			++insertPos;
		}
		seq.insert(insertPos, std::move(data));
		onEntityAdded(uuid, parentId, afterSibling);
	}
}

void SceneEditorWindow::removeEntity()
{
	if (!currentEntityId.isEmpty()) {
		removeEntity(currentEntityId);
	}
}

void SceneEditorWindow::removeEntity(const String& targetId)
{
	const String& parentId = findParent(currentEntityId);

	auto& data = sceneData->getEntityNodeData(parentId).data;
	const bool isSceneRoot = parentId.isEmpty() && data.isSceneRoot();
	if (parentId.isEmpty() && !isSceneRoot) {
		// Don't delete root of prefab
		return;
	}

	auto& children = data.getChildren();
	children.erase(std::remove_if(children.begin(), children.end(), [&] (const EntityData& child)
	{
		return child.getInstanceUUID().toString() == targetId;
	}), children.end());

	onEntityRemoved(targetId, parentId);
}

String SceneEditorWindow::findParent(const String& entityId) const
{
	const auto tree = sceneData->getEntityTree();
	const auto res = findParent(entityId, tree, "");
	return res ? *res : "";
}

const String* SceneEditorWindow::findParent(const String& entityId, const EntityTree& tree, const String& prev) const
{
	if (tree.entityId == entityId) {
		return &prev;
	}

	for (auto& c: tree.children) {
		const auto res = findParent(entityId, c, tree.entityId);
		if (res) {
			return res;
		}
	}

	return nullptr;
}

void SceneEditorWindow::setCustomUI(std::shared_ptr<UIWidget> ui)
{
	if (curCustomUI) {
		curCustomUI->destroy();
	}
	curCustomUI = ui;
	
	auto customUIField = getWidget("customUI");
	customUIField->setShrinkOnLayout(true);
	customUIField->clear();
	if (ui) {
		customUIField->add(ui, 1);
	}
}

void SceneEditorWindow::setToolUI(std::shared_ptr<UIWidget> ui)
{
	if (curToolUI) {
		curToolUI->destroy();
	}
	curToolUI = ui;

	auto customUIField = canvas->getWidget("currentToolUI");
	customUIField->setShrinkOnLayout(true);
	customUIField->clear();
	if (ui) {
		customUIField->add(ui, 1);
	}
	customUIField->setActive(!!ui);
}

void SceneEditorWindow::decayTool()
{
	if (curTool == SceneEditorTool::Polygon) {
		setTool(SceneEditorTool::Translate);
	}
}

void SceneEditorWindow::setModified(bool enabled)
{
	auto button = getWidgetAs<UIButton>("saveButton");
	button->setLabel(LocalisedString::fromHardcodedString(enabled ? "* Save" : "Save"));
	button->setEnabled(enabled);
	modified = enabled;
}

bool SceneEditorWindow::isModified() const
{
	return modified;
}

String SceneEditorWindow::serializeEntity(const EntityData& node) const
{
	YAMLConvert::EmitOptions options;
	options.mapKeyOrder = {{ "name", "uuid", "components", "children" }};
	return YAMLConvert::generateYAML(node.toConfigNode(false), options);
}

std::optional<EntityData> SceneEditorWindow::deserializeEntity(const String& data) const
{
	ConfigFile file;
	try {
		YAMLConvert::parseConfig(file, gsl::as_bytes(gsl::span<const char>(data.c_str(), data.length())));
		if (!isValidEntityTree(file.getRoot())) {
			return {};
		}
		return EntityData(file.getRoot(), false);
	} catch (...) {
		return {};
	}
}

void SceneEditorWindow::assignUUIDs(EntityData& node)
{
	node.setInstanceUUID(UUID::generate());
	for (auto& child: node.getChildren()) {
		assignUUIDs(child);
	}
}

bool SceneEditorWindow::isValidEntityTree(const ConfigNode& node) const
{
	if (node.getType() != ConfigNodeType::Map) {
		return false;
	}
	for (const auto& [k, v]: node.asMap()) {
		if (k != "name" && k != "uuid" && k != "components" && k != "children" && k != "prefab") {
			return false;
		}
	}
	if (node.hasKey("children")) {
		for (const auto& child: node["children"].asSequence()) {
			if (!isValidEntityTree(child)) {
				return false;
			}
		}
	}
	
	return true;
}

void SceneEditorWindow::toggleConsole()
{
	auto console = getWidgetAs<UIDebugConsole>("debugConsole");
	const bool newState = !console->isActive();

	if (newState) {
		console->show();
	} else {
		console->hide();
	}
}

void SceneEditorWindow::setupConsoleCommands()
{
	auto controller = getWidgetAs<UIDebugConsole>("debugConsole")->getController();
	controller->clearCommands();
	gameBridge->setupConsoleCommands(*controller, *this);
}
