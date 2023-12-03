#include "kMath.h"
#include "kMedia.h"
#include "kImage.h"
#include "kRender.h"
#include "kStrings.h"
#include "kArray.h"
#include "kContext.h"
#include "kPlatform.h"
#include "kImGui.h"

struct WorldTime
{
	float Now;
	float Delta;
	float Accumulator;
};

struct World
{
	WorldTime Time;
};

struct Rect
{
	kVec2 Center;
	kVec2 Radius;
};

constexpr int   MaxIterations    = 16;
constexpr float MaxWorldSize     = 1000.0f;
constexpr float MinPenetration   = 1.0f / MaxWorldSize;
constexpr float MinDeltaTimeStep = 0.00001f;
constexpr int   MaxContacts      = 2048;

enum StateFlags
{
	State_IsColliding = 0x1
};

struct State
{
	kVec2 Position;
	kVec2 Velocity;

	kVec2 Force;

	float InvMass;
	kVec2 Gravity;

	uint  Flags;
};

void Integrate(State *states, int n, float t, float dt)
{
	for (int iter = 0; iter < n; ++iter)
	{
		State *s = &states[iter];
		s->Velocity += dt * (s->Gravity + s->InvMass * s->Force);
		s->Position += dt * s->Velocity;
		s->Flags = 0;
	}
}

enum class CollisionType
{
	None,
	Colliding,
	Penetrating,
};

struct Contact
{
	kVec2 Normal;
	// kVec2 Points[2];
	int   Ids[2];
	float Penetration;
	float Restitution;
};

struct ContactManifold
{
	int     Count;
	float   MinPenetration;
	float   MaxPenetration;
	Contact Contacts[MaxContacts];
};

CollisionType DetectCollision(State *states, Rect *rects, int n, ContactManifold *manifold)
{
	manifold->Count          = 0;
	manifold->MinPenetration = REAL_MAX;
	manifold->MaxPenetration = REAL_MIN;

	for (int iter_a = 0; iter_a < n; ++iter_a)
	{
		for (int iter_b = iter_a + 1; iter_b < n; ++iter_b)
		{
			Rect  &rect_a  = rects[iter_a];
			Rect  &rect_b  = rects[iter_b];

			State &state_a = states[iter_a];
			State &state_b = states[iter_b];

			kVec2  min_a   = state_a.Position + rect_a.Center - rect_a.Radius;
			kVec2  max_a   = state_a.Position + rect_a.Center + rect_a.Radius;

			kVec2  min_b   = state_b.Position + rect_b.Center - rect_b.Radius;
			kVec2  max_b   = state_b.Position + rect_b.Center + rect_b.Radius;

			kVec2  rmin    = min_b - rect_a.Radius;
			kVec2  rmax    = max_b + rect_a.Radius;

			kVec2  p       = state_a.Position + rect_a.Center;

			if (p.x >= rmin.x && p.y <= rmax.x && p.y >= rmin.y && p.y <= rmax.y)
			{
				float    penetration     = max_a.y - min_b.y;
				kVec2    normal          = kVec2(0, 1);

				Contact *r               = &manifold->Contacts[manifold->Count];
				r->Ids[0]                = iter_a;
				r->Ids[1]                = iter_b;
				r->Normal                = -normal;
				r->Penetration           = penetration;
				r->Restitution           = 0.4f;

				manifold->MinPenetration = kMin(manifold->MinPenetration, penetration);
				manifold->MaxPenetration = kMax(manifold->MaxPenetration, penetration);

				manifold->Count += 1;
				state_a.Flags |= State_IsColliding;
				state_b.Flags |= State_IsColliding;
			}
		}
	}

	if (manifold->MaxPenetration <= MinPenetration)
		return CollisionType::Colliding;
	return CollisionType::Penetrating;
}

void ResolveContacts(State *states, int n, float dt, ContactManifold &manifold)
{
	for (int iter = 0; iter < manifold.Count; ++iter)
	{
		Contact &contact             = manifold.Contacts[iter];

		int      i                   = contact.Ids[0];
		int      j                   = contact.Ids[1];

		kVec2    rvel                = states[j].Velocity - states[i].Velocity;
		float    separation          = kDotProduct(rvel, contact.Normal);

		float    impulse_denominator = states[i].InvMass + states[j].InvMass;
		if (impulse_denominator == 0.0f)
			continue;

		float bounce         = separation;

		kVec2 relative       = (states[j].Gravity - states[i].Gravity) * dt;
		float acc_separation = kDotProduct(relative, contact.Normal);

		if (acc_separation > 0)
		{
			bounce -= acc_separation;
			bounce = kMax(0.0f, bounce);
		}

		float impulse_numerator = -separation - bounce * contact.Restitution;

		float impulse           = impulse_numerator / impulse_denominator;

		states[i].Velocity -= impulse * states[i].InvMass * contact.Normal;
		states[j].Velocity += impulse * states[j].InvMass * contact.Normal;

		float penetration = contact.Penetration / impulse_denominator;
		states[i].Position += penetration * states[i].InvMass * contact.Normal;
		states[j].Position -= penetration * states[j].InvMass * contact.Normal;
	}
}

static kArray<State> g_Bodies;
static kArray<State> g_Temp;
static kArray<Rect>  g_Rects;
static int           g_GiveUpCounts = 0;

//
//
//

kTexture LoadTexture(kString filepath)
{
	kString content = kReadEntireFile(filepath);

	kImage  img;
	kReadImage(content, &img, 4);

	kTextureSpec spec = {.Format = kFormat_RGBA8_UNORM_SRGB,
	                     .Width  = (u32)img.Width,
	                     .Height = (u32)img.Height,
	                     .Pitch  = (u32)img.Width * img.Channels,
	                     .Pixels = img.Pixels,
	                     .Name   = filepath};

	kTexture     t    = kCreateTexture(spec);

	kFree(content.Items, content.Count + 1);
	kFreeImage(img);

	return t;
}

void Tick(World *world, float t, float dt)
{
	float current = 0.0f;
	float target  = dt;

	g_Temp.Reset();
	g_Temp.CopyArray(g_Bodies);

	ContactManifold manifold = {.Count = 0};

	for (int iter = 0; iter < MaxIterations; ++iter)
	{
		if ((dt - current) < MinDeltaTimeStep)
			break;

		float step = target - current;
		Integrate(g_Temp.Items, (int)g_Temp.Count, t + current, step);

		CollisionType ct      = DetectCollision(g_Temp.Items, g_Rects.Items, (int)g_Temp.Count, &manifold);

		bool          resolve = true;

		if (ct == CollisionType::Penetrating && manifold.MinPenetration > MinPenetration)
		{
			target = (target - current) * 0.5f;

			if ((target - current) > MinDeltaTimeStep)
			{
				g_Temp.Reset();
				g_Temp.CopyArray(g_Bodies);
				resolve = false;
			}
		}

		if (resolve)
		{
			if (ct != CollisionType::None)
			{
				ResolveContacts(g_Temp.Items, (int)g_Temp.Count, step, manifold);
			}
			current = target;
			target  = dt;
			g_Bodies.Reset();
			g_Bodies.CopyArray(g_Temp);
		}
	}

	float step = dt - current;
	if (step >= MinDeltaTimeStep)
	{
		// Give up
		g_GiveUpCounts += 1;
		Integrate(g_Temp.Items, (int)g_Temp.Count, t + current, step);
		CollisionType ct = DetectCollision(g_Temp.Items, g_Rects.Items, (int)g_Temp.Count, &manifold);
		if (ct != CollisionType::None)
		{
			ResolveContacts(g_Temp.Items, (int)g_Temp.Count, step, manifold);
		}
		g_Bodies.Reset();
		g_Bodies.CopyArray(g_Temp);
	}
}

void DrawBody(const State &state, const Rect &rect, kVec4 color, kVec4 outline)
{
	kVec2 pos = state.Position + rect.Center;
	kVec2 dim = rect.Radius + rect.Radius;
	kDrawRectCentered(pos, dim - kVec2(0.5f), color);
	kDrawRectCenteredOutline(pos, dim, outline);
}

void ResetWorld()
{
	g_Rects.Reset();
	g_Bodies.Reset();

	g_Rects.Resize(2);
	g_Bodies.Resize(2);

	g_Rects[0]           = {};
	g_Rects[1]           = {};

	g_Bodies[0]          = {};
	g_Bodies[1]          = {};

	g_Rects[0].Radius    = kVec2(75, 25);
	g_Rects[1].Radius    = kVec2(3);

	g_Bodies[1].InvMass  = 1.0f / 1.0f;
	g_Bodies[1].Gravity  = kVec2(0, -10);

	g_Bodies[0].Position = kVec2(0, -60);
}

void Update(World *world, float dt, float alpha)
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

	kVec2i      size     = kGetWindowSize();
	float       ar       = kGetWindowAspectRatio();
	float       yfactor  = kGetWindowDpiScale();
	kViewport   viewport = {0, 0, size.x, size.y};

	kCameraView view     = kOrthographicView(ar, 100.0f);

	kVec2i      pos      = kGetCursorPosition();
	float       sx       = ar * 50.0f * ((float)pos.x / size.x * 2.0f - 1.0f);
	float       sy       = 50.0f * ((float)(pos.y) / size.y * 2.0f - 1.0f);

	if (kButtonPressed(kButton::Left))
	{
		g_Bodies.Add(State{.Position = kVec2(sx, sy), .InvMass = 1, .Gravity = kVec2(0, -10)});
		g_Rects.Add(Rect{.Center = kVec2(0), .Radius = kVec2(3.0f)});
	}

	kLineThickness(0.5f);

	kBeginScene(view, viewport);

	for (int iter = 0; iter < (int)g_Bodies.Count; ++iter)
	{
		kVec4 outline = kVec4(1);

		if (g_Bodies[iter].Flags & State_IsColliding)
			outline = kVec4(0.7f, 0.1f, 0.1f, 1.0f);

		kVec4 color = kVec4(0.7f, 0.7f, 0.5f, 1.0f);

		if (g_Bodies[iter].InvMass == 0)
			color.xyz *= 0.2f;

		DrawBody(g_Bodies[iter], g_Rects[iter], color, outline);
	}

	kDrawCircle(kVec2(sx, sy), 1.0f, kVec4(1));

	kEndScene();

	kCameraView ui = KOrthographicView(0, (float)size.x, 0, (float)size.y);

	kBeginRenderPass(kRenderPass_HUD);
	kBeginScene(ui, viewport);
	kPushOutLineStyle(kVec3(0.1f), 0.4f);
	kString FPS = kFormatString(arena, "FPS: %d (%.2fms)", (int)(1.0f / dt), 1000.0f * dt);
	kDrawText(FPS, kVec2(4), kVec4(1, 1, 1, 1), 0.5f * yfactor);
	kPopOutLineStyle();
	kEndScene();
	kEndRenderPass();

	ImGui::Begin("State", 0, ImGuiWindowFlags_AlwaysAutoResize);
	if (ImGui::Button("Reset"))
		ResetWorld();

	ImGui::Text("Giveups: %d", g_GiveUpCounts);

	ImGui::End();
}

void OnLoadEvent(void *data)
{
	World *world            = (World *)data;
	world->Time.Accumulator = 0;
	world->Time.Delta       = 1.0f / 60.0f;
	world->Time.Now         = 0.0f;
	ResetWorld();
}

void OnReleaseEvent(void *data)
{
	World *world = (World *)data;
}

void OnUpdateEvent(float dt, void *data)
{
	dt               = kMin(dt, 0.25f);

	World     *world = (World *)data;
	WorldTime &time  = world->Time;
	time.Accumulator += dt;

	while (time.Accumulator >= time.Delta)
	{
		Tick(world, time.Now, time.Delta);
		time.Accumulator -= time.Delta;
		time.Now += time.Delta;
	}

	float alpha = time.Accumulator / time.Delta;

	Update(world, dt, alpha);
}

void Main(int argc, const char **argv)
{
	kSetLogLevel(kLogLevel::Trace);
	World           *world = (World *)kAlloc(sizeof(World));
	kMediaUserEvents user  = {.Data = world, .Update = OnUpdateEvent, .Load = OnLoadEvent, .Release = OnReleaseEvent};
	kEventLoop(kDefaultSpec, user);
}
