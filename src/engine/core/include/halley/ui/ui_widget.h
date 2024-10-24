#pragma once
#include "ui_sizer.h"
#include "ui_root.h"
#include "ui_painter.h"
#include "halley/maths/vector2.h"
#include "halley/maths/rect.h"
#include "halley/maths/vector4.h"
#include "ui_input.h"
#include "ui_data_bind.h"
#include "halley/api/audio_api.h"
#include "ui_style.h"
#include "halley/text/i18n.h"

namespace Halley {
	enum class MouseCursorMode;
	enum class JoystickType;
	class UIEvent;
	class UIValidator;
	class UIDataBind;
	class UIAnchor;
	class UIBehaviour;
	class UIEventHandler;
	class TextInputData;

	enum class UIWidgetUpdateType {
		First,
		Full,
		Partial
	};

	class UIWidget : public IUIElement, public UIParent, public IUISizer, public std::enable_shared_from_this<UIWidget> {
		friend class UIParent;
		friend class UIRoot;

	public:
		UIWidget(String id = "", Vector2f minSize = {}, std::optional<UISizer> sizer = {}, Vector4f innerBorder = {});
		virtual ~UIWidget();

		Vector2f getLayoutMinimumSize(bool force) const override;
		void setRect(Rect4f rect, IUIElementListener* listener) final override;
		virtual void onPreNotifySetRect(IUIElementListener& listener);

		UIRoot* getRoot() final override;
		const UIRoot* getRoot() const final override;
		UIParent* getParent() const;

		void layout(IUIElementListener* listener = nullptr);

		virtual void alignAt(const UIAnchor& anchor);
		void alignAtAnchor();
		virtual void setAnchor(UIAnchor anchor);
		const UIAnchor* getAnchor() const;
		void setAnchor();

		virtual std::optional<UISizer>& tryGetSizer();
		virtual const std::optional<UISizer>& tryGetSizer() const;
		virtual UISizer& getSizer();
		void setSizer(std::optional<UISizer> sizer);

		void add(std::shared_ptr<IUIElement> element, float proportion = 0, Vector4f border = Vector4f(), int fillFlags = UISizerFillFlags::Fill, size_t insertPos = std::numeric_limits<size_t>::max()) override;
		void addSpacer(float size) override;
		void addStretchSpacer(float proportion = 0) override;
		void remove(IUIElement& element) override;

		void clear() override;
		void clearChildren();

		virtual bool isFocusLocked() const;
		virtual bool isMouseOver() const;
		bool isFocused() const;
		void focus();

		void setInteractWithMouse(bool enabled);
		virtual bool canInteractWithMouse() const;
		virtual bool canChildrenInteractWithMouse() const;
		bool canPropagateMouseToChildren() const;
		void setPropagateMouseToChildren(bool enabled);
		virtual void notifyWidgetUnderMouse(const std::shared_ptr<UIWidget>& widget);

		void setId(const String& id);
		const String& getId() const final override;

		Vector2f getPosition(bool ignoreOffset = false) const;
		void setPositionOffset(Vector2f offset);
		virtual Vector2f getLayoutOriginPosition() const;
		virtual Vector2f getLayoutSize(Vector2f size) const;
		Vector2f getSize() const;
		virtual Vector2f getMinimumSize() const;
		Vector4f getInnerBorder() const;
		Rect4f getRect() const final override;
		virtual bool ignoreClip() const;

		void setPosition(Vector2f pos);
		void setBorder(Vector4f border);
		void setMinSize(Vector2f size);
		void setInnerBorder(Vector4f border);

		virtual bool isMouseInside(Vector2f mousePos) const;
		void setMouseOver(bool mouseOver);
		virtual std::optional<std::shared_ptr<UIWidget>> prePressMouse(Vector2f mousePos, int button, KeyMods keyMods);
		virtual void pressMouse(Vector2f mousePos, int button, KeyMods keyMods);
		virtual void releaseMouse(Vector2f mousePos, int button);
		virtual void onMouseOver(Vector2f mousePos);
		virtual void onMouseOver(Vector2f mousePos, KeyMods keyMods);
		virtual void onMouseLeft(Vector2f mousePos);
		virtual Rect4f getMouseRect() const;

		bool isActive() const final override;
		bool isActiveInHierarchy() const final override;
		void setActive(bool active);
		bool isEnabled() const;
		void setEnabled(bool enabled);
		bool isModal() const;
		void setModal(bool modal);
		bool isMouseBlocker() const;
		void setMouseBlocker(bool blocker);
		bool shrinksOnLayout() const;
		void setShrinkOnLayout(bool shrink);

		bool isAlive() const;
		void destroy();
		void forceDestroy();

		void setEventHandler(std::shared_ptr<UIEventHandler> handler);
		UIEventHandler& getEventHandler();
		void setHandle(UIEventType type, UIEventCallback handler);
		void setHandle(UIEventType type, String id, UIEventCallback handler);
		void clearHandle(UIEventType type);
		void clearHandle(UIEventType type, String id);

		void setCanSendEvents(bool canSend);

		virtual void setInputType(UIInputType uiInput);
		virtual void setJoystickType(JoystickType joystickType);
		void setOnlyEnabledWithInputs(const Vector<UIInputType>& inputs);
		const Vector<UIInputType>& getOnlyEnabledWithInput() const;
		virtual void setInputButtons(const UIInputButtons& buttons);
		UIInputType getLastInputType() const;

		void setValidator(std::shared_ptr<UIValidator> validator);
		std::shared_ptr<UIValidator> getValidator() const;
		virtual void onValidatorSet();

		UIDataBind::Format getDataBindFormat() const;
		void setDataBind(std::shared_ptr<UIDataBind> dataBind);
		std::shared_ptr<UIDataBind> getDataBind() const;
		virtual void readFromDataBind();
		void bindData(const String& childId, bool initialValue, UIDataBindBool::WriteCallback callback = {});
		void bindData(const String& childId, int initialValue, UIDataBindInt::WriteCallback callback = {});
		void bindData(const String& childId, float initialValue, UIDataBindFloat::WriteCallback callback = {});
		void bindData(const String& childId, const String& initialValue, UIDataBindString::WriteCallback callback = {});
		void bindData(const String& childId, ConfigNode initialValue, UIDataBindConfigNode::WriteCallback callback = {});

		bool isDescendentOf(const UIWidget& ancestor) const final override;

		std::shared_ptr<UIWidget> tryGetAncestorWidget(const String& id) override;
		template <typename T>
		std::shared_ptr<T> tryGetAncestorWidgetAs(const String& id)
		{
			return std::dynamic_pointer_cast<T>(tryGetAncestorWidget(id));
		}

		void setMouseClip(std::optional<Rect4f> mouseClip, bool force);

		virtual void onManualControlCycleValue(int delta);
		virtual void onManualControlAnalogueAdjustValue(float delta, Time t);
		virtual void onManualControlActivate();

		UIGamepadInput::Priority getInputPriority() const;

		void setChildLayerAdjustment(int delta);
		int getChildLayerAdjustment() const;
		void setNoClipChildren(bool noClip);
		bool getNoClipChildren() const;

		void sendEvent(UIEvent event, bool includeSelf = true) const override;
		void sendEventDown(const UIEvent& event, bool includeSelf = true) const;
		void forceAddChildren(UIInputType inputType, bool forceRecursive);

		void addBehaviour(std::shared_ptr<UIBehaviour> behaviour);
		void clearBehaviours();
		const Vector<std::shared_ptr<UIBehaviour>>& getBehaviours() const;

		std::optional<AudioHandle> playSound(const String& eventName);

		bool needsLayout() const;
		void markAsNeedingLayout() final override;

		virtual bool canReceiveFocus() const;
		virtual bool canReceiveMouseExclusive() const;
		std::shared_ptr<UIWidget> getFocusableOrAncestor();

		virtual void onAddedToRoot(UIRoot& root);
		virtual void onRemovedFromRoot(UIRoot& root);
		void onChildAdded(UIWidget& child) override;
		
		virtual void onMakeUI();

		virtual LocalisedString getToolTip() const;
		void setToolTip(LocalisedString toolTip);

		bool hasStyle() const;
		const Vector<UIStyle>& getStyles() const;

		virtual void setResultValue(ConfigNode data);
		virtual ConfigNode getResultValue();

		void fitToRoot();

		virtual std::optional<Vector2f> transformToChildSpace(Vector2f pos) const;
		virtual std::optional<MouseCursorMode> getMouseCursorMode() const;

		virtual void collectWidgetsForUpdating(Vector<std::shared_ptr<UIWidget>>& dst);
		virtual void collectWidgetsForRendering(size_t curRootIdx, Vector<std::pair<std::shared_ptr<UIWidget>, size_t>>& dst, Vector<std::shared_ptr<UIWidget>>& dstRoots);

	protected:
		virtual void draw(UIPainter& painter) const;
		virtual void drawAfterChildren(UIPainter& painter) const;
		virtual void drawChildren(UIPainter& painter) const;

		virtual bool hasRender() const;
		virtual void onPreRender();
		virtual void render(RenderContext& rc) const;
		virtual RenderContext getRenderContextForChildren(RenderContext& rc);

		virtual void update(Time t, bool moved);

		void updateBehaviours(Time t);

		virtual void onFocus(bool byClicking);
		virtual void onFocusLost();
		virtual TextInputData *getTextInputData();

		virtual void onLayout();
		virtual bool onDestroyRequested();
		virtual void onParentChanged();
		virtual void onActiveChanged(bool active);

		void notifyDataBind(bool data, bool force = false) const;
		void notifyDataBind(int data, bool force = false) const;
		void notifyDataBind(float data, bool force = false) const;
		void notifyDataBind(const String& data, bool force = false) const;
		void notifyDataBind(const ConfigNode& data, bool force = false) const;

		void shrink();
		void forceLayout();

		virtual void onGamepadInput(const UIInputResults& input, Time time);
		virtual void updateInputDevice(const InputDevice& inputDevice);
		virtual bool onKeyPress(KeyboardKeyPress key);
		void receiveKeyPress(KeyboardKeyPress key) override;
		
		virtual void onEnabledChanged();

		virtual void checkActive();

		void playStyleSound(const String& keyId);

		Vector<UIStyle> styles = {};

	private:
		void doDraw(UIPainter& painter) const;
		void doUpdate(UIWidgetUpdateType updateType, Time t, UIInputType inputType, JoystickType joystickType, Vector<std::shared_ptr<UIWidget>>& dst);
		void doPostUpdate();

		void setParent(UIParent* parent);
		void notifyTreeAddedToRoot(UIRoot& root);
		void notifyTreeRemovedFromRoot(UIRoot& root);

		void setWidgetRect(Rect4f rect);
		void resetInputResults();
		void updateActive(bool wasActiveBefore);
		void notifyActivationChange(bool active);

		void removeSizerDeadChildren();

		UIParent* parent = nullptr;
		UIRoot* root = nullptr;
		String id;

		Vector<UIInputType> onlyEnabledWithInputs;
		
		std::unique_ptr<UIInputButtons> gamepadInputButtons;
		UIInputResults gamepadInputResults;
	protected:
		UIInputType lastInputType = UIInputType::Undefined;
	private:
		mutable int layoutNeeded = 1;
		
		Vector2f position;
		Vector2f size;
		Vector2f minSize;
		Vector2f positionOffset;
		std::optional<Rect4f> mouseClip;

		Vector4f innerBorder;
		std::optional<UISizer> sizer;

		mutable Vector2f layoutSize;

		std::shared_ptr<UIEventHandler> eventHandler;
		std::shared_ptr<UIValidator> validator;
		std::shared_ptr<UIDataBind> dataBind;
		std::unique_ptr<UIAnchor> anchor;
		Vector<std::shared_ptr<UIBehaviour>> behaviours;

		std::unique_ptr<LocalisedString> toolTip;

		int childLayerAdjustment = 0;

		bool activeByUser = true;
		bool activeByInput = true;
		bool enabled = true;
		bool alive = true;
		bool focused = false;
		bool mouseOver = false;
		bool positionUpdated = false;
		bool modal = true;
		bool mouseBlocker = true;
		bool mouseInteraction = false;
		bool shrinkOnLayout = true;
		bool destroying = false;
		bool canSendEvents = true;
		bool dontClipChildren = false;
		bool propagateMouseToChildren = true;
	};

	template <typename F>
	void UIParent::descend(F f, bool includeInactive, bool includePending)
	{
		for (auto& c: children) {
			if (c->isActive() || includeInactive) {
				f(c);
				c->descend(f, includeInactive, includePending);
			}
		}

		if (includePending) {
			for (auto& c: childrenWaiting) {
				if (c->isActive() || includeInactive) {
					f(c);
					c->descend(f, includeInactive, includePending);
				}
			}
		}
	}
}
