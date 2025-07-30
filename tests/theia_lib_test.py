
# class Vec2(Structure):
# 	_fields_ = [("x", c_float),
# 			 	("y", c_float)]
	
# class Vec3(Structure):
# 	_fields_ = [("x", c_float),
# 			 	("y", c_float),
# 				("z", c_float)]
	
# class Vec4(Structure):
# 	_fields_ = [("x", c_float),
# 			 	("y", c_float),
# 			 	("z", c_float),
# 				("w", c_float)]
	

# # typedef struct PhysicsConfig {
# # 	Vec2 Gravity;
# # 	Vec2 HeatForce;
# # 	float32 HeatTransferRate;
# # 	float32 SurfaceTensionScalar;
# # 	float32 SurfaceTensionExtraRadius;
# # 	Vec4 Bounds;
# # 	float32 CellSize;
# # 	int32 MaxPhysicsObjects;
# # 	float32 HeatDecay;
# # 	float32 HeaterHeatDelta;
# # 	float32 CoolerHeatDelta;
# # 	float32 HeaterZoneSize;
# # 	float32 CoolerZoneSize;
# # 	float32 SquishZoneSize;
# # 	float32 SquishZoneForceMin;
# # 	float32 SquishZoneForceMax;
# # } PhysicsConfig;

# class PhysicsConfig(Structure):
# 	_fields_ = [("gravity", Vec2),
# 			 	("heat_force", Vec2),
# 				("heat_transfer_rate", c_float),
# 				("surface_tension_scalar", c_float),
# 				("surface_tension_extra_radius", c_float),
# 				("bounds", Vec4),
# 				("cell_size", c_float),
# 				("max_physics_objects", c_int32),
# 				("heat_decay", c_float),
# 				("heater_heat_delta", c_float),
# 				("cooler_heat_delta", c_float),
# 				("heater_zone_size", c_float),
# 				("cooler_zone_size", c_float),
# 				("squish_zone_size", c_float),
# 				("squish_zone_force_min", c_float),
# 				("squish_zone_force_max", c_float)]

from ctypes import *
import pygame

class TheiaSurface(Structure):
	_fields_ = [("flags", c_uint32),
			 	("format", c_uint32),
			 	("w", c_int32),
			 	("h", c_int32),
			 	("pitch", c_uint64),
			 	("pixels", c_void_p),
			 	("refcount", c_int),
			 	("reserved", c_void_p),
				]

frames_per_second = 60
fixed_delta_time = 1.0 / frames_per_second

theia = CDLL("bin/theia/bin/linux64/release/libtheia.so")

(LogLevelNone,
 LogLevelError,
 LogLevelWarning,
 LogLevelInfo,
 LogLevelVerbose,
 LogLevelDisabled,
 LogLevelCount) = (0, 1, 2, 3, 4, 5, 6)

theia.Initialize.argtypes = [ c_int, c_char_p ]
theia.StepSimulation.argtypes = [ c_float ]
theia.RenderSimulationToFile.argtypes = [ c_char_p ]
theia.RenderSimulationToSurface.restype = c_void_p
theia.DestroyRenderedSurface.argtypes = [ c_void_p ]

theia.Initialize(LogLevelWarning, "vulkan".encode('utf-8'))

pygame.init()
screen = pygame.display.set_mode((1480, 320))
render_target = pygame.Surface(
	(screen.get_width(), screen.get_height()),
	depth=32)
clock = pygame.time.Clock()
running = True
sim_frame = 0

while running:
	for event in pygame.event.get():
		if event.type == pygame.QUIT or (event.type == pygame.KEYDOWN and event.key == pygame.K_ESCAPE):
			running = False
	
	screen.fill('black')

	theia.StepSimulation(fixed_delta_time)

	sim_surface_void_p = theia.RenderSimulationToSurface()
	sim_surface_p = cast(sim_surface_void_p, POINTER(TheiaSurface))
	sim_surface = sim_surface_p.contents
	
	sim_surface_pixels_type = c_uint32 * sim_surface.w * sim_surface.h
	sim_surface_carray = sim_surface_pixels_type.from_address(sim_surface.pixels)
	sim_surface_bytes = bytes(sim_surface_carray)

	render_target_buffer = render_target.get_buffer()
	render_target_buffer.write(sim_surface_bytes)
	del render_target_buffer

	theia.DestroyRenderedSurface(sim_surface_void_p)

	screen.blit(render_target, (0, 0))

	pygame.display.flip()
	clock.tick(frames_per_second)
	sim_frame += 1

pygame.quit()

theia.Shutdown()
