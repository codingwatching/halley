#include "base_canvas.h"

#include <utility>
using namespace Halley;

BaseCanvas::BaseCanvas(String id, UIStyle style, UISizer sizer, std::shared_ptr<InputKeyboard> keyboard)
	: UIClickable(std::move(id), {}, std::move(sizer))
	, keyboard(std::move(keyboard))
	, draggingButton({false, false})
{
	bg = style.getSprite("background");
	
	setHandle(UIEventType::MouseWheel, [this] (const UIEvent& event)
	{
		onMouseWheel(event);
	});
}

float BaseCanvas::getZoomLevel() const
{
	return std::pow(2.0f, float(zoomExp));
}

void BaseCanvas::setZoomListener(ZoomListener listener)
{
	zoomListener = listener;
}

void BaseCanvas::setMousePosListener(MousePosListener listener)
{
	mousePosListener = listener;
}

void BaseCanvas::setZoomEnabled(bool enabled)
{
	zoomEnabled = enabled;
}

void BaseCanvas::setScrollEnabled(bool enabled)
{
	scrollEnabled = enabled;
}

void BaseCanvas::setMouseMirror(std::shared_ptr<UIWidget> widget)
{
	mouseMirror = widget;
}

void BaseCanvas::doSetState(State state)
{
}

void BaseCanvas::update(Time t, bool moved)
{
	const Vector2f startPos = getScrollPosition();
	const auto scale = Vector2f(1, 1) / (Vector2f(32, 32) * getZoomLevel());

	bg
		.setPos(getPosition())
		.setSize(getSize())
		.setTexRect(Rect4f(startPos * scale, (startPos + getSize()) * scale));

	UIClickable::update(t, moved);
}

void BaseCanvas::draw(UIPainter& painter) const
{
	painter.draw(bg);
}

void BaseCanvas::pressMouse(Vector2f mousePos, int button, KeyMods keyMods)
{
	const bool needSpaceForLeftClick = false;
	const bool canLeftClickScroll = !needSpaceForLeftClick || keyboard && keyboard->isButtonDown(KeyCode::Space);
	if (button == 0 && canLeftClickScroll) {
		draggingButton[0] = true;
	}
	if (button == 1) {
		draggingButton[1] = true;
	}
	const bool shouldDrag = scrollEnabled && (draggingButton[0] || draggingButton[1]);

	if (shouldDrag && !dragging) {
		dragging = true;
		mouseStartPos = mousePos;
		startScrollPos = getScrollPosition();
	}

	UIClickable::pressMouse(mousePos, button, keyMods);

	if (mouseMirror) {
		mouseMirror->pressMouse(mousePos, button, keyMods);
	}
}

void BaseCanvas::releaseMouse(Vector2f mousePos, int button)
{
	if (button == 0 || button == 1) {
		draggingButton[button] = false;
	}
	const bool shouldDrag = scrollEnabled && (draggingButton[0] || draggingButton[1]);

	if (dragging && !shouldDrag) {
		onMouseOver(mousePos);
		dragging = false;
	}

	UIClickable::releaseMouse(mousePos, button);

	if (mouseMirror) {
		mouseMirror->releaseMouse(mousePos, button);
	}
}

void BaseCanvas::onMouseOver(Vector2f mousePos)
{
	lastMousePos = mousePos;
	if (dragging) {
		setScrollPosition(mouseStartPos - mousePos + startScrollPos);
	}

	if (mousePosListener) {
		mousePosListener(mousePos);
	}

	UIClickable::onMouseOver(mousePos);

	if (mouseMirror) {
		mouseMirror->onMouseOver(mousePos);
	}
}

void BaseCanvas::onDoubleClicked(Vector2f mousePos, KeyMods keyMods)
{
	sendEvent(UIEvent(UIEventType::ButtonDoubleClicked, getId()));
}

void BaseCanvas::refresh()
{
}

void BaseCanvas::onMouseWheel(const UIEvent& event)
{
	if (!zoomEnabled) {
		return;
	}
	
	const float oldZoom = getZoomLevel();
	zoomExp = clamp(zoomExp + signOf(event.getIntData()), -5, 5);
	const float zoom = getZoomLevel();

	if (zoom != oldZoom) {
		const Vector2f childPos = getChildren().at(0)->getPosition() - getPosition();

		const Vector2f panelScrollPos = getScrollPosition();

		if (zoomListener) {
			zoomListener(zoom);
		}

		refresh();

		const Vector2f relMousePos = lastMousePos - getBasePosition();
		const Vector2f oldMousePos = (relMousePos - childPos + panelScrollPos) / oldZoom;
		const Vector2f newScrollPos = oldMousePos * zoom - relMousePos;

		setScrollPosition(newScrollPos);
	}
}

