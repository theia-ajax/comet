import pygame
from theia.sim import SimRenderer

def main():
	pygame.init()
	fps = 60
	sim = SimRenderer(log_level=SimRenderer.LOG_INFO, fps=fps)
	screen = pygame.display.set_mode((1480, 320))
	clock = pygame.time.Clock()
	running = True

	while running:
		for event in pygame.event.get():
			if event.type == pygame.QUIT or (event.type == pygame.KEYDOWN and event.key == pygame.K_ESCAPE):
				running = False
		
		screen.fill('black')

		with sim.next_frame() as surf:
			scaled_surf = pygame.transform.scale(surf, (screen.get_width(), screen.get_height()))
			screen.blit(scaled_surf, (0, 0))

		pygame.display.flip()
		clock.tick(fps)
	
	pygame.quit()


if __name__ == "__main__":
	main()