from ctypes import *

theialib = CDLL("bin/theia/bin/linux64/debug/libtheia.so")

class Vec2(Structure):
	_fields_ = [("x", c_float),
			 	("y", c_float)]
	
class Vec3(Structure):
	_fields_ = [("x", c_float),
			 	("y", c_float),
				("z", c_float)]
	
class Vec4(Structure):
	_fields_ = [("x", c_float),
			 	("y", c_float),
			 	("z", c_float),
				("w", c_float)]
	

# typedef struct PhysicsConfig {
# 	Vec2 Gravity;
# 	Vec2 HeatForce;
# 	float32 HeatTransferRate;
# 	float32 SurfaceTensionScalar;
# 	float32 SurfaceTensionExtraRadius;
# 	Vec4 Bounds;
# 	float32 CellSize;
# 	int32 MaxPhysicsObjects;
# 	float32 HeatDecay;
# 	float32 HeaterHeatDelta;
# 	float32 CoolerHeatDelta;
# 	float32 HeaterZoneSize;
# 	float32 CoolerZoneSize;
# 	float32 SquishZoneSize;
# 	float32 SquishZoneForceMin;
# 	float32 SquishZoneForceMax;
# } PhysicsConfig;

class PhysicsConfig(Structure):
	_fields_ = [("gravity", Vec2),
			 	("heat_force", Vec2),
				("heat_transfer_rate", c_float),
				("surface_tension_scalar", c_float),
				("surface_tension_extra_radius", c_float),
				("bounds", Vec4),
				("cell_size", c_float),
				("max_physics_objects", c_int32),
				("heat_decay", c_float),
				("heater_heat_delta", c_float),
				("cooler_heat_delta", c_float),
				("heater_zone_size", c_float),
				("cooler_zone_size", c_float),
				("squish_zone_size", c_float),
				("squish_zone_force_min", c_float),
				("squish_zone_force_max", c_float)]

theialib.Initialize()

theialib.PhysicsDefaultConfig.restype = PhysicsConfig
Config = theialib.PhysicsDefaultConfig()
theialib.PhysicsInitialize(byref(Config))

theialib.Shutdown()