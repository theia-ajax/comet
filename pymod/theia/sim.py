from ctypes import *
import pygame
from platform import system, machine

class _TheiaLibSurface(Structure):
	_fields_ = [("flags", c_uint32),
			 	("format", c_uint32),
			 	("w", c_int32),
			 	("h", c_int32),
			 	("pitch", c_uint64),
			 	("pixels", c_void_p),
			 	("refcount", c_int),
			 	("reserved", c_void_p),
				]

def _load_theia_sim_lib(platform):
	def _lib_path(lib, system, machine):
		path = './theia/{}/{}/{}'.format(system, machine, lib)
		print(path)
		return path

	index = platform.find('_')
	system = platform[:index]
	machine = platform[index+1:]

	libname = 'libtheia.so'
	if system == "Windows":
		libname = 'theia'
		WinDLL(_lib_path("libgcc_s_seh-1.dll", system, machine))
		WinDLL(_lib_path("SDL3.dll", system, machine))
		
	lib = CDLL(_lib_path(libname, system, machine))

	lib.Initialize.argtypes = [ c_int, c_char_p ]
	lib.StepSimulation.argtypes = [ c_float ]
	lib.RenderSimulationToFile.argtypes = [ c_char_p ]
	lib.RenderSimulationToSurface.restype = c_void_p
	lib.DestroyRenderedSurface.argtypes = [ c_void_p ]

	return lib

SIM_PLATFORM_LINUX_64 = "Linux_x86_64"
SIM_PLATFORM_LINUX_ARM = "Linux_aarch64"
SIM_PLATFORM_WINDOWS_64 = "Windows_AMD64"

class _SimLib:
	VALID_PLATFORMS = [
		SIM_PLATFORM_LINUX_64,
		SIM_PLATFORM_LINUX_ARM,
		SIM_PLATFORM_WINDOWS_64
	]
	
	DEFAULT_RENDER_DRIVERS = {
		SIM_PLATFORM_LINUX_64: "vulkan",
		SIM_PLATFORM_LINUX_ARM: "vulkan",
		SIM_PLATFORM_WINDOWS_64: "direct3d11"
	}

	def _resolve_platform(platform):
		if platform == None:
			platform = '{}_{}'.format(system(), machine())
		if not platform in _SimLib.VALID_PLATFORMS:
			raise ArgumentError('Invalid platform "{}"'.format(platform))
		return platform
	
	def _resolve_render_driver(self, render_driver):
		return render_driver if render_driver != None else _SimLib.DEFAULT_RENDER_DRIVERS[self.platform]
	
	def __init__(self, platform, log_level, render_driver):
		self.platform = _SimLib._resolve_platform(platform)
		render_driver = self._resolve_render_driver(render_driver)
		self.lib = _load_theia_sim_lib(self.platform)
		self.lib.Initialize(log_level, render_driver.encode('utf-8'))
	
	def __del__(self):
		self.lib.Shutdown()

	def step(self, dt):
		self.lib.StepSimulation(dt)
	
	def render(self):
		return _SimRenderSurface(self.lib, self.lib.RenderSimulationToSurface())

class _SimRenderSurface:
	def __init__(self, lib, surf):
		self.lib = lib
		self.surf_void_p = surf

	def __enter__(self):
		sim_surface = cast(self.surf_void_p, POINTER(_TheiaLibSurface)).contents
		sim_surface_pixels_type = c_uint32 * sim_surface.w * sim_surface.h
		sim_surface_carray = sim_surface_pixels_type.from_address(sim_surface.pixels)
		sim_surface_bytes = memoryview(sim_surface_carray)
		return pygame.image.frombuffer(sim_surface_bytes, (sim_surface.w, sim_surface.h), "BGRA")

	def __exit__(self, type, value, traceback):
		self.lib.DestroyRenderedSurface(self.surf_void_p)

class SimRenderer:
	(LOG_NONE,
	LOG_ERROR,
	LOG_WARNING,
	LOG_INFO,
	LOG_VERBOSE,
	LOG_DISABLED) = (0, 1, 2, 3, 4, 5)

	def __init__(self, platform=None, log_level=LOG_WARNING, render_driver=None, fps=60):
		self.fixed_delta_time = 1.0 / fps
		self.lib = _SimLib(platform, log_level, render_driver)

	def next_frame(self):
		self.lib.step(self.fixed_delta_time)
		return self.lib.render()