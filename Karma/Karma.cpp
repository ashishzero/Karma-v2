#include "kMath.h"
#include "kMedia.h"
#include "kImage.h"
#include "kRender.h"
#include "kStrings.h"
#include "kArray.h"
#include "kContext.h"
#include "kPlatform.h"
#include "kImGui.h"
#include "kScene.h"

static struct Camera3D
{
	kVec3 Position;
	kVec3 Forward;
} Camera;

static kArray<kMesh>    DragonMeshes;
static kArray<kTexture> DragonTextures;
static kArray<kVec4>    DragonColors;

void                    Tick(float t, float dt)
{}

void Update(float dt, float alpha)
{
	kArena *arena = kGetFrameArena();

	if (kKeyPressed(kKey::Escape))
	{
		kBreakLoop(0);
	}

	if (kKeyPressed(kKey::F11))
	{
		kToggleWindowFullscreen();
	}

	if (kKeyPressed(kKey::R) && (kGetKeyModFlags() & (kKeyMod_Shift | kKeyMod_Ctrl)))
	{
		kRestartLoop();
	}

	float sensitivity = 0.05f;

	if (kButtonPressed(kButton::Right))
	{
		kCaptureCursor();
	}

	if (kButtonReleased(kButton::Right))
	{
		kReleaseCursor();
	}

	if (kIsButtonDown(kButton::Right))
	{
		kVec2i  delta  = kGetCursorDelta();
		float   xaxis  = delta.x * sensitivity * dt;
		float   yaxis  = delta.y * sensitivity * dt;

		kVec3   right  = kNormalize(kCrossProduct(kVec3(0, 1, 0), Camera.Forward));
		kRotor3 turn1  = kAngleAxisToRotor3(kVec3(0, 1, 0), xaxis);
		kRotor3 turn2  = kAngleAxisToRotor3(right, -yaxis);

		Camera.Forward = kRotate(turn2 * turn1, Camera.Forward);
		Camera.Forward = kNormalize(Camera.Forward);
	}

	float forward   = (float)kIsKeyDown(kKey::W) - (float)kIsKeyDown(kKey::S);
	float right     = (float)kIsKeyDown(kKey::D) - (float)kIsKeyDown(kKey::A);

	kVec3 direction = forward * Camera.Forward + right * kNormalize(kCrossProduct(kVec3(0, 1, 0), Camera.Forward));

	float speed     = 20.0f;
	Camera.Position += dt * speed * kNormalizeZ(direction);

	kVec2i       size     = kGetWindowSize();
	float        ar       = kGetWindowAspectRatio();
	float        yfactor  = kGetWindowDpiScale();
	kViewport    viewport = {0, 0, size.x, size.y};

	kVec2i       pos      = kGetCursorPosition();
	float        sx       = ar * 50.0f * ((float)pos.x / size.x * 2.0f - 1.0f);
	float        sy       = 50.0f * ((float)(pos.y) / size.y * 2.0f - 1.0f);

	static float fov      = 0.2f;

	{
		kMat4        orientation = kLookAtDirection(Camera.Forward, kVec3(0, 1, 0));

		kMat4        tranform    = kTranslation(Camera.Position) * orientation;
		kCameraView  view        = kPerspectiveView(fov, ar, 0.1f, 1000.0f, tranform);

		kRotor3      rotor       = kAngleAxisToRotor3(kNormalize(kVec3(0, 1, 1)), 0.5);

		static float angle       = 0.05f;

		static float time        = 0.0f;

		float        rot         = kSin(time);

		kMat4        modal1      = kTranslation(0, 0, 10) * kRotationX(rot) * kRotationY(angle);
		kMat4        modal2      = kTranslation(0, 0, 20) * kRotationX(0.15f) * kRotationY(angle);
		kMat4        modal3      = kTranslation(0, 0, 30) * kRotationX(0.15f) * kRotationY(angle);

		float        red         = 5;
		// kAbsolute(kSin(time)) * 5.0f;
		time += dt * 0.1f;

		kRender3D::kBeginScene(view, viewport);

		kMat4 dragon = kTranslation(0, 0, 30) * kScale(kVec3(0.1f));

		for (imem iter = 0; iter < DragonMeshes.Count; ++iter)
		{
			kVec4 color = DragonColors[iter];
			color.xyz *= 5.0f;
			kRender3D::kDrawMesh(DragonMeshes[iter], DragonTextures[iter], dragon, color);
		}

		/*kRender3D::kDrawCube(modal1, kVec4(red, 3, 0, 1));
		kRender3D::kDrawCube(modal2, kVec4(3, red, 0, 1));
		kRender3D::kDrawCube(modal3, kVec4(0, 3, red, 1));*/
		kRender3D::kEndScene();

		angle += dt * 0.1f;
	}

	kCameraView view = kOrthographicView(ar, 100.0f);

	kRender2D::kLineThickness(0.1f);

	kRender2D::kBeginScene(view, viewport);
	kRender2D::kDrawCircle(kVec2(sx, sy), 15, kVec4(2, 20, 20, 1));

	kRender2D::kLineThickness(2);

	kRender2D::kDrawBezierCubic(kVec2(0), kVec2(20, -50), kVec2(50, -10), kVec2(80, -40), kVec4(3, 20, 3, 1));

	kRender2D::kEndScene();

	kCameraView  ui    = KOrthographicView(0, (float)size.x, 0, (float)size.y);

	static kVec4 color = kVec4(1);

	kRender2D::kBeginScene(ui, viewport);
	kRender2D::kDrawText("HDR Bloom", kVec2(50, 100), color, 8);
	kRender2D::kEndScene();

	kString FPS = kFormatString(arena, "FPS: %d (%.2fms)", (int)(1.0f / dt), 1000.0f * dt);

	kRender2D::kBeginRenderPass(kRenderPass::HUD);
	kRender2D::kBeginScene(ui, viewport);
	kRender2D::kPushOutLineStyle(kVec3(0.1f), 0.4f);
	kRender2D::kDrawText(FPS, kVec2(4), kVec4(1, 1, 1, 1), 0.5f * yfactor);
	kRender2D::kPopOutLineStyle();
	kRender2D::kEndScene();
	kRender2D::kEndRenderPass();

	//	 ImGui::Begin("Config");
	//
	////	 ImGui::DragFloat("FOV", &fov, 0.001f, 0.03f, 0.4f);
	//
	//	 ImGui::DragFloat("R", &color.x, 0.1f, 0.0f, 5000.0f);
	//	 ImGui::DragFloat("G", &color.y, 0.1f, 0.0f, 5000.0f);
	//	 ImGui::DragFloat("B", &color.z, 0.1f, 0.0f, 5000.0f);
	//
	//	 ImGui::End();
}

static float           g_Now;
static float           g_Accumulator;
static constexpr float FixedDeltaTime = 1.0f / 60.0f;

void                   OnLoadEvent(void *data)
{
	Camera.Position        = kVec3(0, 0, 0);
	Camera.Forward         = kVec3(0, 0, 1);

	const char   *dirpath  = "Resources/tarisland_-_dragon_high_poly/";
	const char   *filepath = "Resources/tarisland_-_dragon_high_poly/scene.gltf";

	cgltf_options options  = {};
	cgltf_data   *out      = 0;
	cgltf_result  res      = cgltf_parse_file(&options, filepath, &out);

	if (res == cgltf_result_success)
	{
		res = cgltf_load_buffers(&options, out, filepath);
		kAssert(res == cgltf_result_success);

		DragonMeshes.Resize(out->meshes_count);
		DragonTextures.Resize(out->meshes_count);
		DragonColors.Resize(out->meshes_count);

		kArray<kVertex3D> vertices;
		kArray<u32>       indices;

		for (umem mesh_iter = 0; mesh_iter < out->meshes_count; ++mesh_iter)
		{
			cgltf_mesh &mesh = out->meshes[mesh_iter];

			for (int primitive_iter = 0; primitive_iter < mesh.primitives_count; ++primitive_iter)
			{
				cgltf_primitive &primitive = mesh.primitives[primitive_iter];

				if (primitive.type != cgltf_primitive_type_triangles)
					continue;

				if (primitive.material)
				{
					if (primitive.material->has_pbr_specular_glossiness)
					{
						if (primitive.material->pbr_specular_glossiness.diffuse_texture.texture)
						{
							cgltf_texture *texture =
								primitive.material->pbr_specular_glossiness.diffuse_texture.texture;

							kString      name = kFormatString("%s%s", dirpath, texture->image->uri);

							kDefer
							{
								kFree(name.Items, name.Count);
							};

							kString      buff = kReadEntireFile(name);
							int          w, h, n;
							u8          *pixels       = kReadImage(buff, &w, &h, &n, 4);

							kTextureSpec spec         = {.Format = kFormat::RGBA8_UNORM_SRGB,
							                             .Width  = (u32)w,
							                             .Height = (u32)h,
							                             .Pitch  = (u32)w * 4,
							                             .Pixels = pixels,
							                             .Name   = name};

							DragonTextures[mesh_iter] = kCreateTexture(spec);

							kFree(buff.Items, buff.Count);
							kFreeImage(pixels, w, h, 4);
						}
						else
						{
							kAssert(false);
						}

						kVec4 color;
						color.x                 = primitive.material->pbr_specular_glossiness.diffuse_factor[0];
						color.y                 = primitive.material->pbr_specular_glossiness.diffuse_factor[1];
						color.z                 = primitive.material->pbr_specular_glossiness.diffuse_factor[2];
						color.w                 = primitive.material->pbr_specular_glossiness.diffuse_factor[3];

						DragonColors[mesh_iter] = color;
					}
					else
					{
						kAssert(false);
					}
				}

				if (primitive.indices)
				{
					cgltf_buffer_view *bv  = primitive.indices->buffer_view;
					void              *ptr = (u8 *)bv->buffer->data + bv->offset + primitive.indices->offset;

					indices.Resize(primitive.indices->count);

					if (primitive.indices->component_type == cgltf_component_type_r_16u)
					{
						u16 *src = (u16 *)ptr;
						for (umem iter = 0; iter < primitive.indices->count; ++iter)
						{
							indices[iter] = src[iter];
						}
					}
					else if (primitive.indices->component_type == cgltf_component_type_r_32u)
					{
						u32 *src = (u32 *)ptr;
						for (umem iter = 0; iter < primitive.indices->count; ++iter)
						{
							indices[iter] = src[iter];
						}
					}
				}

				bool color_attr = false;

				for (int attribute_iter = 0; attribute_iter < primitive.attributes_count; ++attribute_iter)
				{
					cgltf_attribute &attribute = primitive.attributes[attribute_iter];
					cgltf_accessor  *accessor  = attribute.data;
					void            *ptr =
						(u8 *)accessor->buffer_view->buffer->data + accessor->buffer_view->offset + accessor->offset;

					vertices.Resize(attribute.data->count);

					if (attribute.type == cgltf_attribute_type_position)
					{
						kAssert(accessor->component_type == cgltf_component_type_r_32f &&
						        accessor->type == cgltf_type_vec3);

						kVec3 *positions = (kVec3 *)ptr;

						for (umem iter = 0; iter < attribute.data->count; ++iter)
						{
							kVec3 pos               = positions[iter];
							vertices[iter].Position = kVec3(pos.x, -pos.y, -pos.z);
						}
					}
					else if (attribute.type == cgltf_attribute_type_normal)
					{
						kAssert(accessor->component_type == cgltf_component_type_r_32f &&
						        accessor->type == cgltf_type_vec3);

						kVec3 *normals = (kVec3 *)ptr;

						for (umem iter = 0; iter < attribute.data->count; ++iter)
						{
							kVec3 normal          = normals[iter];
							vertices[iter].Normal = kVec3(normal.x, -normal.y, -normal.z);
						}
					}
					else if (attribute.type == cgltf_attribute_type_color)
					{
						kAssert(accessor->component_type == cgltf_component_type_r_32f &&
						        accessor->type == cgltf_type_vec4);

						color_attr    = true;

						kVec4 *colors = (kVec4 *)ptr;

						for (umem iter = 0; iter < attribute.data->count; ++iter)
						{
							vertices[iter].Color = colors[iter];
						}
					}
					else if (attribute.type == cgltf_attribute_type_texcoord)
					{
						kAssert(accessor->component_type == cgltf_component_type_r_32f &&
						        accessor->type == cgltf_type_vec2);

						kVec2 *texcoords = (kVec2 *)ptr;

						for (umem iter = 0; iter < attribute.data->count; ++iter)
						{
							vertices[iter].TexCoord = texcoords[iter];
						}
					}
				}

				if (!color_attr)
				{
					for (kVertex3D &vtx : vertices)
					{
						vtx.Color = kVec4(1);
					}
				}
			}

			kMeshSpec spec          = {.Vertices = vertices, .Indices = indices, .Name = "Dragon"};
			DragonMeshes[mesh_iter] = kCreateMesh(spec);

			kFree(&vertices);
			kFree(&indices);
		}

		cgltf_free(out);
	}
}

void OnReleaseEvent(void *data)
{}

void OnUpdateEvent(float dt, void *data)
{
	dt = kMin(dt, 0.25f);

	g_Accumulator += dt;

	while (g_Accumulator >= FixedDeltaTime)
	{
		Tick(g_Now, FixedDeltaTime);
		g_Accumulator -= FixedDeltaTime;
		g_Now += FixedDeltaTime;
	}

	float alpha = g_Accumulator / FixedDeltaTime;

	Update(dt, alpha);
}

void Main(int argc, const char **argv)
{
	auto spec = kDefaultSpec;
	// spec.RenderPipeline.Hdr = kHighDynamicRange::Disabled;
	// spec.RenderPipeline.Bloom = kBloom::Disabled;

	kSetLogLevel(kLogLevel::Trace);
	kMediaUserEvents user = {.Update = OnUpdateEvent, .Load = OnLoadEvent, .Release = OnReleaseEvent};
	kEventLoop(spec, user);
}
