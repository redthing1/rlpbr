// rlpbr (c) Nikolas Wipper 2021

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0, with exemptions for Ramon Santamaria and the
 * raylib contributors who may, at their discretion, instead license
 * any of the Covered Software under the zlib license. If a copy of
 * the MPL was not distributed with this file, You can obtain one
 * at https://mozilla.org/MPL/2.0/. */

#ifndef RLPBR_SRC_RLPBR_H_
#define RLPBR_SRC_RLPBR_H_

#include <raylib.h>


#ifndef GRAPHICS_API_OPENGL_33
#error OpenGL 3.3 is required for rlpbr to function properly
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef enum LightType {
    POINT = 1, SPOT = 2, SUN = 3
} LightType;

typedef struct Light {
    Vector3 pos;
    Vector3 target;
    Color color;
    float intensity;
    LightType type;
    int on;
} Light;

void InitPBR();
void ClosePBR();

void UpdatePBR(Camera3D camera);

/// Create PBR Material from several textures
Material LoadPBRMaterial(const char *albedo_path,
                         const char *ao_path,
                         const char *metallic_path,
                         const char *normals_path,
                         const char *roughness_path,
                         TextureFilter filter_mode);
void SetupPBRMaterial(Material* mat, TextureFilter filter_mode);

// Apply PBR shader to material without changing its textures
void MakeMaterialPBR(Material *mat);

void *AddLight(Light light);
void RemoveLight(void *light);

void SetLight(void *light, Light new_light);
Light GetLight(void *light);

void EnableLight(void *light);
void DisableLight(void *light);

void UnloadPBRModel(Model pbr);

#ifdef __cplusplus
}
#endif

#endif //RLPBR_SRC_RLPBR_H_
