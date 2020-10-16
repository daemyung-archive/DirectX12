//
// This file is part of the "DirectX12" project
// See "LICENSE" for license information.
//

#include "camera.h"

using namespace DirectX;

//----------------------------------------------------------------------------------------------------------------------

Camera::Camera() {
    UpdatePosition();
    UpdateView();
    UpdateProjection();
}

//----------------------------------------------------------------------------------------------------------------------

void Camera::ZoomBy(float amount) {
    if (abs(amount) > FLT_EPSILON) {
        if (_mode == CameraMode::kArcball) {
            _radius = std::max(_radius - amount, 1.0f);
            UpdatePosition();
            UpdateView();
        } else {
            assert(false);
        }
    }
}

//----------------------------------------------------------------------------------------------------------------------

void Camera::RotateBy(const DirectX::XMFLOAT2 &delta) {
    if (abs(delta.x) > FLT_EPSILON || abs(delta.y) > FLT_EPSILON) {
        if (_mode == CameraMode::kArcball) {
            _phi -= XMConvertToRadians(delta.x);
            _theta = std::clamp(_theta + XMConvertToRadians(delta.y), -XM_PIDIV2, XM_PIDIV2);
            UpdatePosition();
            UpdateView();
        } else {
            assert(false);
        }
    }
}

//----------------------------------------------------------------------------------------------------------------------

void Camera::SetAspectRatio(float aspect_ratio) {
    if (!XMScalarNearEqual(_aspect_ratio, aspect_ratio, FLT_EPSILON)) {
        _aspect_ratio = aspect_ratio;
        UpdateProjection();
    }
}

//----------------------------------------------------------------------------------------------------------------------

void Camera::SetRadius(float radius) {
    if (!XMScalarNearEqual(_radius, radius, FLT_EPSILON)) {
        _radius = radius;
        UpdatePosition();
        UpdateView();
    }
}

//----------------------------------------------------------------------------------------------------------------------

void Camera::UpdatePosition() {
    if (_mode == CameraMode::kArcball) {
        _position = {_radius * cosf(_theta) * cosf(_phi),
                     _radius * sinf(_theta),
                     _radius * cosf(_theta) * sinf(_phi)};


    } else {
        assert(false);
    }
}

//----------------------------------------------------------------------------------------------------------------------

void Camera::UpdateProjection() {
    XMStoreFloat4x4(&_projection, XMMatrixPerspectiveFovLH(_fov, _aspect_ratio, _near, _far));
}

//----------------------------------------------------------------------------------------------------------------------

void Camera::UpdateView() {
    auto position = XMLoadFloat3(&_position);
    auto target = XMLoadFloat3(&_target);
    XMStoreFloat3(&_forward, XMVectorSubtract(position, target));
    XMStoreFloat4x4(&_view, XMMatrixLookAtLH(position, target, kYAxisVector));
}

//----------------------------------------------------------------------------------------------------------------------