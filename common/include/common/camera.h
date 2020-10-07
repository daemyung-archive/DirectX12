//
// This file is part of the "DirectX12" project
// See "LICENSE" for license information.
//

#ifndef CAMERA_H_
#define CAMERA_H_

#include "utility.h"

//----------------------------------------------------------------------------------------------------------------------

enum class CameraMode {
    kArcball
};

//----------------------------------------------------------------------------------------------------------------------

class Camera {
public:
    //! Constructor.
    Camera();

    //! Zoom the camera given the amount.
    //! \param amount The amount how much zoom in or out.
    void ZoomBy(float amount);

    void RotateBy(const DirectX::XMFLOAT2 &delta);

    //! Set the aspect ratio.
    //! \param aspect_ratio The aspect ratio.
    void SetAspectRatio(float aspect_ratio);

    //! Retrieve a projection matrix.
    //! \return A projection matrix.
    [[nodiscard]]
    inline auto GetProjection() const {
        return _projection;
    }

    //! Retrieve a view matrix.
    //! \return A view matrix.
    [[nodiscard]]
    inline auto GetView() const {
        return _view;
    }

private:
    //! Update a projection matrix.
    void UpdateProjection();

    //! Update a view matrix.
    void UpdateView();

private:
    CameraMode _mode = CameraMode::kArcball;
    float _fov = DirectX::XMConvertToRadians(60.0f);
    float _aspect_ratio = 1.0f;
    float _near = 0.001f;
    float _far = 1000.0f;
    float _radius = 5.0f;
    float _phi = DirectX::XM_PI + DirectX::XM_PIDIV2;
    float _theta = 0.0f;
    DirectX::XMFLOAT3 _target = {0.0f, 0.0f, 0.0f};
    DirectX::XMFLOAT4X4 _projection = kIdentityFloat4x4;
    DirectX::XMFLOAT4X4 _view = kIdentityFloat4x4;

};

//----------------------------------------------------------------------------------------------------------------------

#endif