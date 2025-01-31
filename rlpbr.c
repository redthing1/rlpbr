// rlpbr (c) Nikolas Wipper 2021

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0, with exemptions for Ramon Santamaria and the
 * raylib contributors who may, at their discretion, instead license
 * any of the Covered Software under the zlib license. If a copy of
 * the MPL was not distributed with this file, You can obtain one
 * at https://mozilla.org/MPL/2.0/. */

#include "rlpbr.h"
#include "raymath.h"

#ifdef BUNDLE_SHADERS
#include "shaders.h"

#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

Shader pbr_shader;
Texture albedo, ao, metallic, normals, roughness;

typedef struct pbr_internal_light {
    float pos[3];
    float color[3];
    float target[3];
    float intensity;
    int type;
    int on;
    struct pbr_internal_light *next, *prev;
} pbr_internal_light;

pbr_internal_light *lights = NULL;
pbr_internal_light empty = {0};

void InitPBR() {
    #ifdef BUNDLE_SHADERS
    pbr_shader = LoadShaderFromMemory(pbr_vs, pbr_fs);
    #else
    pbr_shader = LoadShader("pbr/shader/pbr.vs", "pbr/shader/pbr.fs");
    #endif

    pbr_shader.locs[SHADER_LOC_MAP_ALBEDO] = GetShaderLocation(pbr_shader, "albedoMap");
    pbr_shader.locs[SHADER_LOC_MAP_NORMAL] = GetShaderLocation(pbr_shader, "normalMap");
    pbr_shader.locs[SHADER_LOC_MAP_METALNESS] = GetShaderLocation(pbr_shader, "metallicMap");
    pbr_shader.locs[SHADER_LOC_MAP_ROUGHNESS] = GetShaderLocation(pbr_shader, "roughnessMap");
    pbr_shader.locs[SHADER_LOC_MAP_OCCLUSION] = GetShaderLocation(pbr_shader, "aoMap");

    pbr_shader.locs[SHADER_LOC_MATRIX_VIEW] = GetShaderLocation(pbr_shader, "matView");
    pbr_shader.locs[SHADER_LOC_MATRIX_PROJECTION] = GetShaderLocation(pbr_shader, "matProjection");
    pbr_shader.locs[SHADER_LOC_MATRIX_MODEL] = GetShaderLocation(pbr_shader, "matModel");
    pbr_shader.locs[SHADER_LOC_MATRIX_MVP] = GetShaderLocation(pbr_shader, "mvp");
    pbr_shader.locs[SHADER_LOC_VECTOR_VIEW] = GetShaderLocation(pbr_shader, "camPos");

    albedo = LoadTextureFromImage(GenImageColor(1, 1, WHITE));
    ao = LoadTextureFromImage(GenImageColor(1, 1, WHITE));
    metallic = LoadTextureFromImage(GenImageColor(1, 1, BLACK));
    normals = LoadTextureFromImage(GenImageColor(1, 1, (Color) {128, 128, 255, 255}));
    roughness = LoadTextureFromImage(GenImageColor(1, 1, GRAY));
}

void ClosePBR() {
    UnloadShader(pbr_shader);
}

void UpdatePBR(Camera3D camera) {
    float cameraPos[3] = {camera.position.x, camera.position.y, camera.position.z};
    SetShaderValue(pbr_shader, pbr_shader.locs[SHADER_LOC_VECTOR_VIEW], cameraPos, SHADER_UNIFORM_VEC3);
}

Material LoadPBRMaterial(const char *albedo_path,
                         const char *ao_path,
                         const char *metallic_path,
                         const char *normals_path,
                         const char *roughness_path,
                         TextureFilter filter_mode) {
    Material mat = LoadMaterialDefault();
    mat.shader = pbr_shader;

    mat.maps[MATERIAL_MAP_ALBEDO].texture = albedo_path ? LoadTexture(albedo_path) : albedo;
    mat.maps[MATERIAL_MAP_OCCLUSION].texture = ao_path ? LoadTexture(ao_path) : ao;
    mat.maps[MATERIAL_MAP_METALNESS].texture = metallic_path ? LoadTexture(metallic_path) : metallic;
    mat.maps[MATERIAL_MAP_NORMAL].texture = normals_path ? LoadTexture(normals_path) : normals;
    mat.maps[MATERIAL_MAP_ROUGHNESS].texture = roughness_path ? LoadTexture(roughness_path) : roughness;

    SetTextureFilter(mat.maps[MATERIAL_MAP_ALBEDO].texture, filter_mode);
    SetTextureFilter(mat.maps[MATERIAL_MAP_NORMAL].texture, filter_mode);
    SetTextureFilter(mat.maps[MATERIAL_MAP_METALNESS].texture, filter_mode);
    SetTextureFilter(mat.maps[MATERIAL_MAP_ROUGHNESS].texture, filter_mode);
    SetTextureFilter(mat.maps[MATERIAL_MAP_OCCLUSION].texture, filter_mode);

    GenTextureMipmaps(&mat.maps[MATERIAL_MAP_ALBEDO].texture);
    GenTextureMipmaps(&mat.maps[MATERIAL_MAP_NORMAL].texture);
    GenTextureMipmaps(&mat.maps[MATERIAL_MAP_METALNESS].texture);
    GenTextureMipmaps(&mat.maps[MATERIAL_MAP_ROUGHNESS].texture);
    GenTextureMipmaps(&mat.maps[MATERIAL_MAP_OCCLUSION].texture);

    return mat;
}

void SetupPBRMaterial(Material* mat, TextureFilter filter_mode) {
    SetTextureFilter(mat->maps[MATERIAL_MAP_ALBEDO].texture, filter_mode);
    SetTextureFilter(mat->maps[MATERIAL_MAP_NORMAL].texture, filter_mode);
    SetTextureFilter(mat->maps[MATERIAL_MAP_METALNESS].texture, filter_mode);
    SetTextureFilter(mat->maps[MATERIAL_MAP_ROUGHNESS].texture, filter_mode);
    SetTextureFilter(mat->maps[MATERIAL_MAP_OCCLUSION].texture, filter_mode);

    GenTextureMipmaps(&mat->maps[MATERIAL_MAP_ALBEDO].texture);
    GenTextureMipmaps(&mat->maps[MATERIAL_MAP_NORMAL].texture);
    GenTextureMipmaps(&mat->maps[MATERIAL_MAP_METALNESS].texture);
    GenTextureMipmaps(&mat->maps[MATERIAL_MAP_ROUGHNESS].texture);
    GenTextureMipmaps(&mat->maps[MATERIAL_MAP_OCCLUSION].texture);
}

void MakeMaterialPBR(Material *mat) {
    mat->shader = pbr_shader;
}

void UpdateLightAt(pbr_internal_light *light, int index) {
    char loc_str[32];

    sprintf(loc_str, "lights[%i].pos", index);
    SetShaderValue(pbr_shader, GetShaderLocation(pbr_shader, loc_str), light->pos, SHADER_UNIFORM_VEC3);

    sprintf(loc_str, "lights[%i].color", index);
    SetShaderValue(pbr_shader, GetShaderLocation(pbr_shader, loc_str), light->color, SHADER_UNIFORM_VEC3);

    sprintf(loc_str, "lights[%i].target", index);
    SetShaderValue(pbr_shader, GetShaderLocation(pbr_shader, loc_str), light->target, SHADER_UNIFORM_VEC3);

    sprintf(loc_str, "lights[%i].intensity", index);
    SetShaderValue(pbr_shader, GetShaderLocation(pbr_shader, loc_str), &light->intensity, SHADER_UNIFORM_FLOAT);

    sprintf(loc_str, "lights[%i].type", index);
    SetShaderValue(pbr_shader, GetShaderLocation(pbr_shader, loc_str), &light->type, SHADER_UNIFORM_INT);

    sprintf(loc_str, "lights[%i].on", index);
    SetShaderValue(pbr_shader, GetShaderLocation(pbr_shader, loc_str), &light->on, SHADER_UNIFORM_INT);
}

void UpdateLight(pbr_internal_light *light) {
    pbr_internal_light *cur = lights;
    int i = 0;
    while (cur && cur != light) {
        cur = cur->next;
        i++;
    }
    if (cur) UpdateLightAt(cur, i);
}

void UpdateAllLights(bool clear_last) {
    pbr_internal_light *cur = lights;
    int i = 0;
    while (cur) {
        UpdateLightAt(cur, i);
        cur = cur->next;
        i++;
    }

    if (clear_last) {
        UpdateLightAt(&empty, i);
    }
}

void SetLightNoUpdate(void *_light, Light new) {
    pbr_internal_light *light = _light;

    light->pos[0] = new.pos.x;
    light->pos[1] = new.pos.y;
    light->pos[2] = new.pos.z;

    light->color[0] = (float) new.color.r / 255.f;
    light->color[1] = (float) new.color.g / 255.f;
    light->color[2] = (float) new.color.b / 255.f;

    if (new.type == SUN) new.target = Vector3Normalize(new.target);

    light->target[0] = new.target.x;
    light->target[1] = new.target.y;
    light->target[2] = new.target.z;

    light->intensity = new.intensity;
    light->type = new.type;
    light->on = new.on;
}

void *AddLight(Light light) {
    pbr_internal_light *cur = lights;
    int i = 0;

    if (!lights) {
        lights = RL_CALLOC(1, sizeof(pbr_internal_light));
        cur = lights;
    } else {
        while (cur->next) {
            cur = cur->next;
            i++;
        }

        i++;
        cur->next = RL_CALLOC(1, sizeof(pbr_internal_light));
        cur->next->prev = cur;
        cur = cur->next;
    }

    cur->pos[0] = light.pos.x;
    SetLightNoUpdate(cur, light);

    UpdateLightAt(cur, i);
    return cur;
}

void RemoveLight(void *_light) {
    pbr_internal_light *light = _light;
    light->prev->next = light->next;
    light->next->prev = light->prev;

    UpdateAllLights(true);
}

void SetLight(void *_light, Light new) {
    SetLightNoUpdate(_light, new);

    UpdateLight(_light);
}

Light GetLight(void *_light) {
    pbr_internal_light *light = _light;

    Light res;
    res.pos = (Vector3) {light->pos[0], light->pos[1], light->pos[2]};
    res.color = (Color) {
        (unsigned char) (light->color[0] * 255.f), (unsigned char) (light->color[1] * 255.f), (unsigned char) (light->color[2] * 255.f), 255
    };
    res.intensity = light->intensity;
    res.on = light->on;
    return res;
}

void SetOn(void *_light, int on) {
    pbr_internal_light *light = _light;
    light->on = on;
    UpdateLight(light);
}

void EnableLight(void *_light) {
    SetOn(_light, 1);
}

void DisableLight(void *_light) {
    SetOn(_light, 0);
}

void UnloadPBRModel(Model pbr) {
    // NOTE: Not unloading shader, because it's shared across models

    UnloadTexture(pbr.materials[0].maps[MATERIAL_MAP_ALBEDO].texture);
    UnloadTexture(pbr.materials[0].maps[MATERIAL_MAP_OCCLUSION].texture);
    UnloadTexture(pbr.materials[0].maps[MATERIAL_MAP_METALNESS].texture);
    UnloadTexture(pbr.materials[0].maps[MATERIAL_MAP_NORMAL].texture);
    UnloadTexture(pbr.materials[0].maps[MATERIAL_MAP_ROUGHNESS].texture);

    UnloadModel(pbr);
}
