//
// This file is part of the "DirectX12" project
// See "LICENSE" for license information.
//

#include "camera.h"

using namespace DirectX;

//----------------------------------------------------------------------------------------------------------------------

const auto kUp = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

//----------------------------------------------------------------------------------------------------------------------

Camera::Camera() {
    UpdateProjection();
    UpdateView();
}

//----------------------------------------------------------------------------------------------------------------------

void Camera::ZoomBy(float amount) {
    if (abs(amount) > FLT_EPSILON) {
        if (_mode == CameraMode::kArcball) {
            _radius = std::max(_radius - amount, 1.0f);
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
            _theta += XMConvertToRadians(delta.y);
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

void Camera::UpdateProjection() {
    XMStoreFloat4x4(&_projection, XMMatrixPerspectiveFovLH(_fov, _aspect_ratio, _near, _far));
}

//----------------------------------------------------------------------------------------------------------------------

void Camera::UpdateView() {
    if (_mode == CameraMode::kArcball) {
        auto position = XMVectorSet(_radius * cosf(_theta) * cosf(_phi),
                                    _radius * sinf(_theta),
                                    _radius * cosf(_theta) * sinf(_phi),
                                    1.0f);
        auto target = XMLoadFloat3(&_target);
        XMStoreFloat4x4(&_view, XMMatrixLookAtLH(position, target, kUp));
    }
}

//----------------------------------------------------------------------------------------------------------------------