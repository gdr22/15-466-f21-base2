#include "Mode.hpp"

#include "Scene.hpp"

#include <glm/glm.hpp>

#include <vector>
#include <deque>
#include <array>
#include <cmath>

#define PI_F 3.14159265f
#define DEG2RAD 3.14159265f / 180.f
#define RADIUS 12

struct PlayMode : Mode {
	PlayMode();
	virtual ~PlayMode();

	//functions called by main loop:
	virtual bool handle_event(SDL_Event const &, glm::uvec2 const &window_size) override;
	virtual void update(float elapsed) override;
	virtual void draw(glm::uvec2 const &drawable_size) override;

	//----- game state -----

	//input tracking:
	struct Button {
		uint8_t downs = 0;
		uint8_t pressed = 0;
	} left, right, up;

	//local copy of the game scene (so code can change it during gameplay):
	Scene scene;

	//game state:
	float score;
	float game_over = false;
	//std::vector<Scene::Transform*> blocks;
	std::vector<float> angles;
	const uint32_t num_tiles = 16; //tiles per ring
	const uint32_t len_tiles = 8; //number of rings on screen at a time
	const float rotation_speed = 90;
	const float spawn_chance = 0.7f;
	float block_speed = 5.f;
	const float gravity = -30.0f;
	float cat_speed = 0.0f; //this is only cat's up and down speed;
	bool grounded = true;
	
	float rotation = 0.f;

	//cat transforms
	Scene::Transform* cat = nullptr;
	Scene::Transform* lbpeet = nullptr;
	Scene::Transform* lfpeet = nullptr;
	Scene::Transform* rbpeet = nullptr;
	Scene::Transform* rfpeet = nullptr;
	Scene::Transform* lear = nullptr;
	Scene::Transform* rear = nullptr;
	Scene::Transform* cattail = nullptr;
	std::array<Scene::Transform*, 8> catTransforms;

	//grass block info
	Scene::Transform grass;
	GLenum grass_vertex_type = GL_TRIANGLES;
	GLuint grass_vertex_start = 0;
	GLuint grass_vertex_count = 0;

	typedef struct Block {
		Scene::Transform* tile;
		float angle;
		float depth;

		void update_pos(float rotation) {
			float world_angle = angle + rotation;
			tile->rotation = glm::angleAxis(world_angle * DEG2RAD, glm::vec3(0.f, 1.f, 0.f));
			
			tile->position.x =  RADIUS * -std::sinf(world_angle * DEG2RAD);
			tile->position.z =  RADIUS * -std::cosf(world_angle * DEG2RAD);
			tile->position.z += RADIUS;
			tile->position.y = depth;
		}

	} Block;

	Block new_block(float angle, float depth);

	std::vector<Block> blocks;

	//hexapod leg to wobble:
	Scene::Transform *hip = nullptr;
	Scene::Transform *upper_leg = nullptr;
	Scene::Transform *lower_leg = nullptr;
	glm::quat hip_base_rotation;
	glm::quat upper_leg_base_rotation;
	glm::quat lower_leg_base_rotation;
	float wobble = 0.0f;
	
	//camera:
	Scene::Camera *camera = nullptr;

};
