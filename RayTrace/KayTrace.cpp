#include "kMath.h"
#include "kMedia.h"
#include "kImage.h"
#include "kRender.h"
#include "kStrings.h"
#include "kArray.h"
#include "kContext.h"
#include "kPlatform.h"
#include "kImGui.h"
#include "kPlatform.h"

//
//
//

struct kRay
{
	kVec3 Origin;
	kVec3 Direction;

	kRay()
	{}
	kRay(kVec3 origin, kVec3 dir) : Origin(origin), Direction(dir)
	{}
};

enum kHitRecordFlags
{
	kHitRecord_FrontFace = 0x1
};

struct kHitRecord
{
	kVec3 Position;
	kVec3 Normal;
	float Distance;
	uint  Flags;
};

struct kSphere
{
	kVec3 Center;
	float Radius;

	kSphere()
	{}
	kSphere(kVec3 center, float radius) : Center(center), Radius(radius)
	{}
};

kVec3 kProject(const kRay &ray, float t)
{
	return ray.Origin + t * ray.Direction;
}

void HitCalculateFrontFace(const kRay &ray, kHitRecord *record)
{
	bool front = kDotProduct(ray.Direction, record->Normal) < 0.0f;
	record->Flags |= front ? kHitRecord_FrontFace : 0;
}

bool RayHitSphere(const kSphere &sphere, const kRay &ray, kRange trange, kHitRecord *record)
{
	kVec3 p            = ray.Origin - sphere.Center;
	float a            = kLengthSq(ray.Direction);
	float hb           = kDotProduct(p, ray.Direction);
	float c            = kLengthSq(p) - sphere.Radius * sphere.Radius;

	float discriminant = hb * hb - a * c;
	if (discriminant < 0.0f)
		return false;

	float sqrtd = kSquareRoot(discriminant);
	float t     = (-hb - sqrtd) / a;

	if (!kRangeSurrounds(trange, t))
	{
		t = (-hb + sqrtd) / a;
		if (!kRangeSurrounds(trange, t))
			return false;
	}

	record->Distance = t;
	record->Position = kProject(ray, t);
	record->Normal   = (record->Position - sphere.Center) / sphere.Radius;
	record->Flags    = 0;
	HitCalculateFrontFace(ray, record);
	return true;
}

kVec3 RandomVec3()
{
	float x = kRandomFloat();
	float y = kRandomFloat();
	float z = kRandomFloat();
	return kVec3(x, y, z);
}

kVec3 RandomVec3(float min, float max)
{
	float x = kRandomFloatRange(min, max);
	float y = kRandomFloatRange(min, max);
	float z = kRandomFloatRange(min, max);
	return kVec3(x, y, z);
}

kVec3 RandomUnitSphere()
{
	while (true)
	{
		kVec3 p = RandomVec3();
		if (kLengthSq(p) < 1)
			return p;
	}
	return kVec3(0);
}

kVec3 RandomUnitVector()
{
	kVec3 p = RandomUnitSphere();
	return kNormalizeZ(p);
}

kVec3 RandomHemisphere(kVec3 normal)
{
	kVec3 dir = RandomUnitVector();
	if (kDotProduct(dir, normal) > 0.0f)
		return dir;
	else
		return -dir;
}

kVec3 Reflect(kVec3 v, kVec3 n)
{
	return v - 2 * kDotProduct(v, n) * n;
}

//
//
//

enum class MaterialType
{
	Lambertian,
	Metal,
};

struct Material
{
	MaterialType Type;
	kVec4        Color;
};

static kArray<kSphere>  Spheres;
static kArray<Material> Materials;

//
//
//

bool Scatter(const Material &material, const kRay &ray, const kHitRecord &record, kVec4 *color, kRay *scattered)
{
	if (material.Type == MaterialType::Lambertian)
	{
		scattered->Direction = record.Normal + RandomUnitVector();
		if (kIsNull(scattered->Direction))
			scattered->Direction = record.Normal;
		scattered->Origin = record.Position;
		*color            = material.Color;
		return true;
	}
	else if (material.Type == MaterialType::Metal)
	{
		kVec3 reflected      = Reflect(kNormalize(ray.Direction), record.Normal);
		scattered->Direction = kNormalize(reflected);
		scattered->Origin    = record.Position;
		*color               = material.Color;
		return true;
	}
	return false;
}

kVec4 RayColor(const kRay &ray, int depth)
{
	if (depth <= 0)
		return kVec4(0);

	kRange     time     = kRange(0.001f, REAL32_MAX);

	kHitRecord record   = {};
	bool       hit      = false;
	imem       material = -1;

	for (imem iter = 0; iter < Spheres.Count; ++iter)
	{
		kHitRecord hr = {};
		if (RayHitSphere(Spheres[iter], ray, time, &hr))
		{
			material = iter;
			hit      = true;
			record   = hr;
			time.Max = hr.Distance;
		}
	}

	if (hit)
	{
		kRay  scattered;
		kVec4 color;

		if (Scatter(Materials[material], ray, record, &color, &scattered))
		{
			return color * RayColor(scattered, depth - 1);
		}
		return kVec4(0);
	}

	float alpha = 0.5f * (ray.Direction.y + 1.0f);
	kVec4 color = kLerp(kVec4(1), kVec4(0.5f, 0.7f, 1.0f, 1.0f), alpha);
	return color;
}

//
//
//

static kImage Output;

void          SetPixel(int y, int x, kVec4 color)
{
	int  index    = y * Output.Width + x;
	u32 *pixels   = (u32 *)Output.Pixels;
	pixels[index] = kColor4ToUint(color);
}

void OnLoadEvent(void *data)
{
	float aspect_ratio = 16.0f / 9.0f;
	int   height       = 512;
	int   width        = (int)(height * aspect_ratio);
	Output             = kAllocImage(width, height, 4);

	Materials.Add(Material{.Type = MaterialType::Lambertian, .Color = kVec4(0.8f, 0.8f, 0.0f, 1.0f)});
	Materials.Add(Material{.Type = MaterialType::Lambertian, .Color = kVec4(0.7f, 0.3f, 0.3f, 1.0f)});
	Materials.Add(Material{.Type = MaterialType::Metal, .Color = kVec4(0.8f, 0.8f, 0.8f, 1.0f)});
	Materials.Add(Material{.Type = MaterialType::Metal, .Color = kVec4(0.8f, 0.6f, 0.2f, 1.0f)});

	Spheres.Add(kSphere(kVec3(0, -100.5f, -1), 100));
	Spheres.Add(kSphere(kVec3(0, 0, -1), 0.5f));
	Spheres.Add(kSphere(kVec3(-1, 0, -1), 0.5f));
	Spheres.Add(kSphere(kVec3(1, 0, -1), 0.5f));
}

static bool  IsRendering      = false;
static float ProgressFraction = 0.0f;

void Render()
{
	float focal_length  = 1.0f;
	float vp_height     = 2.0f;
	float vp_width      = vp_height * ((float)Output.Width / (float)Output.Height);

	kVec3 camera_center = kVec3(0);

	kVec3 vp_u          = kVec3(vp_width, 0, 0);
	kVec3 vp_v          = kVec3(0, -vp_height, 0);

	kVec3 pixel_delta_u = vp_u / (float)Output.Width;
	kVec3 pixel_delta_v = vp_v / (float)Output.Height;

	kVec3 vp_upper_left = camera_center - kVec3(0, 0, focal_length) - vp_u / 2.0f - vp_v / 2.0f;

	kVec3 pixel00_loc   = vp_upper_left + 0.5f * (pixel_delta_u + pixel_delta_v);

	int   sample_count  = 100;
	int   max_depth     = 50;

	for (int y = 0; y < Output.Height; ++y)
	{
		for (int x = 0; x < Output.Width; ++x)
		{
			kVec3 pixel_center = pixel00_loc + ((float)x * pixel_delta_u) + ((float)y * pixel_delta_v);

			kVec4 color        = kVec4(0);
			for (int sample = 0; sample < sample_count; ++sample)
			{
				float offset_u      = -0.5f + kRandomFloat01();
				float offset_v      = -0.5f + kRandomFloat01();
				kVec3 sample_offset = offset_u * pixel_delta_u + offset_v * pixel_delta_v;
				kVec3 pixel_sample  = pixel_center + sample_offset;
				kVec3 ray_direction = kNormalize(pixel_sample - camera_center);
				kRay  ray           = kRay(camera_center, ray_direction);
				color += RayColor(ray, max_depth);
			}

			color /= (float)sample_count;

			SetPixel(y, x, kLinearToSrgb(color));

			ProgressFraction = (float)(x + y * Output.Width) / (Output.Width * Output.Height);
		}
	}
}

void OnReleaseEvent(void *data)
{}

void WriteImageToFile(void *ctx, void *data, int size)
{
	kFile fp = *(kFile *)ctx;
	kWriteFile(fp, (u8 *)data, size);
}

//
//
//

int RayTraceThread(void *arg)
{
	Render();
	kFile file = kOpenFile("Output.png", kFileAccess::Write, kFileShareMode::Exclusive, kFileMethod::CreateAlways);
	if (!kWriteImage(Output, kImageFileFormat::PNG, WriteImageToFile, &file))
	{
		kLogError("Failed to write output image");
	}
	kCloseFile(file);
	IsRendering = false;
	return 0;
}

void OnUpdateEvent(float dt, void *data)
{
	ImGui::Begin("Options", 0, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize);
	if (IsRendering)
	{
		ImGui::BeginDisabled();
		ImGui::Button("Save Image");
		ImGui::EndDisabled();
	}
	else
	{
		if (ImGui::Button("Save Image"))
		{
			IsRendering = true;
			kLaunchThread(RayTraceThread, 0, kThreadAttribute_Games);
		}
	}
	ImGui::Text("Render Progress");
	ImGui::ProgressBar(ProgressFraction);
	ImGui::End();
}

void Main(int argc, const char **argv)
{
	kSetLogLevel(kLogLevel::Trace);
	kMediaUserEvents user = {.Update = OnUpdateEvent, .Load = OnLoadEvent, .Release = OnReleaseEvent};

	kMediaSpec       spec = kDefaultSpec;
	spec.Window.Width     = 800;
	spec.Window.Height    = 600;

	kEventLoop(spec, user);
}
