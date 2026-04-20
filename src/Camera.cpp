#include "Camera.h"

void Camera::SetPosition(float x, float y) {
    cam_pos = { x, y };
}

float Camera::GetPositionX() { return cam_pos.x; }
float Camera::GetPositionY() { return cam_pos.y; }

void Camera::SetZoom(float new_zoom_factor) {
    zoom_factor = new_zoom_factor;
}

float Camera::GetZoom() { return zoom_factor; }