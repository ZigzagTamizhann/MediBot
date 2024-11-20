import pygame
import time

# Initialize Pygame
pygame.init()

# Constants
WINDOW_WIDTH = 800
WINDOW_HEIGHT = 800
CAR_WIDTH = 50
CAR_HEIGHT = 30
CAR_SPEED = 5
OBSTACLE_SIZE = 50
WAIT_TIME = 5

# Colors
BLACK = (0, 0, 0)
WHITE = (255, 255, 255)
RED = (255, 0, 0)
GREEN = (0, 255, 0)
BLUE = (0, 0, 255)

# Setup display
window = pygame.display.set_mode((WINDOW_WIDTH, WINDOW_HEIGHT))
pygame.display.set_caption('Obstacle Avoiding Car Simulation')

# Load car image
car_image = pygame.Surface((CAR_WIDTH, CAR_HEIGHT))
car_image.fill(GREEN)

# Function to draw obstacles
def draw_obstacles(obstacles):
    font = pygame.font.Font(None, 36)
    for i, (x, y) in enumerate(obstacles):
        pygame.draw.rect(window, RED, pygame.Rect(x, y, OBSTACLE_SIZE, OBSTACLE_SIZE))
        label = font.render(str(i + 1), True, WHITE)
        window.blit(label, (x + OBSTACLE_SIZE // 2 - label.get_width() // 2, y + OBSTACLE_SIZE // 2 - label.get_height() // 2))

# Check if car is within the boundaries
def is_within_boundaries(x, y):
    return 0 <= x <= WINDOW_WIDTH - CAR_WIDTH and 0 <= y <= WINDOW_HEIGHT - CAR_HEIGHT

# Check if car collides with any obstacles
def check_collision(x, y, obstacles):
    car_rect = pygame.Rect(x, y, CAR_WIDTH, CAR_HEIGHT)
    for (ox, oy) in obstacles:
        obstacle_rect = pygame.Rect(ox, oy, OBSTACLE_SIZE, OBSTACLE_SIZE)
        if car_rect.colliderect(obstacle_rect):
            return True
    return False

# Function for automatic mode
def automatic_mode():
    clock = pygame.time.Clock()

    # Initialize car position
    car_x = WINDOW_WIDTH // 2 - CAR_WIDTH // 2
    car_y = WINDOW_HEIGHT - CAR_HEIGHT - 10

    # Define obstacle positions on the left and right sides
    obstacles = [(0, i * (WINDOW_HEIGHT // 5)) for i in range(5)] + \
                [(WINDOW_WIDTH - OBSTACLE_SIZE, (4 - i) * (WINDOW_HEIGHT // 5)) for i in range(5)]

    # Define stopping positions near the red boxes
    waypoints = [(CAR_WIDTH, i * (WINDOW_HEIGHT // 5) + OBSTACLE_SIZE // 2) for i in range(5)] + \
                [(WINDOW_WIDTH - OBSTACLE_SIZE - CAR_WIDTH, (4 - i) * (WINDOW_HEIGHT // 5) + OBSTACLE_SIZE // 2) for i in range(5)]

    waypoint_index = 0
    reached_waypoint = False
    visited_boxes = set()

    running = True
    while running:
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                running = False
            if event.type == pygame.KEYDOWN:
                if event.key == pygame.K_q:  # Check if 'Q' key is pressed
                    running = False

        # Move car towards the current waypoint
        if not reached_waypoint:
            target_x, target_y = waypoints[waypoint_index]
            if car_x < target_x:
                car_x += CAR_SPEED
            elif car_x > target_x:
                car_x -= CAR_SPEED

            if car_y < target_y:
                car_y += CAR_SPEED
            elif car_y > target_y:
                car_y -= CAR_SPEED

            # Ensure car stays within boundaries
            if not is_within_boundaries(car_x, car_y):
                running = False
                print("Car is out of bounds.")
                break

            # Check for collision with obstacles
            if check_collision(car_x, car_y, obstacles):
                running = False
                print("Collision detected. Stopping near the red box.")
                break

            # Check if the car reached the waypoint
            if abs(car_x - target_x) < CAR_SPEED and abs(car_y - target_y) < CAR_SPEED:
                reached_waypoint = True
                visited_boxes.add(waypoint_index)
                # Wait for 5 seconds at the waypoint
                time.sleep(WAIT_TIME)
                waypoint_index = (waypoint_index + 1) % len(waypoints)
                reached_waypoint = False

        # Clear the screen
        window.fill(BLACK)

        # Draw obstacles (red boxes) and car
        draw_obstacles(obstacles)
        window.blit(car_image, (car_x, car_y))

        # Draw boundary (blue)
        pygame.draw.rect(window, BLUE, pygame.Rect(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT), 5)

        # Update display
        pygame.display.flip()
        clock.tick(30)

        # Check if all red boxes have been visited
        if len(visited_boxes) == len(waypoints):
            return

# Main function
def pymain():
    while True:
        automatic_mode()

if __name__ == "__main__":
    pymain()
